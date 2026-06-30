#include "manager.h"
#include "scanner.h"
#include "utils/thumbnail.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <vlc/vlc.h>

static double getDurationFromVLC(const QString& filePath) {
    const char* args[] = {"--no-xlib", "--quiet", nullptr};
    libvlc_instance_t* inst = libvlc_new(2, args);
    if (!inst) return 0;
    libvlc_media_t* media = libvlc_media_new_path(inst, filePath.toUtf8().constData());
    if (!media) { libvlc_release(inst); return 0; }
    libvlc_media_parse_with_options(media, libvlc_media_parse_local, 0);
    libvlc_media_track_info_t* tracks = nullptr;
    int dur = libvlc_media_get_duration(media);
    libvlc_media_release(media);
    libvlc_release(inst);
    return dur > 0 ? dur / 1000.0 : 0;
}

// ---- PlaylistManager ----

PlaylistManager::PlaylistManager(QObject* parent) : QObject(parent) {
    m_db = new DatabaseManager(this);
}

int PlaylistManager::createFromFolder(const QString& folderPath, const QString& title) {
    int pid = m_db->addPlaylist(title);
    QStringList files = m_scanner.scanFolder(folderPath);
    addFilesToPlaylist(pid, files);
    return pid;
}

int PlaylistManager::createFromFiles(const QStringList& files, const QString& title) {
    int pid = m_db->addPlaylist(title);
    addFilesToPlaylist(pid, files);
    return pid;
}

void PlaylistManager::addFilesToPlaylist(int pid, const QStringList& files) {
    auto existing = m_db->getVideos(pid);
    QSet<QString> existingPaths;
    for (const auto& v : existing) existingPaths.insert(v["file_path"].toString());

    int sortOrder = existing.size();
    for (const auto& fp : files) {
        if (existingPaths.contains(fp)) continue;
        QString title = m_scanner.getFileTitle(fp);
        double dur = getDurationFromVLC(fp);
        m_db->addVideo(pid, fp, title, dur, sortOrder);
        sortOrder++;
    }
}

void PlaylistManager::deletePlaylist(int id) {
    m_db->deletePlaylist(id);
}

QVector<QVariantMap> PlaylistManager::getAllPlaylists() {
    return m_db->getAllPlaylists();
}

QVector<QVariantMap> PlaylistManager::getVideos(int pid) {
    return m_db->getVideos(pid);
}

void PlaylistManager::saveProgress(int vid, double pos, bool completed) {
    m_db->saveProgress(vid, pos, completed);
}

void PlaylistManager::addHistory(int pid) {
    m_db->addHistory(pid);
}

QVector<QVariantMap> PlaylistManager::getHistory(int limit) {
    return m_db->getHistory(limit);
}

QVector<QVariantMap> PlaylistManager::getPlaylistStats(int pid) {
    return m_db->getPlaylistStats(pid);
}

QVariantMap PlaylistManager::getLastPlayedVideo() {
    return m_db->getLastPlayedVideo();
}
