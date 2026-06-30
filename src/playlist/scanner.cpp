#include "scanner.h"
#include "utils/thumbnail.h"
#include <QDirIterator>
#include <QFileInfo>

PlaylistScanner::PlaylistScanner() {}

QStringList PlaylistScanner::scanFolder(const QString& folderPath, bool recursive) {
    QStringList files;
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive) flags = QDirIterator::Subdirectories;
    QDirIterator it(folderPath, QDir::Files, flags);
    while (it.hasNext()) {
        QString path = it.next();
        if (isMediaFile(path)) files << path;
    }
    files.sort();
    return files;
}

QString PlaylistScanner::getFileTitle(const QString& filePath) {
    QFileInfo fi(filePath);
    QString base = fi.completeBaseName();
    return base.replace('_', ' ').replace('-', ' ').trimmed();
}
