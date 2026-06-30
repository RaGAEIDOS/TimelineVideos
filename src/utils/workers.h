#pragma once
#include <QThread>
#include <QStringList>
#include <QMutex>
#include <QSize>

class ScanWorker : public QThread {
    Q_OBJECT
public:
    explicit ScanWorker(const QString& folderPath, bool recursive = true);
    void cancel();
signals:
    void progressUpdated(const QString& msg);
    void scanFinished(const QStringList& files);
    void scanError(const QString& err);
protected:
    void run() override;
private:
    QString m_folderPath;
    bool m_recursive;
    volatile bool m_cancelled = false;
};

class ThumbnailWorker : public QThread {
    Q_OBJECT
public:
    ThumbnailWorker(const QString& filePath, const QString& thumbPath, QSize size = QSize(160, 90));
signals:
    void thumbReady(const QString& filePath);
protected:
    void run() override;
private:
    QString m_filePath, m_thumbPath;
    QSize m_size;
    QPixmap fallback();
};
