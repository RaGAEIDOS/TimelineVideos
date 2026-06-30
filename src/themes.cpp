#include "themes.h"
#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStyleFactory>

Theme Theme::darkDefault() {
    Theme t;
    t.name           = "Default Dark";
    t.bgPrimary      = QColor(18, 18, 42);
    t.bgSecondary    = QColor(22, 22, 42);
    t.bgTertiary     = QColor(26, 26, 46);
    t.bgWidget       = QColor(30, 30, 50);
    t.textPrimary    = QColor(220, 220, 230);
    t.textSecondary  = QColor(160, 160, 176);
    t.textMuted      = QColor(128, 128, 144);
    t.accent         = QColor(108, 92, 231);
    t.accentHover    = QColor(124, 108, 247);
    t.border         = QColor(42, 42, 78);
    t.danger         = QColor(255, 107, 107);
    t.success        = QColor(80, 250, 123);
    t.scrollbarHover = QColor(108, 92, 231);
    t.tabSelected    = QColor(108, 92, 231);
    t.overlay        = QColor(30, 30, 50);
    return t;
}

Theme Theme::dracula() {
    Theme t;
    t.name           = "Dracula";
    t.bgPrimary      = QColor(40, 42, 54);
    t.bgSecondary    = QColor(68, 71, 90);
    t.bgTertiary     = QColor(48, 50, 65);
    t.bgWidget       = QColor(55, 58, 75);
    t.textPrimary    = QColor(248, 248, 242);
    t.textSecondary  = QColor(200, 200, 190);
    t.textMuted      = QColor(98, 114, 164);
    t.accent         = QColor(189, 147, 249);
    t.accentHover    = QColor(204, 168, 252);
    t.border         = QColor(68, 71, 90);
    t.danger         = QColor(255, 85, 85);
    t.success        = QColor(80, 250, 123);
    t.scrollbarHover = QColor(189, 147, 249);
    t.tabSelected    = QColor(189, 147, 249);
    t.overlay        = QColor(68, 71, 90);
    return t;
}

QString Theme::stylesheet(const QString& accentOverride) const {
    QString a = accentOverride.isEmpty() ? accent.name(QColor::HexRgb) : accentOverride;
    QString ah = accentHover.name(QColor::HexRgb);
    QString bp = bgPrimary.name(QColor::HexRgb);
    QString bs = bgSecondary.name(QColor::HexRgb);
    QString bt = bgTertiary.name(QColor::HexRgb);
    QString bw = bgWidget.name(QColor::HexRgb);
    QString tp = textPrimary.name(QColor::HexRgb);
    QString ts = textSecondary.name(QColor::HexRgb);
    QString tm = textMuted.name(QColor::HexRgb);
    QString b  = border.name(QColor::HexRgb);
    QString d  = danger.name(QColor::HexRgb);

    return QString(
        "QMainWindow{background-color:%1;}"
        "QSplitter::handle{background-color:%5;width:1px;}"
        "QWidget{color:%2;}"
        "QToolTip{background-color:%4;color:%2;border:1px solid %5;padding:4px 8px;border-radius:4px;}"
        "QScrollBar:vertical{background:%1;width:8px;border-radius:4px;}"
        "QScrollBar::handle:vertical{background:%5;border-radius:4px;min-height:30px;}"
        "QScrollBar::handle:vertical:hover{background:%7;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
        "QScrollBar:horizontal{background:%1;height:8px;border-radius:4px;}"
        "QScrollBar::handle:horizontal{background:%5;border-radius:4px;min-width:30px;}"
        "QScrollBar::handle:horizontal:hover{background:%7;}"
        "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;}"
        "QProgressBar{background:#444;border:none;border-radius:3px;height:6px;text-align:center;}"
        "QProgressBar::chunk{background:%7;border-radius:3px;}"
        "QComboBox{background:%3;color:%2;border:1px solid %5;border-radius:6px;padding:6px 10px;font-size:11px;}"
        "QComboBox:hover{border-color:%7;}"
        "QComboBox::drop-down{border:none;padding-right:8px;}"
        "QComboBox QAbstractItemView{background:%3;color:%2;selection-background-color:%7;border:1px solid %5;}"
        "QLineEdit{background:%3;color:%2;border:1px solid %5;border-radius:6px;padding:6px 10px;font-size:12px;}"
        "QLineEdit:focus{border-color:%7;}"
        "QPushButton{background:%5;color:%2;border:none;border-radius:6px;padding:6px 14px;font-size:11px;}"
        "QPushButton:hover{background:%6;}"
        "QPushButton:pressed{background:%7;}"
        "QListWidget{background:transparent;border:none;outline:none;}"
        "QListWidget::item{color:%8;padding:8px;border-bottom:1px solid %5;}"
        "QListWidget::item:hover{background:%5;color:%2;}"
        "QListWidget::item:selected{background:%7;color:%2;}"
        "QTabWidget::pane{background:%3;border:none;}"
        "QTabBar::tab{background:%4;color:%8;padding:6px 12px;font-size:11px;border:none;min-width:60px;}"
        "QTabBar::tab:selected{background:%3;color:%2;border-bottom:2px solid %7;}"
        "QTabBar::tab:hover{color:%2;}"
        "QMenuBar{background:%1;color:%2;border-bottom:1px solid %5;}"
        "QMenuBar::item:selected{background:%5;}"
        "QMenu{background:%4;color:%2;border:1px solid %5;}"
        "QMenu::item:selected{background:%7;}"
        "QStatusBar{background:%1;color:%8;border-top:1px solid %5;font-size:11px;}"
        "QSlider::groove:horizontal{height:4px;background:#444;border-radius:2px;}"
        "QSlider::handle:horizontal{background:%7;width:14px;height:14px;margin:-5px 0;border-radius:7px;}"
        "QSlider::sub-page:horizontal{background:%7;border-radius:2px;}"
        "QMessageBox{background:%3;}"
        "QDialog{background:%4;color:%2;}"
    ).arg(bp, tp, bs, bt, b, bw, a, tm, d);
}

