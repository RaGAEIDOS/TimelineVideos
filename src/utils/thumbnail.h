#pragma once
#include <QString>
#include <QPixmap>

bool isMediaFile(const QString& filePath);
QPixmap generateThumbnail(const QString& filePath, int width = 160, int height = 90);
QPixmap fallbackThumbnail(int width, int height);
QPixmap playlistThumbnail(const QString& title, int videoCount, int width = 160, int height = 90);
bool hasCachedThumbnail(const QString& filePath);
bool generateThumbnailToFile(const QString& filePath, const QString& outputPath, int width, int height);
QString thumbnailCachePath(const QString& filePath);
