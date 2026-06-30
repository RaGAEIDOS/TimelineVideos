#include "videolist.h"
#include "utils/thumbnail.h"
#include "utils/workers.h"
#include "utils/timeutils.h"
#include "i18n.h"
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QMenu>
#include <QPainter>
#include <QRegularExpression>
#include <QApplication>

// ---- VideoCard ----

VideoCard::VideoCard(const QVariantMap& data, QWidget* parent) : QFrame(parent) {
    m_videoId = data["id"].toInt();
    m_filePath = data["file_path"].toString();
    m_duration = data["duration"].toDouble();
    m_completed = data["completed"].toInt();
    double position = data["position"].toDouble();
    QString rawTitle = data["title"].toString();
    if (rawTitle.isEmpty()) rawTitle = QFileInfo(m_filePath).fileName();
    m_title = QFileInfo(rawTitle).completeBaseName().replace('_',' ').replace('-',' ').trimmed();

    setFixedSize(200, 180);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(QString(
        "VideoCard{background-color:%1;border-radius:10px;border:1px solid #2a2a4e;}"
        "VideoCard:hover{background-color:#2a2a3e;border-color:#6c5ce7;}"
    ).arg(m_completed ? "#1a2e1a" : "#1e1e2e"));

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0,0,0,0);
    lay->setSpacing(0);

    m_thumbLabel = new QLabel();
    m_thumbLabel->setFixedSize(200, 112);
    m_thumbLabel->setStyleSheet("border-radius:10px 10px 0 0;");
    m_thumbLabel->setAlignment(Qt::AlignCenter);
    QPixmap thumb = generateThumbnail(m_filePath);
    m_thumbLabel->setPixmap(thumb.scaled(200,112,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    lay->addWidget(m_thumbLabel);

    QLabel* tl = new QLabel(m_title);
    tl->setStyleSheet(QString("color:%1;padding:6px 8px 2px;font-size:10px;").arg(m_completed ? "#808080" : "#e0e0e0"));
    tl->setWordWrap(true);
    tl->setFixedHeight(36);
    lay->addWidget(tl);

    QHBoxLayout* sub = new QHBoxLayout();
    sub->setContentsMargins(8,0,8,6);
    QLabel* dl = new QLabel(formatDuration(m_duration));
    dl->setStyleSheet("color:#606070;font-size:10px;");
    sub->addWidget(dl);
    sub->addStretch();
    m_statusLabel = new QLabel(m_completed ? QString::fromUtf8("\xE2\x9C\x85") : QString::fromUtf8("\xE2\xAC\x9C"));
    m_statusLabel->setStyleSheet("font-size:12px;");
    sub->addWidget(m_statusLabel);
    lay->addLayout(sub);
}

void VideoCard::setThumbnail(const QPixmap& thumb) {
    if (m_thumbLabel)
        m_thumbLabel->setPixmap(thumb.scaled(200, 112, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void VideoCard::updateState(bool completed) {
    m_completed = completed;
    m_statusLabel->setText(completed ? QString::fromUtf8("\xE2\x9C\x85") : QString::fromUtf8("\xE2\xAC\x9C"));
    setStyleSheet(QString(
        "VideoCard{background-color:%1;border-radius:10px;border:1px solid #2a2a4e;}"
        "VideoCard:hover{background-color:#2a2a3e;border-color:#6c5ce7;}"
    ).arg(completed ? "#1a2e1a" : "#1e1e2e"));
}

void VideoCard::mouseDoubleClickEvent(QMouseEvent* e) {
    emit playRequested(m_videoId);
    QFrame::mouseDoubleClickEvent(e);
}

void VideoCard::contextMenuEvent(QContextMenuEvent* e) {
    QMenu menu(this);
    menu.setStyleSheet("QMenu{background:#1a1a30;color:#fff;border:1px solid #2a2a4e;}QMenu::item:selected{background:#6c5ce7;}");
    if (m_completed)
        menu.addAction(_("mark_unwatched"))->setData(false);
    else
        menu.addAction(_("mark_watched"))->setData(true);
    QAction* sel = menu.exec(e->globalPos());
    if (sel) emit toggleWatched(m_videoId, sel->data().toBool());
}

// ---- VideoItemWidget ----

VideoItemWidget::VideoItemWidget(const QVariantMap& data, int index, QWidget* parent) : QFrame(parent) {
    m_videoId = data["id"].toInt();
    m_index = index;
    m_completed = data["completed"].toInt();
    m_duration = data["duration"].toDouble();
    QString rawTitle = data["title"].toString();
    if (rawTitle.isEmpty()) rawTitle = QFileInfo(data["file_path"].toString()).fileName();
    m_title = QFileInfo(rawTitle).completeBaseName().replace('_',' ').replace('-',' ').trimmed();

    double position = data["position"].toDouble();
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(QString(
        "VideoItemWidget{background-color:%1;border-radius:6px;padding:6px;margin:2px 0;}"
        "VideoItemWidget:hover{background-color:#2a2a3e;}"
    ).arg(m_completed ? "#1a2e1a" : "#1e1e2e"));

    QHBoxLayout* lay = new QHBoxLayout(this);
    lay->setContentsMargins(8,4,8,4);

    m_statusLabel = new QLabel(m_completed ? QString::fromUtf8("\xE2\x9C\x85") : QString::fromUtf8("\xE2\x96\xB6"));
    m_statusLabel->setFixedWidth(24);
    lay->addWidget(m_statusLabel);

    QLabel* idx = new QLabel(QString::number(index + 1));
    idx->setFixedWidth(24);
    idx->setStyleSheet("color:#808090;font-size:11px;");
    lay->addWidget(idx);

    QPixmap thumb = generateThumbnail(data["file_path"].toString(), 60, 34);
    QLabel* thumbLbl = new QLabel();
    thumbLbl->setPixmap(thumb.scaled(60,34,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    thumbLbl->setFixedSize(60,34);
    thumbLbl->setStyleSheet("border-radius:4px;");
    lay->addWidget(thumbLbl);

    m_titleLabel = new QLabel(m_title);
    m_titleLabel->setStyleSheet(QString("color:%1;font-size:10px;").arg(m_completed ? "#808080" : "#e0e0e0"));
    m_titleLabel->setWordWrap(true);
    lay->addWidget(m_titleLabel, 1);

    QLabel* dl = new QLabel(formatDuration(m_duration));
    dl->setStyleSheet("color:#606070;font-size:10px;");
    dl->setFixedWidth(50);
    dl->setAlignment(Qt::AlignRight);
    lay->addWidget(dl);

    if (!m_completed && position > 5) {
        QPushButton* resume = new QPushButton(_("resume"));
        resume->setFixedSize(60,22);
        resume->setStyleSheet("QPushButton{background:#6c5ce7;color:white;border:none;border-radius:4px;font-size:9px;}QPushButton:hover{background:#7c6cf7;}");
        connect(resume, &QPushButton::clicked, this, [this](){ emit playRequested(m_videoId); });
        lay->addWidget(resume);
    }
}

void VideoItemWidget::markCompleted(bool completed) {
    m_completed = completed;
    m_statusLabel->setText(completed ? QString::fromUtf8("\xE2\x9C\x85") : QString::fromUtf8("\xE2\x96\xB6"));
    QString c = completed ? "#808080" : "#e0e0e0";
    m_titleLabel->setStyleSheet(QString("color:%1;font-size:10px;").arg(c));
    setStyleSheet(QString(
        "VideoItemWidget{background-color:%1;border-radius:6px;padding:6px;margin:2px 0;}"
        "VideoItemWidget:hover{background-color:#2a2a3e;}"
    ).arg(completed ? "#1a2e1a" : "#1e1e2e"));
}

void VideoItemWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    emit playRequested(m_videoId);
    QFrame::mouseDoubleClickEvent(e);
}

void VideoItemWidget::contextMenuEvent(QContextMenuEvent* e) {
    QMenu menu(this);
    menu.setStyleSheet("QMenu{background:#1a1a30;color:#fff;border:1px solid #2a2a4e;}QMenu::item:selected{background:#6c5ce7;}");
    if (m_completed)
        menu.addAction(_("mark_unwatched"))->setData(false);
    else
        menu.addAction(_("mark_watched"))->setData(true);
    QAction* sel = menu.exec(e->globalPos());
    if (sel) emit toggleWatched(m_videoId, sel->data().toBool());
}

// ---- VideoListView ----

VideoListView::VideoListView(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0,0,0,0);
    lay->setSpacing(0);

    m_headerLabel = new QLabel(_("videos"));
    m_headerLabel->setStyleSheet("color:#ffffff;font-size:14px;font-weight:bold;padding:8px 12px;");
    lay->addWidget(m_headerLabel);

    m_hintLabel = new QLabel(_("double_click_hint"));
    m_hintLabel->setStyleSheet("color:#606070;font-size:11px;padding:0 12px 4px;");
    m_hintLabel->setAlignment(Qt::AlignLeft);
    lay->addWidget(m_hintLabel);

    m_stack = new QVBoxLayout();
    lay->addLayout(m_stack);
    setupGrid();
}

void VideoListView::setupList() {
    clearStack();
    m_listWidget = new QListWidget();
    m_listWidget->setStyleSheet("QListWidget{background:transparent;border:none;outline:none;}QListWidget::item{border:none;padding:1px;}");
    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_stack->addWidget(m_listWidget);
    m_gridMode = false;
}

void VideoListView::setupGrid() {
    clearStack();
    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea{background:transparent;border:none;}QScrollBar:vertical{background:#12122a;width:8px;border-radius:4px;}QScrollBar::handle:vertical{background:#2a2a4e;border-radius:4px;min-height:30px;}");
    QWidget* container = new QWidget();
    m_gridLayout = new QGridLayout(container);
    m_gridLayout->setContentsMargins(8,4,8,4);
    m_gridLayout->setSpacing(8);
    m_gridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    scroll->setWidget(container);
    m_stack->addWidget(scroll);
    m_gridMode = true;
}

void VideoListView::clearStack() {
    while (m_stack->count() > 0) {
        QWidget* w = m_stack->itemAt(0)->widget();
        if (w) { m_stack->removeWidget(w); w->deleteLater(); }
    }
}

void VideoListView::loadVideos(const QVector<QVariantMap>& videos) {
    m_allVideos = videos;
    applyFilters();
}

void VideoListView::setViewMode(bool grid) {
    if (grid == m_gridMode) return;
    if (grid) setupGrid(); else setupList();
    redisplay();
}

void VideoListView::setSearch(const QString& text) {
    m_searchText = text;
    applyFilters();
}

void VideoListView::setSort(const QString& key) {
    m_sortKey = key;
    applyFilters();
}

void VideoListView::setFilter(const QString& key) {
    m_filterKey = key;
    applyFilters();
}

void VideoListView::clear() {
    m_allVideos.clear();
    m_filteredVideos.clear();
    if (m_listWidget) { m_listWidget->clear(); m_listWidgets.clear(); }
    if (m_gridLayout) {
        while (m_gridLayout->count() > 0) {
            QLayoutItem* item = m_gridLayout->takeAt(0);
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        m_gridCards.clear();
    }
}

QString VideoListView::videoTitle(const QVariantMap& v) {
    QString raw = v["title"].toString();
    if (raw.isEmpty()) raw = QFileInfo(v["file_path"].toString()).fileName();
    return QFileInfo(raw).completeBaseName().replace('_',' ').replace('-',' ').trimmed();
}

void VideoListView::applyFilters() {
    QVector<QVariantMap> videos = m_allVideos;

    // Search
    if (!m_searchText.isEmpty()) {
        QString t = m_searchText.toLower();
        videos.erase(std::remove_if(videos.begin(), videos.end(),
            [&](const QVariantMap& v) { return videoTitle(v).toLower().indexOf(t) < 0; }), videos.end());
    }

    // Filter
    if (m_filterKey == "watched")
        videos.erase(std::remove_if(videos.begin(), videos.end(),
            [](const QVariantMap& v) { return !v["completed"].toInt(); }), videos.end());
    else if (m_filterKey == "unwatched")
        videos.erase(std::remove_if(videos.begin(), videos.end(),
            [](const QVariantMap& v) { return v["completed"].toInt(); }), videos.end());

    // Sort
    if (m_sortKey == "a-z" || m_sortKey == "title")
        std::sort(videos.begin(), videos.end(), [&](const QVariantMap& a, const QVariantMap& b) {
            return videoTitle(a).toLower() < videoTitle(b).toLower();
        });
    else if (m_sortKey == "z-a")
        std::sort(videos.begin(), videos.end(), [&](const QVariantMap& a, const QVariantMap& b) {
            return videoTitle(a).toLower() > videoTitle(b).toLower();
        });
    else if (m_sortKey == "0-9" || m_sortKey == "title_number")
        std::sort(videos.begin(), videos.end(), [&](const QVariantMap& a, const QVariantMap& b) {
            QRegularExpression re("(\\d+)");
            auto m1 = re.match(videoTitle(a));
            auto m2 = re.match(videoTitle(b));
            int n1 = m1.hasMatch() ? m1.captured(1).toInt() : 999999;
            int n2 = m2.hasMatch() ? m2.captured(1).toInt() : 999999;
            return n1 < n2;
        });

    m_filteredVideos = videos;
    redisplay();
}

void VideoListView::redisplay() {
    if (m_gridMode) redisplayGrid(); else redisplayList();
}

void VideoListView::redisplayList() {
    if (!m_listWidget) { setupList(); return; }
    m_listWidget->clear();
    m_listWidgets.clear();
    for (int i = 0; i < m_filteredVideos.size(); i++) {
        VideoItemWidget* w = new VideoItemWidget(m_filteredVideos[i], i);
        connect(w, &VideoItemWidget::playRequested, this, &VideoListView::videoPlayRequested);
        connect(w, &VideoItemWidget::toggleWatched, this, &VideoListView::videoToggleWatched);
        m_listWidgets << w;
        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(w->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, w);
    }
    m_hintLabel->setVisible(m_filteredVideos.isEmpty());
    scheduleThumbnails();
}

void VideoListView::redisplayGrid() {
    if (!m_gridLayout) { setupGrid(); return; }
    while (m_gridLayout->count() > 0) {
        QLayoutItem* item = m_gridLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_gridCards.clear();

    int cols = qMax(1, width() / 216);
    for (int i = 0; i < m_filteredVideos.size(); i++) {
        VideoCard* card = new VideoCard(m_filteredVideos[i]);
        connect(card, &VideoCard::playRequested, this, &VideoListView::videoPlayRequested);
        connect(card, &VideoCard::toggleWatched, this, &VideoListView::videoToggleWatched);
        m_gridCards << card;
        m_gridLayout->addWidget(card, i / cols, i % cols);
    }
    m_hintLabel->setVisible(m_filteredVideos.isEmpty());
    scheduleThumbnails();
}

void VideoListView::updateVideoState(int videoId, bool completed) {
    for (auto& v : m_allVideos)
        if (v["id"].toInt() == videoId) { v["completed"] = completed ? 1 : 0; break; }
    for (auto* w : m_listWidgets)
        if (w->videoId() == videoId) { w->markCompleted(completed); break; }
    for (auto* c : m_gridCards)
        if (c->videoId() == videoId) { c->updateState(completed); break; }
}

void VideoListView::retranslate() {
    m_headerLabel->setText(_("videos"));
    m_hintLabel->setText(_("double_click_hint"));
}

void VideoListView::scheduleThumbnails() {
    for (auto* w : m_thumbWorkers) { w->quit(); w->wait(2000); w->deleteLater(); }
    m_thumbWorkers.clear();
    m_thumbQueue.clear();

    if (m_gridMode) {
        for (auto* c : m_gridCards)
            if (!hasCachedThumbnail(c->filePath()))
                m_thumbQueue << c;
    }
    processNextThumb();
}

void VideoListView::processNextThumb() {
    while (!m_thumbQueue.isEmpty()) {
        VideoCard* card = m_thumbQueue.takeFirst();
        QString fp = card->filePath();
        if (hasCachedThumbnail(fp)) continue;

        QString cached = thumbnailCachePath(fp);
        ThumbnailWorker* tw = new ThumbnailWorker(fp, cached, QSize(200, 112));
        connect(tw, &ThumbnailWorker::thumbReady, this, [this, fp](const QString&) {
            QPixmap p(thumbnailCachePath(fp));
            if (p.isNull()) return;
            if (m_gridMode)
                for (auto* c : m_gridCards)
                    if (c->filePath() == fp) { c->setThumbnail(p); break; }
        });
        connect(tw, &ThumbnailWorker::finished, this, [this, tw]() {
            tw->deleteLater();
            processNextThumb();
        });
        m_thumbWorkers << tw;
        tw->start();
        return;
    }
}

void VideoListView::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (m_gridMode && m_gridLayout) redisplayGrid();
}
