#include "controls.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStyle>
#include <QIcon>
#include <QSize>
#include "i18n.h"

PlaybackControls::PlaybackControls(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void PlaybackControls::setupUI() {
    QHBoxLayout* lay = new QHBoxLayout(this);
    lay->setContentsMargins(8,4,8,4);
    lay->setSpacing(4);

    btnPrev = new QPushButton();
    btnPrev->setIcon(QIcon(":/icons/prev.svg"));
    btnPrev->setIconSize(QSize(18, 18));
    btnPrev->setFixedSize(32,32);
    btnPrev->setToolTip(_("previous"));
    btnPrev->setStyleSheet("QPushButton{background:#2a2a4e;color:#fff;border:none;border-radius:16px;}QPushButton:hover{background:#6c5ce7;}");
    connect(btnPrev, &QPushButton::clicked, this, &PlaybackControls::previousClicked);

    btnPlay = new QPushButton();
    btnPlay->setIcon(QIcon(":/icons/play.svg"));
    btnPlay->setIconSize(QSize(22, 22));
    btnPlay->setFixedSize(40,40);
    btnPlay->setToolTip(_("play"));
    btnPlay->setStyleSheet("QPushButton{background:#6c5ce7;color:#fff;border:none;border-radius:20px;}QPushButton:hover{background:#7c6cf7;}");
    connect(btnPlay, &QPushButton::clicked, this, &PlaybackControls::playPauseClicked);

    btnNext = new QPushButton();
    btnNext->setIcon(QIcon(":/icons/next.svg"));
    btnNext->setIconSize(QSize(18, 18));
    btnNext->setFixedSize(32,32);
    btnNext->setToolTip(_("next"));
    btnNext->setStyleSheet("QPushButton{background:#2a2a4e;color:#fff;border:none;border-radius:16px;}QPushButton:hover{background:#6c5ce7;}");
    connect(btnNext, &QPushButton::clicked, this, &PlaybackControls::nextClicked);

    btnStop = new QPushButton();
    btnStop->setIcon(QIcon(":/icons/stop.svg"));
    btnStop->setIconSize(QSize(16, 16));
    btnStop->setFixedSize(28,28);
    btnStop->setToolTip(_("stop"));
    btnStop->setStyleSheet("QPushButton{background:#2a2a4e;color:#fff;border:none;border-radius:14px;}QPushButton:hover{background:#6c5ce7;}");
    connect(btnStop, &QPushButton::clicked, this, &PlaybackControls::stopClicked);

    btnFullscreen = new QPushButton();
    btnFullscreen->setIcon(QIcon(":/icons/fullscreen.svg"));
    btnFullscreen->setIconSize(QSize(16, 16));
    btnFullscreen->setFixedSize(28,28);
    btnFullscreen->setToolTip(_("fullscreen"));
    btnFullscreen->setStyleSheet("QPushButton{background:#2a2a4e;color:#fff;border:none;border-radius:14px;}QPushButton:hover{background:#6c5ce7;}");
    connect(btnFullscreen, &QPushButton::clicked, this, &PlaybackControls::fullscreenClicked);

    seekSlider = new QSlider(Qt::Horizontal);
    seekSlider->setRange(0, 1000);
    seekSlider->setStyleSheet(
        "QSlider::groove:horizontal{height:4px;background:#2a2a4e;border-radius:2px;}"
        "QSlider::handle:horizontal{background:#6c5ce7;width:12px;height:12px;margin:-4px 0;border-radius:6px;}"
        "QSlider::sub-page:horizontal{background:#6c5ce7;border-radius:2px;}"
    );
    connect(seekSlider, &QSlider::sliderMoved, this, [this](int val) {
        emit seekChanged(val / 10.0);
    });
    connect(seekSlider, &QSlider::sliderPressed, this, [this]() { m_dragging = true; });
    connect(seekSlider, &QSlider::sliderReleased, this, [this]() { m_dragging = false; });

    timeLabel = new QLabel("0:00 / 0:00");
    timeLabel->setStyleSheet("color:#808090;font-size:11px;padding:0 6px;");

    volumeSlider = new QSlider(Qt::Horizontal);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(100);
    volumeSlider->setFixedWidth(80);
    volumeSlider->setToolTip(_("volume"));
    volumeSlider->setStyleSheet(
        "QSlider::groove:horizontal{height:3px;background:#2a2a4e;border-radius:2px;}"
        "QSlider::handle:horizontal{background:#6c5ce7;width:10px;height:10px;margin:-3px 0;border-radius:5px;}"
        "QSlider::sub-page:horizontal{background:#6c5ce7;border-radius:2px;}"
    );
    connect(volumeSlider, &QSlider::valueChanged, this, &PlaybackControls::volumeChanged);

    speedCombo = new QComboBox();
    speedCombo->setFocusPolicy(Qt::NoFocus);
    speedCombo->addItems({"0.25x","0.5x","0.75x","1x","1.25x","1.5x","2x"});
    speedCombo->setCurrentText("1x");
    speedCombo->setFixedWidth(60);
    speedCombo->setStyleSheet(
        "QComboBox{background:#2a2a4e;color:#fff;border:none;border-radius:4px;padding:4px;font-size:11px;}"
        "QComboBox:hover{background:#3a3a5e;}"
        "QComboBox::drop-down{border:none;width:16px;}"
        "QComboBox QAbstractItemView{background:#1a1a30;color:#fff;selection-background-color:#6c5ce7;border:none;}"
    );
    connect(speedCombo, &QComboBox::currentTextChanged, this, [this](const QString& t) {
        QString v = t; v.chop(1);
        emit speedChanged(v.toDouble());
    });

    lay->addWidget(btnPrev);
    lay->addWidget(btnPlay);
    lay->addWidget(btnNext);
    lay->addWidget(btnStop);
    lay->addWidget(seekSlider, 1);
    lay->addWidget(timeLabel);
    lay->addWidget(volumeSlider);
    lay->addWidget(speedCombo);
    lay->addWidget(btnFullscreen);
}

void PlaybackControls::setPlayingState(bool playing) {
    m_isPlaying = playing;
    btnPlay->setIcon(QIcon(QString(":/icons/%1.svg").arg(playing ? "pause" : "play")));
    btnPlay->setToolTip(playing ? _("pause") : _("play"));
}

void PlaybackControls::updateTime(double pos, double dur) {
    auto fmt = [](double s) -> QString {
        if (s <= 0) return "0:00";
        int t = (int)s;
        int h = t / 3600, m = (t % 3600) / 60, sc = t % 60;
        if (h > 0) return QString("%1:%2:%3").arg(h).arg(m,2,10,QChar('0')).arg(sc,2,10,QChar('0'));
        return QString("%1:%2").arg(m).arg(sc,2,10,QChar('0'));
    };
    timeLabel->setText(fmt(pos) + " / " + fmt(dur));
    if (dur > 0 && !m_dragging) {
        seekSlider->blockSignals(true);
        seekSlider->setValue((int)(pos / dur * 1000));
        seekSlider->blockSignals(false);
    }
}

void PlaybackControls::retranslate() {
    btnPrev->setToolTip(_("previous"));
    btnPlay->setToolTip(m_isPlaying ? _("pause") : _("play"));
    btnNext->setToolTip(_("next"));
    btnStop->setToolTip(_("stop"));
    btnFullscreen->setToolTip(_("fullscreen"));
    volumeSlider->setToolTip(_("volume"));
}

void PlaybackControls::reset() {
    seekSlider->setValue(0);
    timeLabel->setText("0:00 / 0:00");
    btnPlay->setIcon(QIcon(":/icons/play.svg"));
    btnPlay->setToolTip(_("play"));
}
