#define LOG Logger::instance()

#include "vlcplayer.h"
#include "utils/logger.h"
#include <QVBoxLayout>
#include <QFileInfo>
#include <QUrl>
#include <QMouseEvent>
#include <QEvent>

VLCPlayer::VLCPlayer(QWidget* parent) : QFrame(parent) {
    setStyleSheet("background-color: #000000; border-radius: 8px;");
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0,0,0,0);

    m_videoFrame = new QFrame();
    m_videoFrame->setStyleSheet("background-color: #000000;");
    m_videoFrame->setAutoFillBackground(true);
    m_videoFrame->installEventFilter(this);
    lay->addWidget(m_videoFrame);

    qputenv("VLC_PLUGIN_PATH", "C:/Program Files/VideoLAN/VLC/plugins");

    const char* args[] = {"--quiet", "--no-video-title-show",
                          "--plugin-path", "C:/Program Files/VideoLAN/VLC/plugins", nullptr};
    m_instance = libvlc_new(4, args);
    if (!m_instance)
        m_instance = libvlc_new(0, nullptr);
    if (m_instance) {
        m_player = libvlc_media_player_new(m_instance);
        if (m_player) {
            m_vlcOk = true;
            libvlc_media_player_set_hwnd(m_player, (void*)(intptr_t)m_videoFrame->winId());
        }
    }
    connect(&m_timer, &QTimer::timeout, this, &VLCPlayer::updatePosition);
    m_timer.setInterval(500);
}

void VLCPlayer::showEvent(QShowEvent* e) {
    QFrame::showEvent(e);
    if (m_player && m_videoFrame)
        libvlc_media_player_set_hwnd(m_player, (void*)(intptr_t)m_videoFrame->winId());
}

bool VLCPlayer::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_videoFrame && event->type() == QEvent::MouseButtonDblClick) {
        emit fullscreenRequested();
        return true;
    }
    return QFrame::eventFilter(obj, event);
}

VLCPlayer::~VLCPlayer() {
    m_timer.stop();
    if (m_player) { libvlc_media_player_stop(m_player); libvlc_media_player_release(m_player); }
    if (m_instance) libvlc_release(m_instance);
}

bool VLCPlayer::loadMedia(const QString& filePath) {
    if (!QFileInfo::exists(filePath) || !m_vlcOk || !m_player || !m_instance) {
        LOG.warn("VLCPlayer", "loadMedia failed");
        return false;
    }
    clearMedia();
    m_currentPath = filePath;

    QUrl url = QUrl::fromLocalFile(filePath);
    QByteArray uri = url.toEncoded();
    libvlc_media_t* media = libvlc_media_new_location(m_instance, uri.constData());
    if (!media) {
        LOG.warn("VLCPlayer", "libvlc_media_new_location failed for: " + filePath);
        return false;
    }
    libvlc_media_player_set_media(m_player, media);
    libvlc_media_parse_with_options(media, libvlc_media_parse_local, 3000);
    double dur = libvlc_media_get_duration(media) / 1000.0;
    if (dur < 0) dur = 0;
    libvlc_media_release(media);

    libvlc_media_player_set_hwnd(m_player, (void*)(intptr_t)m_videoFrame->winId());

    LOG.info("VLCPlayer", "Media loaded: " + filePath + " duration=" + QString::number(dur));
    emit mediaLoaded(dur);
    return true;
}

void VLCPlayer::play() {
    if (!m_vlcOk || !m_player) return;
    libvlc_media_player_set_hwnd(m_player, (void*)(intptr_t)m_videoFrame->winId());
    libvlc_media_player_play(m_player);
    m_isPlaying = true;
    m_timer.start();
    LOG.info("VLCPlayer", "Play started");
}

void VLCPlayer::pause() {
    if (!m_vlcOk || !m_player) return;
    libvlc_media_player_set_pause(m_player, 1);
    m_isPlaying = false;
    m_timer.stop();
}

void VLCPlayer::stop() {
    if (!m_vlcOk || !m_player) return;
    m_timer.stop();
    libvlc_media_player_stop(m_player);
    m_isPlaying = false;
    m_currentPath.clear();
}

void VLCPlayer::clearMedia() {
    if (!m_vlcOk || !m_player) return;
    m_timer.stop();
    m_isPlaying = false;
    m_currentPath.clear();
    libvlc_media_player_set_media(m_player, nullptr);
}

void VLCPlayer::togglePlayPause() {
    if (m_isPlaying) pause(); else play();
}

void VLCPlayer::seek(double percent) {
    if (!m_player) return;
    libvlc_media_player_set_position(m_player, (float)(percent / 100.0));
}

void VLCPlayer::setPosition(double seconds) {
    if (!m_player) return;
    libvlc_time_t len = libvlc_media_player_get_length(m_player);
    if (len > 0) libvlc_media_player_set_position(m_player, (float)(seconds * 1000.0 / len));
}

void VLCPlayer::setVolume(int vol) {
    m_volume = qBound(0, vol, 100);
    if (m_player) libvlc_audio_set_volume(m_player, m_volume);
}

void VLCPlayer::setSpeed(double speed) {
    m_speed = speed;
    if (m_player) libvlc_media_player_set_rate(m_player, (float)speed);
}

double VLCPlayer::currentPosition() const {
    if (!m_player) return 0;
    float pos = libvlc_media_player_get_position(m_player);
    libvlc_time_t len = libvlc_media_player_get_length(m_player);
    if (len > 0 && pos >= 0) return (pos * len) / 1000.0;
    return 0;
}

double VLCPlayer::duration() const {
    if (!m_player) return 0;
    libvlc_time_t len = libvlc_media_player_get_length(m_player);
    return len > 0 ? len / 1000.0 : 0;
}

void VLCPlayer::updatePosition() {
    if (!m_vlcOk || !m_player) return;
    libvlc_state_t state = libvlc_media_player_get_state(m_player);
    if (state == libvlc_Ended) {
        m_timer.stop();
        m_isPlaying = false;
        emit endReached();
        return;
    }
    double pos = currentPosition();
    double dur = duration();
    if (dur > 0) emit positionChanged(pos, dur);
}
