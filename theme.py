"""
theme.py
---------
A modern dark QSS stylesheet applied at the QApplication level.

Design language:
  • deep navy-black surfaces with layered "cards" (#0f1220 → #1a1e33)
  • one blue accent (#4da3ff) + a green "watched" accent (#2fd47a)
  • 10–12 px rounded corners everywhere, subtle 1px borders
  • smooth hover/pressed states, YouTube-style progress bar

Standard Qt widgets only — no custom-painted skins — so Arabic
rendering keeps using Qt's proven native text path.
"""

MODERN_STYLESHEET = """
/* ---------- base ---------- */
QWidget {
    background-color: #0f1220;
    color: #e8eaf2;
    font-size: 13px;
}

QMainWindow, QDialog {
    background-color: #0f1220;
}

QLabel {
    background: transparent;
}

/* ---------- playlist ---------- */
QListWidget {
    background-color: #14172a;
    border: 1px solid #232741;
    border-radius: 12px;
    padding: 6px;
    outline: none;
    selection-background-color: #2b3358;
}

QListWidget::item {
    border-radius: 8px;
    padding: 6px 8px;
    margin: 2px 0px;
    border: 1px solid transparent;
}

QListWidget::item:selected {
    background-color: #2b3358;
    color: #ffffff;
    border: 1px solid #3d477a;
}

QListWidget::item:hover:!selected {
    background-color: #1c2138;
}

/* ---------- buttons ---------- */
QPushButton {
    background-color: #1a1e33;
    border: 1px solid #2b3050;
    border-radius: 9px;
    padding: 7px 16px;
    color: #e8eaf2;
    min-height: 20px;
}

QPushButton:hover {
    background-color: #232846;
    border: 1px solid #4da3ff;
}

QPushButton:pressed {
    background-color: #12152a;
}

QPushButton:disabled {
    color: #565b78;
    border: 1px solid #232741;
    background-color: #14172a;
}

/* primary / accent button (set via setProperty("accent", True)) */
QPushButton[accent="true"] {
    background-color: #2563c9;
    border: 1px solid #3b7de0;
    color: #ffffff;
    font-weight: bold;
}

QPushButton[accent="true"]:hover {
    background-color: #2f74e0;
    border: 1px solid #6cb0ff;
}

QPushButton[accent="true"]:pressed {
    background-color: #1c4fa3;
}

/* ---------- cards (stat boxes, dialogs bodies) ---------- */
QFrame[card="true"] {
    background-color: #171a2b;
    border: 1px solid #232741;
    border-radius: 12px;
}

/* ---------- menu bar ---------- */
QMenuBar {
    background-color: #0f1220;
    color: #c8cce0;
    border-bottom: 1px solid #1c2038;
    padding: 3px;
}

QMenuBar::item {
    background: transparent;
    padding: 5px 12px;
    border-radius: 7px;
}

QMenuBar::item:selected {
    background-color: #1a1e33;
    color: #ffffff;
}

QMenu {
    background-color: #171a2b;
    border: 1px solid #2b3050;
    border-radius: 10px;
    color: #e8eaf2;
    padding: 6px;
}

QMenu::item {
    padding: 7px 24px;
    border-radius: 7px;
}

QMenu::item:selected {
    background-color: #2563c9;
    color: #ffffff;
}

QMenu::separator {
    height: 1px;
    background: #232741;
    margin: 5px 8px;
}

/* ---------- checkboxes ---------- */
QCheckBox {
    spacing: 6px;
}

QCheckBox::indicator {
    width: 16px;
    height: 16px;
    border-radius: 5px;
    border: 1px solid #3a4066;
    background: #14172a;
}

QCheckBox::indicator:checked {
    background-color: #2fd47a;
    border: 1px solid #2fd47a;
}

QCheckBox::indicator:hover {
    border: 1px solid #4da3ff;
}

/* ---------- combo box (sort dropdown) ---------- */
QComboBox {
    background-color: #1a1e33;
    border: 1px solid #2b3050;
    border-radius: 9px;
    padding: 6px 12px;
    color: #e8eaf2;
    min-height: 20px;
}

QComboBox:hover {
    border: 1px solid #4da3ff;
}

QComboBox:on {
    background-color: #232846;
    border: 1px solid #4da3ff;
}

QComboBox QAbstractItemView {
    background-color: #171a2b;
    border: 1px solid #2b3050;
    border-radius: 8px;
    selection-background-color: #2563c9;
    selection-color: #ffffff;
    padding: 4px;
}

QComboBox::drop-down {
    border: none;
    width: 22px;
}

QComboBox::down-arrow {
    image: none;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid #8b90b5;
    margin-right: 8px;
}

/* ---------- progress bar (watched ratio) ---------- */
QProgressBar {
    background-color: #14172a;
    border: 1px solid #232741;
    border-radius: 8px;
    text-align: center;
    color: #c8cce0;
    font-size: 11px;
    min-height: 16px;
}

QProgressBar::chunk {
    border-radius: 7px;
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #2fd47a, stop:1 #38c9c0);
}

/* ---------- text inputs ---------- */
QLineEdit, QPlainTextEdit {
    background-color: #14172a;
    border: 1px solid #2b3050;
    border-radius: 9px;
    padding: 7px 10px;
    color: #e8eaf2;
    selection-background-color: #2563c9;
}

QLineEdit:focus, QPlainTextEdit:focus {
    border: 1px solid #4da3ff;
}

/* ---------- scrollbars ---------- */
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    border-radius: 5px;
    margin: 0px;
}

QScrollBar::handle:vertical {
    background: #2b3050;
    border-radius: 5px;
    min-height: 26px;
}

QScrollBar::handle:vertical:hover {
    background: #3d477a;
}

QScrollBar:add-line:vertical, QScrollBar:sub-line:vertical,
QScrollBar:add-page:vertical, QScrollBar:sub-page:vertical {
    height: 0px;
    background: transparent;
}

/* ---------- dialogs / tooltips ---------- */
QMessageBox, QFileDialog, QDialog {
    background-color: #0f1220;
}

QToolTip {
    background-color: #171a2b;
    color: #e8eaf2;
    border: 1px solid #2b3050;
    border-radius: 6px;
    padding: 5px 8px;
}

QStatusBar {
    background-color: #0f1220;
    color: #8b90b5;
    border-top: 1px solid #1c2038;
}
"""

# Alias so existing imports keep working
DARK_STYLESHEET = MODERN_STYLESHEET
