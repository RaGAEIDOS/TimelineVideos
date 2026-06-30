#include "mainwindow.h"
#include "utils/thumbnail.h"
#include "utils/logger.h"
#include "utils/timeutils.h"
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSlider>
#include <QStyle>
#include <QDir>
#include <QDialog>
#include <QTextBrowser>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QActionGroup>

#define LOG Logger::instance()

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    LOG.info("MainWindow", "Starting TimelineVideo application");
    m_i18n = &I18n::instance();
    m_manager = new PlaylistManager(this);
    setupUI();
    setupMenu();
    applyTheme();
    loadPlaylists();
    loadHistory();

    connect(&m_saveTimer, &QTimer::timeout, this, &MainWindow::periodicSave);
    m_saveTimer.setInterval(30000);
    m_saveTimer.start();
    restoreLastSession();
    LOG.info("MainWindow", "Application started successfully");
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    setWindowTitle(_("app_title"));
    setGeometry(100, 100, 1400, 800);
    setMinimumSize(900, 500);

    QWidget* central = new QWidget();
    setCentralWidget(central);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->setHandleWidth(2);
    m_splitter->setChildrenCollapsible(false);

    // Sidebar
    m_sidebar = new PlaylistSidebar();
    connect(m_sidebar, &PlaylistSidebar::playlistSelected, this, &MainWindow::onPlaylistSelected);
    connect(m_sidebar, &PlaylistSidebar::addFolderRequested, this, &MainWindow::onAddFolder);
    connect(m_sidebar, &PlaylistSidebar::addFilesRequested, this, &MainWindow::onAddFiles);
    connect(m_sidebar, &PlaylistSidebar::deleteRequested, this, &MainWindow::onDeletePlaylist);
    m_splitter->addWidget(m_sidebar);

    // Center: player + controls
    QWidget* centerWidget = new QWidget();
    centerWidget->setMinimumWidth(320);
    QVBoxLayout* centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(4,4,4,4);
    centerLayout->setSpacing(4);

    m_player = new VLCPlayer();
    centerLayout->addWidget(m_player, 1);

    connect(m_player, &VLCPlayer::fullscreenRequested, this, &MainWindow::onFullscreen);

    m_controls = new PlaybackControls();
    connect(m_controls, &PlaybackControls::playPauseClicked, this, &MainWindow::onPlayPause);
    connect(m_controls, &PlaybackControls::stopClicked, this, &MainWindow::onStop);
    connect(m_controls, &PlaybackControls::nextClicked, this, &MainWindow::onNext);
    connect(m_controls, &PlaybackControls::previousClicked, this, &MainWindow::onPrevious);
    connect(m_controls, &PlaybackControls::volumeChanged, m_player, &VLCPlayer::setVolume);
    connect(m_controls, &PlaybackControls::speedChanged, m_player, &VLCPlayer::setSpeed);
    connect(m_controls, &PlaybackControls::seekChanged, this, &MainWindow::onSeek);
    connect(m_controls, &PlaybackControls::fullscreenClicked, this, &MainWindow::onFullscreen);
    centerLayout->addWidget(m_controls);

    connect(m_player, &VLCPlayer::positionChanged, m_controls, &PlaybackControls::updateTime);
    connect(m_player, &VLCPlayer::endReached, this, &MainWindow::onVideoEnded);
    connect(m_player, &VLCPlayer::mediaLoaded, this, &MainWindow::onMediaLoaded);

    m_timelineBar = new TimelineBar();
    centerLayout->addWidget(m_timelineBar);

    m_splitter->addWidget(centerWidget);

    // Right: toolbar + video list
    QWidget* rightWidget = new QWidget();
    rightWidget->setMinimumWidth(200);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0,0,0,0);
    rightLayout->setSpacing(0);

    m_toolbar = new VideoToolbar();
    connect(m_toolbar, &VideoToolbar::searchChanged, this, &MainWindow::onSearch);
    connect(m_toolbar, &VideoToolbar::sortChanged, this, &MainWindow::onSort);
    connect(m_toolbar, &VideoToolbar::filterChanged, this, &MainWindow::onFilter);
    connect(m_toolbar, &VideoToolbar::viewToggled, this, &MainWindow::onViewToggle);
    rightLayout->addWidget(m_toolbar);

    m_videoList = new VideoListView();
    connect(m_videoList, &VideoListView::videoPlayRequested, this, &MainWindow::onPlayVideoById);
    connect(m_videoList, &VideoListView::videoToggleWatched, this, &MainWindow::onToggleWatched);
    rightLayout->addWidget(m_videoList, 1);

    m_splitter->addWidget(rightWidget);

    // Right-info: playlist info panel
    m_playlistInfo = new PlaylistInfo();
    m_playlistInfo->hide();
    m_splitter->addWidget(m_playlistInfo);

    m_splitter->setSizes({240, 600, 320, 0});
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 2);
    m_splitter->setStretchFactor(2, 1);
    m_splitter->setStretchFactor(3, 0);

    mainLayout->addWidget(m_splitter, 1);

    m_status = statusBar();
    m_status->showMessage(_("select_playlist"));
}

