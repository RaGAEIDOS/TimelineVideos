#include "dbmanager.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDir>
#include <QDebug>
#include <QDateTime>

static QVariantMap recordToMap(const QSqlQuery& q) {
    QVariantMap map;
    QSqlRecord rec = q.record();
    for (int i = 0; i < rec.count(); i++)
        map[rec.fieldName(i)] = rec.value(i);
    return map;
}

DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {
    initDb();
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) m_db.close();
}

void DatabaseManager::initDb() {
    QString dataDir = qgetenv("DATA_DIR");
    if (dataDir.isEmpty()) dataDir = QDir::currentPath() + "/data";
    QDir().mkpath(dataDir);
    QString dbPath = dataDir + "/timeline.db";

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        qWarning() << "Failed to open database:" << m_db.lastError().text();
        return;
    }
    m_db.exec("PRAGMA journal_mode=WAL");
    m_db.exec("PRAGMA foreign_keys=ON");

    m_db.exec(
        "CREATE TABLE IF NOT EXISTS playlists ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL, description TEXT DEFAULT '',"
        "thumbnail_path TEXT DEFAULT '',"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "last_played TIMESTAMP DEFAULT NULL)"
    );
    m_db.exec(
        "CREATE TABLE IF NOT EXISTS videos ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "playlist_id INTEGER NOT NULL,"
        "file_path TEXT NOT NULL, title TEXT DEFAULT '',"
        "duration REAL DEFAULT 0, sort_order INTEGER DEFAULT 0,"
        "FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE)"
    );
    m_db.exec(
        "CREATE TABLE IF NOT EXISTS watch_progress ("
        "video_id INTEGER PRIMARY KEY,"
        "position REAL DEFAULT 0, completed INTEGER DEFAULT 0,"
        "last_watched TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (video_id) REFERENCES videos(id) ON DELETE CASCADE)"
    );
    m_db.exec(
        "CREATE TABLE IF NOT EXISTS watch_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "playlist_id INTEGER NOT NULL,"
        "started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE)"
    );
    m_db.exec("CREATE INDEX IF NOT EXISTS idx_videos_playlist ON videos(playlist_id)");
    m_db.exec("CREATE INDEX IF NOT EXISTS idx_watch_history_playlist ON watch_history(playlist_id)");
}

// ---- Playlists ----

int DatabaseManager::addPlaylist(const QString& title, const QString& desc, const QString& thumb) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO playlists (title,description,thumbnail_path) VALUES (?,?,?)");
    q.addBindValue(title); q.addBindValue(desc); q.addBindValue(thumb);
    q.exec();
    return q.lastInsertId().toInt();
}

void DatabaseManager::updatePlaylist(int id, const QString& title, const QString& desc, const QString& thumb) {
    QSqlQuery q(m_db);
    QStringList sets;
    QVariantList vals;
    if (!title.isEmpty()) { sets << "title=?"; vals << title; }
    if (!desc.isEmpty()) { sets << "description=?"; vals << desc; }
    if (!thumb.isEmpty()) { sets << "thumbnail_path=?"; vals << thumb; }
    if (sets.isEmpty()) return;
    vals << id;
    q.prepare("UPDATE playlists SET " + sets.join(",") + " WHERE id=?");
    for (const auto& v : vals) q.addBindValue(v);
    q.exec();
}

void DatabaseManager::deletePlaylist(int id) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM playlists WHERE id=?");
    q.addBindValue(id);
    q.exec();
}

QVariantMap DatabaseManager::getPlaylist(int id) {
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM playlists WHERE id=?");
    q.addBindValue(id);
    q.exec();
    if (q.next()) return recordToMap(q);
    return {};
}

QVector<QVariantMap> DatabaseManager::getAllPlaylists() {
    QSqlQuery q(m_db);
    q.exec(
        "SELECT p.*, COUNT(v.id) as video_count,"
        "SUM(CASE WHEN wp.completed=1 THEN 1 ELSE 0 END) as completed_count,"
        "ROUND(SUM(v.duration),2) as total_duration "
        "FROM playlists p "
        "LEFT JOIN videos v ON v.playlist_id=p.id "
        "LEFT JOIN watch_progress wp ON wp.video_id=v.id "
        "GROUP BY p.id ORDER BY p.last_played DESC, p.created_at DESC"
    );
    QVector<QVariantMap> result;
    while (q.next()) result << recordToMap(q);
    return result;
}

