#pragma once
#include <QObject>
#include <QStringList>
#include "database/dbmanager.h"
#include "playlist/scanner.h"

class PlaylistManager : public QObject {
    Q_OBJECT
public:
    explicit PlaylistManager(QObject* parent = nullptr);

    int createFromFolder(const QString& folderPath, const QString& title);
    int createFromFiles(const QStringList& files, const QString& title);
    void deletePlaylist(int id);
    QVector<QVariantMap> getAllPlaylists();
    QVector<QVariantMap> getVideos(int playlistId);
    void saveProgress(int videoId, double position, bool completed = false);
    void addHistory(int playlistId);
    QVector<QVariantMap> getHistory(int limit = 20);
    QVector<QVariantMap> getPlaylistStats(int playlistId);

    DatabaseManager* db() { return m_db; }

private:
    void addFilesToPlaylist(int playlistId, const QStringList& files);
    DatabaseManager* m_db;
    PlaylistScanner m_scanner;
};