void MainWindow::selectPlaylist(int playlistId) {
    onPlaylistSelected(playlistId);
}

void MainWindow::retranslateUI() {
    setWindowTitle(_("app_title"));
    menuBar()->clear();
    setupMenu();
    m_toolbar->retranslate();
    m_sidebar->retranslate();
    m_controls->retranslate();
    m_timelineBar->retranslate();
    m_videoList->retranslate();
    m_playlistInfo->retranslate();
    m_status->showMessage(_("select_playlist"));
}

void MainWindow::setupMenu() {
    QMenuBar* mb = menuBar();

    QMenu* fileMenu = mb->addMenu(_("open_file"));
    QAction* actFolder = fileMenu->addAction(_("add_folder"));
    actFolder->setShortcut(QKeySequence("Ctrl+F"));
    connect(actFolder, &QAction::triggered, this, &MainWindow::onAddFolder);

    QAction* actFiles = fileMenu->addAction(_("add_files"));
    actFiles->setShortcut(QKeySequence("Ctrl+O"));
    connect(actFiles, &QAction::triggered, this, &MainWindow::onAddFiles);

    fileMenu->addSeparator();
    QAction* actQuit = fileMenu->addAction("Quit");
    actQuit->setShortcut(QKeySequence("Ctrl+Q"));
    connect(actQuit, &QAction::triggered, this, &QWidget::close);

    QMenu* viewMenu = mb->addMenu(_("view_grid"));
    QAction* actGrid = viewMenu->addAction(_("view_grid"));
    actGrid->setShortcut(QKeySequence("Ctrl+G"));
    connect(actGrid, &QAction::triggered, this, [this](){ m_videoList->setViewMode(true); });

    QAction* actList = viewMenu->addAction(_("view_list"));
    actList->setShortcut(QKeySequence("Ctrl+L"));
    connect(actList, &QAction::triggered, this, [this](){ m_videoList->setViewMode(false); });

    viewMenu->addSeparator();
    QAction* actFS = viewMenu->addAction(_("fullscreen"));
    actFS->setShortcut(QKeySequence("F11"));
    connect(actFS, &QAction::triggered, this, &MainWindow::onFullscreen);

    QMenu* settingsMenu = mb->addMenu(_("settings"));
    QMenu* langMenu = settingsMenu->addMenu(_("language"));
    QAction* actAr = langMenu->addAction(_("arabic"));
    connect(actAr, &QAction::triggered, this, [this](){ m_i18n->setLanguage("ar"); retranslateUI(); });
    QAction* actEn = langMenu->addAction(_("english"));
    connect(actEn, &QAction::triggered, this, [this](){ m_i18n->setLanguage("en"); retranslateUI(); });

    // Theme submenu
    QMenu* themeMenu = settingsMenu->addMenu(_("theme"));
    auto themeNames = ThemeManager::instance().themeNames();
    QActionGroup* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);
    for (int i = 0; i < themeNames.size(); ++i) {
        QAction* act = themeMenu->addAction(themeNames[i]);
        act->setCheckable(true);
        act->setChecked(i == ThemeManager::instance().currentIndex());
        act->setData(i);
        themeGroup->addAction(act);
    }
    connect(themeGroup, &QActionGroup::triggered, this, [this](QAction* act) {
        int idx = act->data().toInt();
        ThemeManager::instance().applyTheme(idx);
    });

    QMenu* helpMenu = mb->addMenu(_("help"));
    QAction* actShortcuts = helpMenu->addAction(_("shortcuts"));
    actShortcuts->setShortcut(QKeySequence("Ctrl+K"));
    connect(actShortcuts, &QAction::triggered, this, &MainWindow::showShortcutsDialog);

    helpMenu->addSeparator();
    QAction* actPalestine = helpMenu->addAction(_("support_palestine"));
    actPalestine->setIcon(QIcon(":/icons/heart.svg"));
    connect(actPalestine, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://irusa.org/middle-east/palestine/"));
    });

    helpMenu->addSeparator();
    QAction* actAbout = helpMenu->addAction(_("about"));
    connect(actAbout, &QAction::triggered, this, [this](){ QMessageBox::about(this, _("about"), _("about_text")); });
}

