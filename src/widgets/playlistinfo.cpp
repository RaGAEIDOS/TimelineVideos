#include "playlistinfo.h"
#include "utils/timeutils.h"
#include "i18n.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

PlaylistInfo::PlaylistInfo(QWidget* parent) : QWidget(parent) {
    setMinimumWidth(220);
    setMaximumWidth(300);

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(12,12,12,12);
    lay->setSpacing(8);

    m_headerLabel = new QLabel(_("playlist_info"));
    m_headerLabel->setStyleSheet("color:#ffffff;font-size:14px;font-weight:bold;");
    lay->addWidget(m_headerLabel);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color:#2a2a4e;");
    lay->addWidget(line);

    m_titleLabel = new QLabel();
    m_titleLabel->setStyleSheet("color:#e0e0e0;font-size:16px;font-weight:bold;padding:4px 0;");
    m_titleLabel->setWordWrap(true);
    lay->addWidget(m_titleLabel);

    m_countLabel = new QLabel();
    m_countLabel->setStyleSheet("color:#808090;font-size:12px;padding:2px 0;");
    lay->addWidget(m_countLabel);

    m_watchedLabel = new QLabel();
    m_watchedLabel->setStyleSheet("color:#808090;font-size:12px;padding:2px 0;");
    lay->addWidget(m_watchedLabel);

    m_durationLabel = new QLabel();
    m_durationLabel->setStyleSheet("color:#808090;font-size:12px;padding:2px 0;");
    lay->addWidget(m_durationLabel);

    QFrame* line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("color:#2a2a4e;");
    lay->addWidget(line2);

    m_progressBar = new QProgressBar();
    m_progressBar->setFixedHeight(8);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar{background:#1e1e2e;border:none;border-radius:4px;}"
        "QProgressBar::chunk{background:#6c5ce7;border-radius:4px;}"
    );
    lay->addWidget(m_progressBar);

    lay->addStretch();
    clear();
}

void PlaylistInfo::showPlaylistInfo(const QString& title, const QVector<QVariantMap>& videos) {
    m_titleLabel->setText(title);

    int total = videos.size();
    int watched = 0;
    double totalDuration = 0;
    double watchedDuration = 0;
    for (const auto& v : videos) {
        double dur = v["duration"].toDouble();
        totalDuration += dur;
        if (v["completed"].toInt()) {
            watched++;
            watchedDuration += dur;
        }
    }

    m_countLabel->setText(QString("%1: %2").arg(_("video_count"), QString::number(total)));

    double pct = total > 0 ? (watched * 100.0 / total) : 0;
    m_watchedLabel->setText(QString("%1: %2/%3 (%4%)").arg(_("watched_count"), QString::number(watched), QString::number(total), QString::number(pct, 'f', 1)));

    m_durationLabel->setText(QString("%1: %2 / %3").arg(_("total_duration"), formatDuration(watchedDuration), formatDuration(totalDuration)));

    m_progressBar->setRange(0, total > 0 ? total : 1);
    m_progressBar->setValue(watched);
    m_progressBar->setVisible(true);
}

void PlaylistInfo::clear() {
    m_titleLabel->setText("");
    m_countLabel->setText("");
    m_watchedLabel->setText("");
    m_durationLabel->setText("");
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
}

void PlaylistInfo::retranslate() {
    m_headerLabel->setText(_("playlist_info"));
}
