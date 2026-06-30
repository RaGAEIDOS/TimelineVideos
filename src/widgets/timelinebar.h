#pragma once
#include <QFrame>
#include <QLabel>
#include <QPainter>

class TimelineBar : public QFrame {
    Q_OBJECT
public:
    explicit TimelineBar(QWidget* parent = nullptr);
    void updateStats(double watchedDur, double remainingDur, double totalDur,
                     int completedCount, int totalCount);
    void retranslate();
protected:
    void paintEvent(QPaintEvent* e) override;

private:
    double m_watchedDur = 0, m_remainingDur = 0, m_totalDur = 0;
    int m_completedCount = 0, m_totalCount = 0;
    QLabel* m_label = nullptr;
};