void MainWindow::applyTheme() {
    ThemeManager::instance().applyTheme(ThemeManager::instance().currentIndex());
}

// ---- Data loading ----

void MainWindow::loadPlaylists() {
    auto playlists = m_manager->getAllPlaylists();
    m_sidebar->loadPlaylists(playlists);
}

void MainWindow::loadHistory() {
    auto history = m_manager->getHistory();
    m_sidebar->loadHistory(history);
}

// ---- Playlist actions ----

void MainWindow::onAddFolder() {
    QString folder = QFileDialog::getExistingDirectory(this, _("add_folder"));
    if (folder.isEmpty()) return;
    bool ok;
    QString title = QInputDialog::getText(this, _("new_playlist"), _("playlist_name"), QLineEdit::Normal, "", &ok);
    if (!ok || title.trimmed().isEmpty()) title = QDir(folder).dirName();
    title = title.trimmed();
    LOG.info("MainWindow", QString("Adding folder: %1 as '%2'").arg(folder, title));
    m_status->showMessage(_("scanning"));

    if (m_scanWorker) { m_scanWorker->cancel(); m_scanWorker->wait(3000); m_scanWorker->deleteLater(); }
    m_scanWorker = new ScanWorker(folder);
    connect(m_scanWorker, &ScanWorker::progressUpdated, this, [this](const QString& msg){ m_status->showMessage(msg); });
    connect(m_scanWorker, &ScanWorker::scanFinished, this, [this, title](const QStringList& files){ onScanDone(files, title); });
    connect(m_scanWorker, &ScanWorker::scanError, this, &MainWindow::onScanError);
    connect(m_scanWorker, &ScanWorker::finished, m_scanWorker, &QObject::deleteLater);
    m_scanWorker->start();
}

void MainWindow::onScanDone(const QStringList& files, const QString& title) {
    m_status->showMessage(_("loading"));
    m_manager->createFromFiles(files, title);
    loadPlaylists();
    m_status->showMessage(QString(_("scan_complete")).replace("{count}", QString::number(files.size())));
    m_scanWorker = nullptr;
}

void MainWindow::onScanError(const QString& err) {
    LOG.error("MainWindow", "Scan error: " + err);
    m_status->showMessage(_("error_scan"));
    QMessageBox::warning(this, _("error_scan"), err);
}

void MainWindow::onAddFiles() {
    QStringList files = QFileDialog::getOpenFileNames(this, _("add_files"), "",
        "Media Files (*.mp4 *.avi *.mkv *.mov *.wmv *.flv *.webm *.m4v *.mp3 *.wav *.flac *.ogg *.aac *.wma *.m4a);;All Files (*)");
    if (files.isEmpty()) return;
    bool ok;
    QString title = QInputDialog::getText(this, _("new_playlist"), _("playlist_name"), QLineEdit::Normal, "", &ok);
    if (!ok || title.trimmed().isEmpty()) {
        title = QFileInfo(files.first()).absoluteDir().dirName();
        if (title.isEmpty()) title = _("new_playlist");
    }
    m_manager->createFromFiles(files, title.trimmed());
    loadPlaylists();
}

void MainWindow::onDeletePlaylist(int playlistId) {
    LOG.info("MainWindow", QString("Deleting playlist id=%1").arg(playlistId));
    m_player->clearMedia();
    if (confirmDialog(_("confirm_delete_title"), _("confirm_delete_msg")))
        doDeletePlaylist(playlistId);
}

