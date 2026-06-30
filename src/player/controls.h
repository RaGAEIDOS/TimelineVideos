#pragma once
#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>

class PlaybackControls : public QWidget {
    Q_OBJECT
public:
    explicit PlaybackControls(QWidget* parent = nullptr);
    void setPlayingState(bool playing);
    void updateTime(double pos, double dur);
    void reset();
    void retranslate();

signals:
    void playPauseClicked();
    void stopClicked();
    void nextClicked();
    void previousClicked();
    void volumeChanged(int vol);
    void speedChanged(double speed);
    void seekChanged(double percent);
    void fullscreenClicked();

private:
    void setupUI();
    QPushButton* btnPlay = nullptr;
    QPushButton* btnStop = nullptr;
    QPushButton* btnPrev = nullptr;
    QPushButton* btnNext = nullptr;
    QPushButton* btnFullscreen = nullptr;
    QSlider* seekSlider = nullptr;
    QSlider* volumeSlider = nullptr;
    QComboBox* speedCombo = nullptr;
    QLabel* timeLabel = nullptr;
    bool m_dragging = false;
    bool m_isPlaying = false;
};