QVector<QVariantMap> DatabaseManager::getPlaylistStats(int id) {
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT COUNT(v.id) as total_videos,"
        "ROUND(SUM(v.duration),2) as total_duration,"
        "SUM(CASE WHEN wp.completed=1 THEN 1 ELSE 0 END) as completed_count,"
        "ROUND(SUM(CASE WHEN wp.completed=1 THEN v.duration ELSE 0 END),2) as watched_duration,"
        "ROUND(SUM(CASE WHEN wp.completed=0 OR wp.completed IS NULL THEN v.duration ELSE 0 END),2) as remaining_duration "
        "FROM videos v LEFT JOIN watch_progress wp ON wp.video_id=v.id WHERE v.playlist_id=?"
    );
    q.addBindValue(id);
    q.exec();
    QVector<QVariantMap> result;
    while (q.next()) result << recordToMap(q);
    return result;
}

// ---- Videos ----

int DatabaseManager::addVideo(int pid, const QString& fp, const QString& title, double dur, int sort) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO videos (playlist_id,file_path,title,duration,sort_order) VALUES (?,?,?,?,?)");
    q.addBindValue(pid); q.addBindValue(fp); q.addBindValue(title); q.addBindValue(dur); q.addBindValue(sort);
    q.exec();
    return q.lastInsertId().toInt();
}

void DatabaseManager::deleteVideo(int id) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM videos WHERE id=?");
    q.addBindValue(id);
    q.exec();
}

void DatabaseManager::updateVideoDuration(int id, double duration) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE videos SET duration=? WHERE id=?");
    q.addBindValue(duration); q.addBindValue(id);
    q.exec();
}

QVector<QVariantMap> DatabaseManager::getVideos(int pid) {
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT v.*, wp.position, wp.completed, wp.last_watched "
        "FROM videos v LEFT JOIN watch_progress wp ON wp.video_id=v.id "
        "WHERE v.playlist_id=? ORDER BY v.sort_order ASC, v.id ASC"
    );
    q.addBindValue(pid);
    q.exec();
    QVector<QVariantMap> result;
    while (q.next()) result << recordToMap(q);
    return result;
}

QVariantMap DatabaseManager::getVideo(int id) {
    QSqlQuery q(m_db);
    q.prepare("SELECT v.*, wp.position, wp.completed FROM videos v LEFT JOIN watch_progress wp ON wp.video_id=v.id WHERE v.id=?");
    q.addBindValue(id);
    q.exec();
    if (q.next()) return recordToMap(q);
    return {};
}

// ---- Progress ----

void DatabaseManager::saveProgress(int vid, double pos, bool completed) {
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO watch_progress (video_id,position,completed,last_watched) "
        "VALUES (?,?,?,CURRENT_TIMESTAMP) "
        "ON CONFLICT(video_id) DO UPDATE SET "
        "position=excluded.position, completed=excluded.completed, last_watched=CURRENT_TIMESTAMP"
    );
    q.addBindValue(vid); q.addBindValue(pos); q.addBindValue(completed ? 1 : 0);
    q.exec();
}

QVariantMap DatabaseManager::getProgress(int vid) {
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM watch_progress WHERE video_id=?");
    q.addBindValue(vid);
    q.exec();
    if (q.next()) return recordToMap(q);
    return {};
}

// ---- History ----

void DatabaseManager::addHistory(int pid) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO watch_history (playlist_id) VALUES (?)");
    q.addBindValue(pid);
    q.exec();
    q.prepare("UPDATE playlists SET last_played=CURRENT_TIMESTAMP WHERE id=?");
    q.addBindValue(pid);
    q.exec();
}

QVariantMap DatabaseManager::getLastPlayedVideo() {
    QSqlQuery q(m_db);
    q.exec(
        "SELECT wp.video_id, wp.position, wp.completed, wp.last_watched,"
        "v.playlist_id, v.file_path, v.title, v.duration "
        "FROM watch_progress wp "
        "JOIN videos v ON v.id = wp.video_id "
        "ORDER BY wp.last_watched DESC LIMIT 1"
    );
    if (q.next()) return recordToMap(q);
    return {};
}

QVector<QVariantMap> DatabaseManager::getHistory(int limit) {
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT wh.id,wh.started_at,p.id as playlist_id,p.title,p.thumbnail_path "
        "FROM watch_history wh JOIN playlists p ON p.id=wh.playlist_id "
        "ORDER BY wh.started_at DESC LIMIT ?"
    );
    q.addBindValue(limit);
    q.exec();
    QVector<QVariantMap> result;
    while (q.next()) result << recordToMap(q);
    return result;
}