bool MainWindow::confirmDialog(const QString& title, const QString& text) {
    QMessageBox msg(this);
    msg.setWindowTitle(title);
    msg.setText(text);
    msg.setIcon(QMessageBox::Question);
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg.setDefaultButton(QMessageBox::No);
    msg.setStyleSheet(
        "QMessageBox{background-color:#1e1e2e;}"
        "QPushButton{background-color:#2a2a4e;color:#ffffff;border:none;border-radius:4px;padding:6px 20px;min-width:70px;}"
        "QPushButton:hover{background-color:#6c5ce7;}"
    );
    return msg.exec() == QMessageBox::Yes;
}

void MainWindow::doDeletePlaylist(int playlistId) {
    m_saveTimer.stop();
    LOG.info("MainWindow", QString("Executing delete for playlist id=%1").arg(playlistId));
    try {
        if (m_currentPlaylistId == playlistId) clearCurrent();
        QApplication::processEvents();
        m_manager->deletePlaylist(playlistId);
        QApplication::processEvents();
        m_sidebar->clearSelection();
        loadPlaylists();
        QApplication::processEvents();
        loadHistory();
        QApplication::processEvents();
        m_status->showMessage(_("playlist_deleted"));
        LOG.info("MainWindow", QString("Playlist %1 deleted").arg(playlistId));
    } catch (...) {
        LOG.error("MainWindow", QString("Error deleting playlist %1").arg(playlistId));
    }
    m_saveTimer.start();
}

void MainWindow::onPlaylistSelected(int playlistId) {
    LOG.info("MainWindow", QString("Selected playlist id=%1").arg(playlistId));
    m_currentPlaylistId = playlistId;
    LOG.info("MainWindow", "Step1: getVideos");
    m_currentVideos = m_manager->getVideos(playlistId);
    LOG.info("MainWindow", QString("Step2: loadVideos count=%1").arg(m_currentVideos.size()));
    m_videoList->loadVideos(m_currentVideos);
    LOG.info("MainWindow", "Step3: addHistory");
    m_manager->addHistory(playlistId);
    LOG.info("MainWindow", "Step4: loadHistory");
    loadHistory();
    LOG.info("MainWindow", "Step5: updateTimelineStats");
    updateTimelineStats();
    if (!m_currentVideos.isEmpty() && m_currentIndex < 0) m_currentIndex = 0;
    LOG.info("MainWindow", "Step6: done");

    // Show playlist info
    auto playlists = m_manager->getAllPlaylists();
    QString plTitle;
    for (const auto& p : playlists)
        if (p["id"].toInt() == playlistId) { plTitle = p["title"].toString(); break; }
    m_playlistInfo->showPlaylistInfo(plTitle, m_currentVideos);
    m_playlistInfo->show();
    m_splitter->setSizes({240, 500, 280, 220});
}

// ---- Last session restore ----

void MainWindow::restoreLastSession() {
    QVariantMap lastVideo = m_manager->getLastPlayedVideo();
    if (lastVideo.isEmpty()) return;

    int playlistId = lastVideo["playlist_id"].toInt();
    int videoId = lastVideo["video_id"].toInt();
    bool completed = lastVideo["completed"].toInt() != 0;
    double position = lastVideo["position"].toDouble();

    if (completed || position <= 0) return;

    m_sidebar->selectPlaylist(playlistId);

    for (int i = 0; i < m_currentVideos.size(); i++) {
        if (m_currentVideos[i]["id"].toInt() == videoId) {
            m_currentIndex = i;
            m_currentVideoId = videoId;
            break;
        }
    }

    QString title = lastVideo["title"].toString();
    if (title.isEmpty()) title = QFileInfo(lastVideo["file_path"].toString()).fileName();
    QString timeStr = formatDuration(position);
    m_status->showMessage(
        QString(_("last_video_resume")).replace("{title}", title).replace("{time}", timeStr)
    );
}

// ---- Playback ----

