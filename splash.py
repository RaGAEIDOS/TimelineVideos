"""
splash.py
---------
A modern loading screen shown while the heavy startup work runs in a
background thread (importing UI modules, opening/migrating the
database, scanning the folder, warming up yt-dlp) — so the app never
freezes at launch and the main window appears only when everything
is already loaded.

Usage (main.py):

    splash = SplashWindow()
    splash.show()
    worker = SplashWorker(steps)          # [(key, callable), …]
    worker.step.connect(splash.show_step)
    worker.done.connect(on_ready)         # runs on the GUI thread
    worker.start()
"""

from typing import Callable, Dict, List, Tuple

from PyQt5.QtCore import Qt, QThread, pyqtSignal
from PyQt5.QtGui import QFont, QPixmap
from PyQt5.QtWidgets import (
    QHBoxLayout, QLabel, QProgressBar, QVBoxLayout, QWidget,
)

from i18n import tr
from resources import ICON_MARK_PNG


class SplashWorker(QThread):
    """Runs each step's callable on a background thread, reporting
    progress by signal. `done` carries a {key: result} dict and is
    emitted on the receiver's (GUI) thread — safe to touch widgets
    there."""

    step = pyqtSignal(int, int, str)       # current, total, step key
    done = pyqtSignal(object)              # {key: result}
    failed = pyqtSignal(str)               # fatal error message

    def __init__(self, steps: List[Tuple[str, Callable]],
                 parent=None):
        super().__init__(parent)
        self._steps = steps
        self.results: Dict[str, object] = {}

    def run(self):
        total = len(self._steps)
        for i, (key, fn) in enumerate(self._steps):
            self.step.emit(i + 1, total, key)
            try:
                self.results[key] = fn()
            except Exception as exc:  # keep going; UI decides what's fatal
                self.results[key] = None
                self.results[f"{key}_error"] = exc
        self.step.emit(total, total, "splash.step.ready")
        self.done.emit(self.results)


class SplashWindow(QWidget):
    """Frameless dark splash: logo, brand, tagline, step label and a
    determinate progress bar."""

    WIDTH, HEIGHT = 520, 300

    def __init__(self):
        super().__init__(None, Qt.SplashScreen | Qt.FramelessWindowHint)
        self.setFixedSize(self.WIDTH, self.HEIGHT)
        self.setAttribute(Qt.WA_StyledBackground, True)

        root = QVBoxLayout(self)
        root.setContentsMargins(36, 30, 36, 26)
        root.addStretch(1)

        # --- logo + brand ---
        brand_row = QHBoxLayout()
        brand_row.setAlignment(Qt.AlignCenter)
        logo = QLabel()
        mark = QPixmap(ICON_MARK_PNG)
        if not mark.isNull():
            logo.setPixmap(mark.scaledToHeight(58, Qt.SmoothTransformation))
        brand_row.addWidget(logo)
        brand_row.addSpacing(14)
        brand = QLabel(
            '<span style="color:#f0f2fa;">Timeline</span>'
            '<span style="color:#4da3ff;">Videos</span>'
        )
        brand_font = QFont()
        brand_font.setPointSize(24)
        brand_font.setBold(True)
        brand.setFont(brand_font)
        brand_row.addWidget(brand)
        root.addLayout(brand_row)

        root.addSpacing(10)

        self.tagline = QLabel(tr("splash.tagline"))
        self.tagline.setAlignment(Qt.AlignCenter)
        self.tagline.setStyleSheet("color: #8b90b5; font-size: 13px;")
        root.addWidget(self.tagline)

        root.addStretch(1)

        # --- progress ---
        self.bar = QProgressBar()
        self.bar.setRange(0, 100)
        self.bar.setValue(0)
        self.bar.setTextVisible(False)
        self.bar.setFixedHeight(10)
        root.addWidget(self.bar)

        root.addSpacing(10)

        self.step_label = QLabel("…")
        self.step_label.setAlignment(Qt.AlignCenter)
        self.step_label.setStyleSheet("color: #c8cce0; font-size: 12px;")
        root.addWidget(self.step_label)

    def show_step(self, current: int, total: int, key: str):
        total = max(total, 1)
        self.bar.setValue(int(current / total * 100))
        self.step_label.setText(f"{tr(key)}…")
        self.tagline.setText(tr("splash.tagline"))
