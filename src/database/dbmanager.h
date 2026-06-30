#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QVariantMap>
#include <QVector>

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    // Playlists
    int addPlaylist(const QString& title, const QString& desc = "", const QString& thumb = "");
    void updatePlaylist(int id, const QString& title = "", const QString& desc = "", const QString& thumb = "");
    void deletePlaylist(int id);
    QVariantMap getPlaylist(int id);
    QVector<QVariantMap> getAllPlaylists();
    QVector<QVariantMap> getPlaylistStats(int id);

    // Videos
    int addVideo(int playlistId, const QString& filePath, const QString& title = "", double duration = 0, int sortOrder = 0);
    void deleteVideo(int id);
    void updateVideoDuration(int id, double duration);
    QVector<QVariantMap> getVideos(int playlistId);
    QVariantMap getVideo(int id);

    // Progress
    void saveProgress(int videoId, double position, bool completed = false);
    QVariantMap getProgress(int videoId);

    // History
    void addHistory(int playlistId);
    QVector<QVariantMap> getHistory(int limit = 20);
    QVariantMap getLastPlayedVideo();

private:
    void initDb();
    QSqlDatabase m_db;
};
