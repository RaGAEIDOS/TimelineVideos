#include "timelinebar.h"
#include "utils/timeutils.h"
#include "i18n.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

TimelineBar::TimelineBar(QWidget* parent) : QFrame(parent) {
    setFixedHeight(36);
    setStyleSheet("background:#16162a;border-top:1px solid #2a2a4e;");
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8,2,8,2);
    m_label = new QLabel();
    m_label->setStyleSheet("color:#808090;font-size:10px;");
    lay->addWidget(m_label);
}

void TimelineBar::updateStats(double w, double r, double t, int cc, int tc) {
    m_watchedDur = w; m_remainingDur = r; m_totalDur = t;
    m_completedCount = cc; m_totalCount = tc;
    m_label->setText(
        QString("%1: %2  |  %3: %4  |  %5: %6  (%7/%8)")
        .arg(_("watched"), formatDurationHuman(w))
        .arg(_("remaining"), formatDurationHuman(r))
        .arg(_("total_time"), formatDurationHuman(t))
        .arg(cc).arg(tc)
    );
    update();
}

void TimelineBar::retranslate() {
    updateStats(m_watchedDur, m_remainingDur, m_totalDur, m_completedCount, m_totalCount);
}

void TimelineBar::paintEvent(QPaintEvent*) {
    if (m_totalDur <= 0) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int barH = 4;
    int y = height() - barH - 4;
    int margin = 8;
    int w = width() - margin * 2;
    double pct = (m_watchedDur / m_totalDur) * 100.0;
    if (pct > 100) pct = 100;
    // Background
    p.setBrush(QColor("#2a2a4e"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(margin, y, w, barH, 2, 2);
    // Watched fill
    p.setBrush(QColor("#6c5ce7"));
    p.drawRoundedRect(margin, y, (int)(w * pct / 100.0), barH, 2, 2);
}
