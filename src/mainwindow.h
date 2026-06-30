#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QTimer>
#include <QStatusBar>
#include "i18n.h"
#include "themes.h"
#include "playlist/manager.h"
#include "player/vlcplayer.h"
#include "player/controls.h"
#include "widgets/sidebar.h"
#include "widgets/videolist.h"
#include "widgets/toolbar.h"
#include "widgets/timelinebar.h"
#include "widgets/playlistinfo.h"
#include "utils/workers.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    void selectPlaylist(int playlistId);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onPlaylistSelected(int playlistId);
    void onAddFolder();
    void onAddFiles();
    void onDeletePlaylist(int playlistId);
    void onPlayVideoById(int videoId);
    void onPlayPause();
    void onStop();
    void onNext();
    void onPrevious();
    void onSeek(double percent);
    void onSeekRelative(double deltaSec);
    void onVolumeChange(int delta);
    void onVideoEnded();
    void onMediaLoaded(double duration);
    void onFullscreen();
    void onSearch(const QString& text);
    void onSort(const QString& key);
    void onFilter(const QString& key);
    void onViewToggle(bool grid);
    void onToggleWatched(int videoId, bool watched);
    void periodicSave();
    void onScanDone(const QStringList& files, const QString& title);
    void onScanError(const QString& err);

private:
    void setupUI();
    void setupMenu();
    void retranslateUI();
    void showShortcutsDialog();
    void applyTheme();
    void loadPlaylists();
    void loadHistory();
    void playVideo(int index);
    void clearCurrent();
    void updateTimelineStats();
    bool confirmDialog(const QString& title, const QString& text);
    void doDeletePlaylist(int playlistId);
    void toggleControls();

    I18n* m_i18n;
    PlaylistManager* m_manager;
    PlaylistSidebar* m_sidebar;
    VLCPlayer* m_player;
    PlaybackControls* m_controls;
    VideoListView* m_videoList;
    PlaylistInfo* m_playlistInfo;
    VideoToolbar* m_toolbar;
    TimelineBar* m_timelineBar;
    QSplitter* m_splitter;
    QStatusBar* m_status;
    QTimer m_saveTimer;
    ScanWorker* m_scanWorker = nullptr;

    int m_currentPlaylistId = -1;
    int m_currentVideoId = -1;
    int m_currentIndex = -1;
    QVector<QVariantMap> m_currentVideos;
    bool m_isFullscreen = false;
    bool m_controlsVisible = true;
    QList<int> m_savedSplitterSizes;
};
