#pragma once
#include <QFrame>
#include <QTimer>
#include <QShowEvent>
#include <QUrl>
#include <vlc/vlc.h>

class VLCPlayer : public QFrame {
    Q_OBJECT
public:
    explicit VLCPlayer(QWidget* parent = nullptr);
    ~VLCPlayer();

    bool loadMedia(const QString& filePath);
    void play();
    void pause();
    void stop();
    void clearMedia();
    void togglePlayPause();
    void seek(double percent);
    void setPosition(double seconds);
    void setVolume(int vol);
    int volume() const { return m_volume; }
    void setSpeed(double speed);
    double speed() const { return m_speed; }
    bool isPlaying() const { return m_isPlaying; }
    bool isReady() const { return m_vlcOk; }
    bool hasMedia() const { return !m_currentPath.isEmpty(); }
    double currentPosition() const;
    double duration() const;

protected:
    void showEvent(QShowEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;

signals:
    void positionChanged(double pos, double dur);
    void endReached();
    void mediaLoaded(double duration);
    void fullscreenRequested();

private slots:
    void updatePosition();

private:
    libvlc_instance_t* m_instance = nullptr;
    libvlc_media_player_t* m_player = nullptr;
    QTimer m_timer;
    QFrame* m_videoFrame = nullptr;
    QString m_currentPath;
    int m_volume = 100;
    double m_speed = 1.0;
    bool m_isPlaying = false;
    bool m_vlcOk = false;
};
