#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QVector>
#include <QVariantMap>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include "utils/workers.h"

class VideoCard : public QFrame {
    Q_OBJECT
public:
    explicit VideoCard(const QVariantMap& data, QWidget* parent = nullptr);
    int videoId() const { return m_videoId; }
    bool isCompleted() const { return m_completed; }
    QString filePath() const { return m_filePath; }
    void updateState(bool completed);
    void setThumbnail(const QPixmap& thumb);

signals:
    void playRequested(int videoId);
    void toggleWatched(int videoId, bool watched);

protected:
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;

private:
    int m_videoId;
    QString m_filePath;
    double m_duration;
    bool m_completed;
    QString m_title;
    QLabel* m_thumbLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
};

class VideoItemWidget : public QFrame {
    Q_OBJECT
public:
    explicit VideoItemWidget(const QVariantMap& data, int index, QWidget* parent = nullptr);
    int videoId() const { return m_videoId; }
    bool isCompleted() const { return m_completed; }
    void markCompleted(bool completed);

signals:
    void playRequested(int videoId);
    void toggleWatched(int videoId, bool watched);

protected:
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;

private:
    int m_videoId, m_index;
    double m_duration;
    bool m_completed;
    QString m_title;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
};

class VideoListView : public QWidget {
    Q_OBJECT
public:
    explicit VideoListView(QWidget* parent = nullptr);
    void loadVideos(const QVector<QVariantMap>& videos);
    void setViewMode(bool grid);
    void setSearch(const QString& text);
    void setSort(const QString& key);
    void setFilter(const QString& key);
    void updateVideoState(int videoId, bool completed);
    void clear();
    void retranslate();

signals:
    void videoPlayRequested(int videoId);
    void videoToggleWatched(int videoId, bool watched);

protected:
    void resizeEvent(QResizeEvent* e) override;

private:
    void setupList();
    void setupGrid();
    void clearStack();
    void applyFilters();
    void redisplay();
    void redisplayList();
    void redisplayGrid();
    QString videoTitle(const QVariantMap& v);

    void scheduleThumbnails();
    void processNextThumb();

    bool m_gridMode = false;
    QString m_sortKey = "0-9", m_filterKey = "all", m_searchText;
    QVector<QVariantMap> m_allVideos, m_filteredVideos;
    QVBoxLayout* m_stack = nullptr;
    QListWidget* m_listWidget = nullptr;
    QGridLayout* m_gridLayout = nullptr;
    QVector<VideoCard*> m_gridCards;
    QVector<VideoItemWidget*> m_listWidgets;
    QLabel* m_headerLabel = nullptr;
    QLabel* m_hintLabel = nullptr;
    QVector<ThumbnailWorker*> m_thumbWorkers;
    QVector<VideoCard*> m_thumbQueue;
};
