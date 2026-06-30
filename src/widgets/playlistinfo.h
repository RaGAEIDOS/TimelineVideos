#pragma once
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QVariantMap>
#include <QVector>

class PlaylistInfo : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistInfo(QWidget* parent = nullptr);
    void showPlaylistInfo(const QString& title, const QVector<QVariantMap>& videos);
    void clear();
    void retranslate();
private:
    QLabel* m_titleLabel = nullptr;
    QLabel* m_countLabel = nullptr;
    QLabel* m_watchedLabel = nullptr;
    QLabel* m_durationLabel = nullptr;
    QLabel* m_headerLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
};
