#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QTabWidget>
#include <QListWidget>
#include <QFrame>
#include <QLabel>
#include <QPushButton>

class PlaylistCard : public QFrame {
    Q_OBJECT
public:
    explicit PlaylistCard(const QVariantMap& data, QWidget* parent = nullptr);
    int playlistId() const { return m_playlistId; }
signals:
    void clicked(int playlistId);
protected:
    void mousePressEvent(QMouseEvent* e) override;
private:
    int m_playlistId;
    QLabel* m_thumbLabel = nullptr;
};

class PlaylistSidebar : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistSidebar(QWidget* parent = nullptr);
    void loadPlaylists(const QVector<QVariantMap>& playlists);
    void loadHistory(const QVector<QVariantMap>& history);
    void clearSelection();
    void selectPlaylist(int playlistId);
    void retranslate();
    int selectedPlaylistId() const { return m_selectedId; }

signals:
    void playlistSelected(int playlistId);
    void addFolderRequested();
    void addFilesRequested();
    void deleteRequested(int playlistId);

private:
    void setupUI();
    void onCardClicked(int pid);
    void onHistoryClicked(QListWidgetItem* item);
    void deleteSelected();

    int m_selectedId = -1;
    QVBoxLayout* m_playlistLayout = nullptr;
    QWidget* m_playlistContainer = nullptr;
    QListWidget* m_historyList = nullptr;
    QTabWidget* m_tabs = nullptr;
    QVector<PlaylistCard*> m_cards;
    QLabel* m_headerLabel = nullptr;
    QPushButton* btnAddFolder = nullptr;
    QPushButton* btnAddFiles = nullptr;
    QPushButton* btnDelete = nullptr;
};
