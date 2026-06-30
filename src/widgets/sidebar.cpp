#include "sidebar.h"
#include "utils/thumbnail.h"
#include "utils/timeutils.h"
#include "i18n.h"
#include <QHBoxLayout>
#include <QScrollArea>
#include <QStyle>
#include <QFileInfo>
#include <QIcon>
#include <QSize>

// ---- PlaylistCard ----

PlaylistCard::PlaylistCard(const QVariantMap& data, QWidget* parent) : QFrame(parent) {
    m_playlistId = data["id"].toInt();
    bool active = data["is_active"].toBool();
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(80);
    setStyleSheet(QString(
        "PlaylistCard{background-color:%1;border-radius:8px;margin:2px 6px;}"
        "PlaylistCard:hover{background-color:#3a3a5e;}"
    ).arg(active ? "#2a2a4e" : "#1e1e2e"));

    QHBoxLayout* lay = new QHBoxLayout(this);
    lay->setContentsMargins(6,6,8,6);
    lay->setSpacing(8);

    // Thumbnail
    m_thumbLabel = new QLabel();
    QPixmap thumb = playlistThumbnail(data["title"].toString(), data["video_count"].toInt(), 90, 50);
    m_thumbLabel->setPixmap(thumb.scaled(90,50,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    m_thumbLabel->setFixedSize(90,50);
    m_thumbLabel->setStyleSheet("border-radius:6px;");
    lay->addWidget(m_thumbLabel);

    // Info
    QVBoxLayout* info = new QVBoxLayout();
    info->setSpacing(2);

    QLabel* title = new QLabel(data["title"].toString());
    title->setStyleSheet("color:#ffffff;font-size:10px;font-weight:bold;");
    title->setWordWrap(true);
    title->setFixedHeight(22);
    info->addWidget(title);

    int total = data["video_count"].toInt();
    int completed = data["completed_count"].toInt();
    QString ratio = QString("%1/%2").arg(completed).arg(total);
    double dur = data["total_duration"].toDouble();
    QLabel* meta = new QLabel(ratio + "  ·  " + formatDuration(dur));
    meta->setStyleSheet("color:#808090;font-size:9px;");
    info->addWidget(meta);

    // Progress bar
    if (total > 0) {
        QFrame* bar = new QFrame();
        bar->setFixedHeight(4);
        bar->setStyleSheet("background-color:#333;border-radius:2px;");
        QHBoxLayout* bl = new QHBoxLayout(bar);
        bl->setContentsMargins(0,0,0,0);
        QFrame* fill = new QFrame();
        double pct = getProgressPercent(completed, total);
        fill->setFixedWidth((int)(86 * pct / 100));
        fill->setFixedHeight(4);
        fill->setStyleSheet("background-color:#6c5ce7;border-radius:2px;");
        bl->addWidget(fill);
        bl->addStretch();
        info->addWidget(bar);
    }
    lay->addLayout(info, 1);
}

void PlaylistCard::mousePressEvent(QMouseEvent* e) {
    emit clicked(m_playlistId);
    QFrame::mousePressEvent(e);
}

// ---- PlaylistSidebar ----

PlaylistSidebar::PlaylistSidebar(QWidget* parent) : QWidget(parent) {
    setMinimumWidth(220);
    setMaximumWidth(320);
    setupUI();
}

void PlaylistSidebar::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    m_headerLabel = new QLabel(_("playlists"));
    m_headerLabel->setStyleSheet("color:#ffffff;font-size:16px;font-weight:bold;padding:14px 12px 6px;");
    layout->addWidget(m_headerLabel);

    // Buttons
    QWidget* btnW = new QWidget();
    btnW->setStyleSheet("background:transparent;");
    QHBoxLayout* bl = new QHBoxLayout(btnW);
    bl->setContentsMargins(8,2,8,6);
    bl->setSpacing(4);

    btnAddFolder = new QPushButton();
    btnAddFolder->setIcon(QIcon(":/icons/folder.svg"));
    btnAddFolder->setIconSize(QSize(18, 18));
    btnAddFolder->setToolTip(_("add_folder"));
    btnAddFolder->setStyleSheet("QPushButton{background:#2a2a4e;color:#fff;border:none;border-radius:6px;padding:6px 10px;}QPushButton:hover{background:#3a3a5e;}");
    connect(btnAddFolder, &QPushButton::clicked, this, &PlaylistSidebar::addFolderRequested);
    bl->addWidget(btnAddFolder);

    btnAddFiles = new QPushButton();
    btnAddFiles->setIcon(QIcon(":/icons/add_file.svg"));
    btnAddFiles->setIconSize(QSize(18, 18));
    btnAddFiles->setToolTip(_("add_files"));
    btnAddFiles->setStyleSheet("QPushButton{background:#2a2a4e;color:#fff;border:none;border-radius:6px;padding:6px 10px;}QPushButton:hover{background:#3a3a5e;}");
    connect(btnAddFiles, &QPushButton::clicked, this, &PlaylistSidebar::addFilesRequested);
    bl->addWidget(btnAddFiles);
    bl->addStretch();

    btnDelete = new QPushButton();
    btnDelete->setIcon(QIcon(":/icons/delete.svg"));
    btnDelete->setIconSize(QSize(18, 18));
    btnDelete->setToolTip(_("delete"));
    btnDelete->setStyleSheet("QPushButton{background:transparent;color:#e74c3c;border:none;border-radius:6px;padding:6px 10px;}QPushButton:hover{background:#3a1a1a;}");
    connect(btnDelete, &QPushButton::clicked, this, &PlaylistSidebar::deleteSelected);
    bl->addWidget(btnDelete);
    layout->addWidget(btnW);

    // Tabs
    m_tabs = new QTabWidget();
    m_tabs->setStyleSheet(
        "QTabWidget::pane{background:#16162a;border:none;}"
        "QTabBar::tab{background:#1a1a30;color:#808090;padding:6px 12px;font-size:11px;border:none;min-width:60px;}"
        "QTabBar::tab:selected{background:#16162a;color:#fff;border-bottom:2px solid #6c5ce7;}"
        "QTabBar::tab:hover{color:#fff;}"
    );

    // Playlists tab
    QWidget* plTab = new QWidget();
    QVBoxLayout* pll = new QVBoxLayout(plTab);
    pll->setContentsMargins(0,0,0,0);

    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet(
        "QScrollArea{background:transparent;border:none;}"
        "QScrollBar:vertical{background:#12122a;width:6px;border-radius:3px;}"
        "QScrollBar::handle:vertical{background:#2a2a4e;border-radius:3px;min-height:20px;}"
    );

    m_playlistContainer = new QWidget();
    m_playlistContainer->setStyleSheet("background:transparent;");
    m_playlistLayout = new QVBoxLayout(m_playlistContainer);
    m_playlistLayout->setContentsMargins(0,2,0,2);
    m_playlistLayout->setSpacing(2);
    m_playlistLayout->addStretch();

    scroll->setWidget(m_playlistContainer);
    pll->addWidget(scroll);
    m_tabs->addTab(plTab, _("playlists"));

    // History tab
    QWidget* histTab = new QWidget();
    QVBoxLayout* hl = new QVBoxLayout(histTab);
    hl->setContentsMargins(0,0,0,0);
    m_historyList = new QListWidget();
    m_historyList->setStyleSheet(
        "QListWidget{background:transparent;border:none;outline:none;}"
        "QListWidget::item{color:#a0a0b0;padding:8px;border-bottom:1px solid #2a2a4e;}"
        "QListWidget::item:hover{background:#2a2a4e;color:#fff;}"
        "QListWidget::item:selected{background:#6c5ce7;color:#fff;}"
    );
    connect(m_historyList, &QListWidget::itemClicked, this, &PlaylistSidebar::onHistoryClicked);
    hl->addWidget(m_historyList);
    m_tabs->addTab(histTab, _("history"));

    layout->addWidget(m_tabs, 1);
}

void PlaylistSidebar::loadPlaylists(const QVector<QVariantMap>& playlists) {
    m_cards.clear();
    while (m_playlistLayout->count() > 0) {
        QLayoutItem* item = m_playlistLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_playlistLayout->addStretch();

    for (const auto& pl : playlists) {
        QVariantMap data = pl;
        data["is_active"] = (data["id"].toInt() == m_selectedId);
        PlaylistCard* card = new PlaylistCard(data);
        connect(card, &PlaylistCard::clicked, this, &PlaylistSidebar::onCardClicked);
        m_cards << card;
        m_playlistLayout->insertWidget(m_playlistLayout->count() - 1, card);
    }
}

void PlaylistSidebar::loadHistory(const QVector<QVariantMap>& history) {
    m_historyList->clear();
    for (const auto& h : history) {
        QString text = h["title"].toString() + "  ·  " + h["started_at"].toString().left(10);
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, h["playlist_id"].toInt());
        m_historyList->addItem(item);
    }
}

void PlaylistSidebar::clearSelection() {
    m_selectedId = -1;
    for (auto* card : m_cards) {
        card->setStyleSheet("PlaylistCard{background-color:#1e1e2e;border-radius:8px;margin:2px 6px;}PlaylistCard:hover{background-color:#3a3a5e;}");
    }
}

void PlaylistSidebar::onCardClicked(int pid) {
    m_selectedId = pid;
    emit playlistSelected(pid);
}

void PlaylistSidebar::onHistoryClicked(QListWidgetItem* item) {
    int pid = item->data(Qt::UserRole).toInt();
    m_selectedId = pid;
    emit playlistSelected(pid);
}

void PlaylistSidebar::retranslate() {
    m_headerLabel->setText(_("playlists"));
    btnAddFolder->setToolTip(_("add_folder"));
    btnAddFiles->setToolTip(_("add_files"));
    btnDelete->setToolTip(_("delete"));
    m_tabs->setTabText(0, _("playlists"));
    m_tabs->setTabText(1, _("history"));
}

void PlaylistSidebar::deleteSelected() {
    if (m_selectedId >= 0)
        emit deleteRequested(m_selectedId);
}
