#pragma once
#include <QString>
#include <QStringList>

class PlaylistScanner {
public:
    PlaylistScanner();
    QStringList scanFolder(const QString& folderPath, bool recursive = true);
    QString getFileTitle(const QString& filePath);
};
