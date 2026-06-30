#pragma once
#include <QString>
#include <QColor>
#include <QVector>

struct Theme {
    QString name;
    QColor bgPrimary;
    QColor bgSecondary;
    QColor bgTertiary;
    QColor bgWidget;
    QColor textPrimary;
    QColor textSecondary;
    QColor textMuted;
    QColor accent;
    QColor accentHover;
    QColor border;
    QColor danger;
    QColor success;
    QColor scrollbarHover;
    QColor tabSelected;
    QColor overlay;

    QString stylesheet(const QString& accentHex = QString()) const;
    static Theme darkDefault();
    static Theme dracula();
};

class ThemeManager {
public:
    static ThemeManager& instance();
    void applyTheme(const Theme& theme);
    void applyTheme(int index);
    const Theme& currentTheme() const { return m_themes[m_currentIndex]; }
    int currentIndex() const { return m_currentIndex; }
    int themeCount() const { return m_themes.size(); }
    const Theme& theme(int index) const { return m_themes[index]; }
    QStringList themeNames() const;

private:
    ThemeManager();
    QVector<Theme> m_themes;
    int m_currentIndex = 0;
};
