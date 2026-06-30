#include "workers.h"
#include "thumbnail.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QPainter>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

ScanWorker::ScanWorker(const QString& folderPath, bool recursive)
    : m_folderPath(folderPath), m_recursive(recursive) {}

void ScanWorker::cancel() { m_cancelled = true; }

void ScanWorker::run() {
    QStringList files;
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (m_recursive) flags = QDirIterator::Subdirectories;
    QDirIterator it(m_folderPath, QDir::Files, flags);
    while (it.hasNext()) {
        if (m_cancelled) return;
        QString path = it.next();
        if (isMediaFile(path)) {
            files << path;
            emit progressUpdated(QString("Found %1 files...").arg(files.size()));
        }
    }
    emit progressUpdated(QString("Scanning complete: %1 files").arg(files.size()));
    if (!m_cancelled) emit scanFinished(files);
}

// ---- ThumbnailWorker ----

ThumbnailWorker::ThumbnailWorker(const QString& filePath, const QString& thumbPath, QSize size)
    : m_filePath(filePath), m_thumbPath(thumbPath), m_size(size) {}

void ThumbnailWorker::run() {
#ifdef Q_OS_WIN
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif
    qputenv("VLC_PLUGIN_PATH", "C:/Program Files/VideoLAN/VLC/plugins");
    if (!generateThumbnailToFile(m_filePath, m_thumbPath, m_size.width(), m_size.height())) {
        QPixmap fb = fallbackThumbnail(m_size.width(), m_size.height());
        fb.save(m_thumbPath);
    }
#ifdef Q_OS_WIN
    CoUninitialize();
#endif
    emit thumbReady(m_filePath);
}
