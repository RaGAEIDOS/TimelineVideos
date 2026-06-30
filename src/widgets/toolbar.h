#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

class VideoToolbar : public QWidget {
    Q_OBJECT
public:
    explicit VideoToolbar(QWidget* parent = nullptr);
    void retranslate();

signals:
    void searchChanged(const QString& text);
    void sortChanged(const QString& key);
    void filterChanged(const QString& key);
    void viewToggled(bool gridMode);

private:
    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_sortCombo = nullptr;
    QComboBox* m_filterCombo = nullptr;
    QPushButton* m_viewToggle = nullptr;
    bool m_gridMode = true;
};
