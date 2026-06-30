#include "thumbnail.h"
#include "logger.h"
#include <QFileInfo>
#include <QProcess>
#include <QPainter>
#include <QLinearGradient>
#include <QDir>
#include <QDateTime>
#include <QCryptographicHash>
#include <QUrl>
#include <QThread>
#include <unordered_map>
#include <vlc/vlc.h>

static const QStringList MEDIA_EXTS = {
    ".mp4",".avi",".mkv",".mov",".wmv",".flv",".webm",".m4v",
    ".mp3",".wav",".flac",".ogg",".aac",".wma",".m4a"
};

bool isMediaFile(const QString& filePath) {
    for (const auto& ext : MEDIA_EXTS)
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) return true;
    return false;
}

static QString thumbCacheDir() {
    QString d = qgetenv("DATA_DIR");
    if (d.isEmpty()) d = QDir::currentPath() + "/data";
    d += "/thumbs";
    QDir().mkpath(d);
    return d;
}

static QString thumbPathFor(const QString& filePath) {
    QByteArray hash = QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex();
    return thumbCacheDir() + "/" + QString::fromLatin1(hash) + ".png";
}

QString thumbnailCachePath(const QString& filePath) { return thumbPathFor(filePath); }

bool hasCachedThumbnail(const QString& filePath) {
    return QFileInfo::exists(thumbPathFor(filePath));
}

bool generateThumbnailToFile(const QString& filePath, const QString& outputPath, int width, int height) {
    // libvlc directly (COM must be initialized by caller on Windows)
    {
        const char* vlcArgs[] = {"-I", "dummy", "--quiet", "--no-audio",
                                 "--no-video-title-show", "--no-plugins-cache", nullptr};
        libvlc_instance_t* vlc = libvlc_new(6, vlcArgs);
        if (vlc) {
            QUrl url = QUrl::fromLocalFile(filePath);
            QByteArray uri = url.toEncoded();
            libvlc_media_t* media = libvlc_media_new_location(vlc, uri.constData());
            if (media) {
                libvlc_media_player_t* mp = libvlc_media_player_new_from_media(media);
                if (mp) {
                    libvlc_media_parse_with_options(media, libvlc_media_parse_local, 3000);
                    libvlc_time_t duration = libvlc_media_get_duration(media);
                    libvlc_media_player_play(mp);
                    for (int i = 0; i < 10; i++) {
                        QThread::msleep(100);
                        libvlc_state_t state = libvlc_media_player_get_state(mp);
                        if (state == libvlc_Playing || state == libvlc_Paused) break;
                    }
                    if (duration > 0) libvlc_media_player_set_time(mp, qMin<libvlc_time_t>(duration / 4, 30000));
                    QThread::msleep(200);
                    QByteArray out = QDir::toNativeSeparators(outputPath).toUtf8();
                    int ret = libvlc_video_take_snapshot(mp, 0, out.constData(), width, height);
                    libvlc_media_player_stop(mp);
                    libvlc_media_player_release(mp);
                    libvlc_media_release(media);
                    libvlc_release(vlc);
                    if (ret == 0 && QFileInfo::exists(outputPath)) return true;
                    return false;
                }
                libvlc_media_release(media);
            }
            libvlc_release(vlc);
        }
    }

    // ffmpeg (skip if not available)
    {
        QProcess which;
        which.start("ffmpeg", {"-version"});
        if (which.waitForFinished(2000)) {
            QProcess proc;
            QStringList args;
            args << "-y" << "-ss" << "5"
                 << "-i" << QDir::toNativeSeparators(filePath)
                 << "-vframes" << "1"
                 << "-s" << QString("%1x%2").arg(width).arg(height)
                 << QDir::toNativeSeparators(outputPath)
                 << "-loglevel" << "error";
            proc.start("ffmpeg", args);
            if (proc.waitForFinished(8000) && QFileInfo::exists(outputPath)) return true;
        }
    }

    return false;
}

QPixmap generateThumbnail(const QString& filePath, int width, int height) {
    QString cached = thumbPathFor(filePath);
    if (QFileInfo::exists(cached)) {
        QPixmap p(cached);
        if (!p.isNull()) return p;
    }
    return fallbackThumbnail(width, height);
}

QPixmap fallbackThumbnail(int width, int height) {
    QPixmap pix(width, height);
    pix.fill(QColor(30, 30, 50));
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(108, 92, 231), 2));
    painter.setBrush(QBrush(QColor(108, 92, 231, 80)));
    int cx = width / 2, cy = height / 2, r = qMin(width, height) / 6;
    painter.drawEllipse(QPointF(cx, cy), r, r);
    QPolygonF tri;
    tri << QPointF(cx - r * 0.35, cy - r * 0.45)
        << QPointF(cx + r * 0.5, cy)
        << QPointF(cx - r * 0.35, cy + r * 0.45);
    painter.setBrush(Qt::white);
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(tri);
    painter.end();
    return pix;
}

QPixmap playlistThumbnail(const QString& title, int videoCount, int width, int height) {
    static std::unordered_map<QString, QPixmap> cache;
    QString key = title + QString::number(videoCount);
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;

    QPixmap pix(width, height);
    pix.fill(QColor(25, 25, 50));
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    QLinearGradient grad(0, 0, width, height);
    grad.setColorAt(0, QColor(60, 40, 120));
    grad.setColorAt(1, QColor(30, 20, 80));
    painter.fillRect(0, 0, width, height, grad);
    painter.setPen(QPen(QColor(108, 92, 231), 2));
    for (int i = 0; i < 3; i++) {
        int y = height / 2 - 10 + i * 8;
        int x = width / 2 - 12;
        painter.drawRoundedRect(x, y, 24, 6, 2, 2);
    }
    painter.end();
    cache[key] = pix;
    return pix;
}