void MainWindow::playVideo(int index) {
    if (index < 0 || index >= m_currentVideos.size()) return;
    m_currentIndex = index;
    QVariantMap v = m_currentVideos[index];
    m_currentVideoId = v["id"].toInt();
    QString filePath = v["file_path"].toString();
    if (!QFileInfo::exists(filePath)) {
        QMessageBox::warning(this, _("error_no_file"), filePath + "\n" + _("video_not_found"));
        return;
    }
    LOG.info("MainWindow", QString("Playing video [%1/%2]: %3").arg(index+1).arg(m_currentVideos.size()).arg(filePath));
    if (!m_player->loadMedia(filePath)) return;

    QVariantMap progress = m_manager->db()->getProgress(v["id"].toInt());
    if (!progress.isEmpty() && progress["position"].toDouble() > 0 && !progress["completed"].toBool())
        m_player->setPosition(progress["position"].toDouble());

    m_player->play();
    m_controls->setPlayingState(true);
    m_status->showMessage(QFileInfo(filePath).fileName());
}

void MainWindow::onPlayVideoById(int videoId) {
    for (int i = 0; i < m_currentVideos.size(); i++)
        if (m_currentVideos[i]["id"].toInt() == videoId) { playVideo(i); break; }
}

void MainWindow::onPlayPause() {
    if (m_currentVideoId < 0) {
        if (!m_currentVideos.isEmpty()) playVideo(0);
        return;
    }
    if (!m_player->hasMedia()) {
        playVideo(m_currentIndex);
        return;
    }
    m_player->togglePlayPause();
    m_controls->setPlayingState(m_player->isPlaying());
}

void MainWindow::onStop() { m_player->stop(); m_controls->setPlayingState(false); m_controls->reset(); }

void MainWindow::onNext() {
    if (m_currentIndex < m_currentVideos.size() - 1) playVideo(m_currentIndex + 1);
}

void MainWindow::onPrevious() {
    if (m_currentIndex > 0) playVideo(m_currentIndex - 1);
}

void MainWindow::onSeek(double percent) { m_player->seek(percent); }

void MainWindow::onSeekRelative(double deltaSec) {
    if (!m_player->isReady()) return;
    double pos = m_player->currentPosition() + deltaSec;
    double dur = m_player->duration();
    if (dur <= 0) return;
    pos = qBound(0.0, pos, dur);
    m_player->setPosition(pos);
}

void MainWindow::onVolumeChange(int delta) {
    int vol = qBound(0, m_player->volume() + delta, 100);
    m_player->setVolume(vol);
}

void MainWindow::onVideoEnded() {
    if (m_currentVideoId >= 0) {
        m_manager->saveProgress(m_currentVideoId, 0, true);
        m_videoList->updateVideoState(m_currentVideoId, true);
        updateTimelineStats();
    }
    onNext();
}

void MainWindow::onMediaLoaded(double duration) {
    if (m_currentVideoId >= 0 && duration > 0)
        m_manager->db()->updateVideoDuration(m_currentVideoId, duration);
    updateTimelineStats();
}

void MainWindow::showShortcutsDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle(_("shortcuts_title"));
    dlg.setMinimumSize(400, 350);
    dlg.setStyleSheet("QDialog{background:#1a1a30;color:#dcdce6;}"
                      "QTextBrowser{background:#12122a;color:#dcdce6;border:1px solid #2a2a4e;padding:12px;font-size:13px;}");
    QVBoxLayout* lay = new QVBoxLayout(&dlg);
    QTextBrowser* tb = new QTextBrowser();
    tb->setText(_("shortcuts_text"));
    lay->addWidget(tb);
    QPushButton* btn = new QPushButton(_("ok"));
    btn->setStyleSheet("QPushButton{background:#6c5ce7;color:#fff;padding:6px 24px;border-radius:4px;}QPushButton:hover{background:#5a4bd1;}");
    connect(btn, &QPushButton::clicked, &dlg, &QDialog::accept);
    lay->addWidget(btn, 0, Qt::AlignCenter);
    dlg.exec();
}

void MainWindow::onFullscreen() {
    m_isFullscreen = !m_isFullscreen;
    if (m_isFullscreen) {
        m_savedSplitterSizes = m_splitter->sizes();
        showFullScreen();
        menuBar()->hide();
        m_sidebar->hide();
        m_toolbar->hide();
        m_playlistInfo->hide();
        m_status->hide();
        // Show video list on the right side in fullscreen
        m_videoList->show();
        m_videoList->setMinimumWidth(280);
        m_controls->setVisible(m_controlsVisible);
        m_splitter->setSizes({width() - 320, 320});
    } else {
        showNormal();
        menuBar()->show();
        m_sidebar->show();
        m_toolbar->show();
        m_videoList->show();
        m_playlistInfo->show();
        m_status->show();
        m_controls->setVisible(true);
        if (!m_savedSplitterSizes.isEmpty())
            m_splitter->setSizes(m_savedSplitterSizes);
        m_savedSplitterSizes.clear();
    }
}