ThemeManager::ThemeManager() {
    m_themes.append(Theme::darkDefault());
    m_themes.append(Theme::dracula());

    // Restore saved theme
    QSettings settings("TimelineVideo", "TimelineVideo");
    m_currentIndex = settings.value("theme", 0).toInt();
    if (m_currentIndex < 0 || m_currentIndex >= m_themes.size())
        m_currentIndex = 0;
    applyTheme(m_currentIndex);
}

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

void ThemeManager::applyTheme(const Theme& theme) {
    // Find index
    for (int i = 0; i < m_themes.size(); ++i) {
        if (m_themes[i].name == theme.name) {
            m_currentIndex = i;
            break;
        }
    }
    applyTheme(m_currentIndex);
}

void ThemeManager::applyTheme(int index) {
    if (index < 0 || index >= m_themes.size()) return;
    m_currentIndex = index;
    const Theme& t = m_themes[index];

    // Save preference
    QSettings settings("TimelineVideo", "TimelineVideo");
    settings.setValue("theme", index);

    // Stylesheet
    QString ss = t.stylesheet();
    qApp->setStyleSheet(ss);

    // Palette
    QPalette p;
    p.setColor(QPalette::Window, t.bgPrimary);
    p.setColor(QPalette::WindowText, t.textPrimary);
    p.setColor(QPalette::Base, t.bgSecondary);
    p.setColor(QPalette::AlternateBase, t.bgWidget);
    p.setColor(QPalette::ToolTipBase, t.overlay);
    p.setColor(QPalette::ToolTipText, t.textPrimary);
    p.setColor(QPalette::Text, t.textPrimary);
    p.setColor(QPalette::Button, t.bgTertiary);
    p.setColor(QPalette::ButtonText, t.textPrimary);
    p.setColor(QPalette::BrightText, t.danger);
    p.setColor(QPalette::Highlight, t.accent);
    p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    qApp->setPalette(p);
}

QStringList ThemeManager::themeNames() const {
    QStringList names;
    for (const auto& t : m_themes)
        names << t.name;
    return names;
}