// ---- Keyboard ----

void MainWindow::keyPressEvent(QKeyEvent* event) {
    QWidget* focus = QApplication::focusWidget();
    if (qobject_cast<QLineEdit*>(focus)) {
        QMainWindow::keyPressEvent(event); return;
    }
    if (qobject_cast<QSlider*>(focus)) {
        QMainWindow::keyPressEvent(event); return;
    }

    switch (event->key()) {
    case Qt::Key_Escape: if (m_isFullscreen) onFullscreen(); break;
    case Qt::Key_F11: onFullscreen(); break;
    case Qt::Key_Space: onPlayPause(); break;
    case Qt::Key_Right: onSeekRelative(5); break;
    case Qt::Key_Left: onSeekRelative(-5); break;
    case Qt::Key_Up: onVolumeChange(5); break;
    case Qt::Key_Down: onVolumeChange(-5); break;
    case Qt::Key_N: onNext(); break;
    case Qt::Key_P: onPrevious(); break;
    case Qt::Key_S: onStop(); break;
    case Qt::Key_F: onFullscreen(); break;
    case Qt::Key_H: toggleControls(); break;
    default: QMainWindow::keyPressEvent(event);
    }
}

// ---- Toolbar ----

void MainWindow::onSearch(const QString& text) { m_videoList->setSearch(text); }
void MainWindow::onSort(const QString& key) { m_videoList->setSort(key); }
void MainWindow::onFilter(const QString& key) { m_videoList->setFilter(key); }
void MainWindow::onViewToggle(bool grid) { m_videoList->setViewMode(grid); }

void MainWindow::toggleControls() {
    m_controlsVisible = !m_controlsVisible;
    m_controls->setVisible(m_controlsVisible);
    m_status->showMessage(m_controlsVisible ? _("controls_visible") : _("controls_hidden"));
}

void MainWindow::onToggleWatched(int videoId, bool watched) {
    m_manager->saveProgress(videoId, 0, watched);
    m_videoList->updateVideoState(videoId, watched);
    updateTimelineStats();
}

// ---- Helpers ----

void MainWindow::clearCurrent() {
    m_currentPlaylistId = -1;
    m_currentVideoId = -1;
    m_currentVideos.clear();
    m_currentIndex = -1;
    m_player->clearMedia();
    m_controls->setPlayingState(false);
    m_controls->reset();
    m_videoList->clear();
    m_timelineBar->updateStats(0,0,0,0,0);
}

void MainWindow::updateTimelineStats() {
    if (m_currentPlaylistId < 0) return;
    auto stats = m_manager->getPlaylistStats(m_currentPlaylistId);
    if (!stats.isEmpty()) {
        auto s = stats.first();
        m_timelineBar->updateStats(
            s["watched_duration"].toDouble(),
            s["remaining_duration"].toDouble(),
            s["total_duration"].toDouble(),
            s["completed_count"].toInt(),
            s["total_videos"].toInt()
        );
    }
}

void MainWindow::periodicSave() {
    if (!m_player->isReady() || !m_player->isPlaying() || m_currentVideoId < 0) return;
    double pos = m_player->currentPosition();
    double dur = m_player->duration();
    if (dur > 0 && pos > 0)
        m_manager->saveProgress(m_currentVideoId, pos, false);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    LOG.info("MainWindow", "Shutting down TimelineVideo");
    m_saveTimer.stop();
    if (m_scanWorker && m_scanWorker->isRunning()) {
        m_scanWorker->cancel();
        m_scanWorker->wait(2000);
    }
    if (m_player->isReady() && m_currentVideoId >= 0) {
        double pos = m_player->currentPosition();
        double dur = m_player->duration();
        if (dur > 0 && pos > 0) {
            bool completed = std::abs(pos - dur) < 2.0;
            m_manager->saveProgress(m_currentVideoId, pos, completed);
        }
    }
    LOG.info("MainWindow", "Application closed");
    event->accept();
}
