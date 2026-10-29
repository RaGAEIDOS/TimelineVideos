"""
main_window.py
--------------
Timeline Videos v2 — a bilingual (Arabic RTL / English) playlist
manager for Debian/Linux. No embedded video surface: double-clicking
(or "Play Selected") launches the video with the OS default player
(xdg-open → mpv/vlc…), the same as double-clicking it in the file
manager.

Each opened folder is its own workspace, persisted in a small
SQLite database stored NEXT TO the videos (see video_db.py):
watched status, play count, last played, size, duration —
YouTube-style, files are never moved around.

Row rendering note: rows use the QListWidgetItem's OWN native text
and checkbox (not a nested QWidget via setItemWidget) — a custom
child-widget-per-row breaks Arabic shaping (letters rendered
disconnected) on some platforms. Native item text uses Qt's proven
rendering path; the "SUB" badge and the duration are painted on top
by VideoRowDelegate.
"""

import os
import subprocess
import sys

from PyQt5.QtCore import Qt, QRect, QSize, QThread, pyqtSignal
from PyQt5.QtGui import QColor, QFont, QIcon, QPixmap
from PyQt5.QtWidgets import (
    QAction, QActionGroup, QAbstractItemView, QApplication, QComboBox,
    QDialog, QFileDialog, QFrame, QHBoxLayout, QLabel, QListWidget,
    QListWidgetItem, QMainWindow, QMenu, QMessageBox, QPlainTextEdit,
    QProgressBar, QPushButton, QStyledItemDelegate, QSystemTrayIcon,
    QVBoxLayout, QWidget,
)

from external_player import open_in_file_manager, open_with_default_app
from i18n import format_duration, get_language, is_rtl, set_language, tr
from media_probe import ffprobe_available, probe_duration
from playlist_manager import (
    PlaylistManager, SORT_ADDED_AT, SORT_HASH_NUMBER, SORT_LAST_OPENED,
    SORT_MANUAL, SORT_NAME, SORT_NUMBER, SORT_SIZE, has_subtitle,
    subtitle_labels,
)
from resources import ICON_ICO, ICON_MARK_PNG
from youtube_dialog import YoutubePlaylistDialog

APP_NAME = "Timeline Videos"
APP_VERSION = "2.1"

SUB_BADGE_COLOR = QColor("#2fd47a")
DURATION_COLOR = QColor("#aab0cc")
WATCHED_TINT = QColor("#5a5f7d")

HAS_SUBTITLE_ROLE = Qt.UserRole + 1
DURATION_ROLE = Qt.UserRole + 2
PATH_ROLE = Qt.UserRole + 3


# ------------------------------------------------------------------
# row delegate: native text/checkbox + painted duration & SUB badge
# ------------------------------------------------------------------

class VideoRowDelegate(QStyledItemDelegate):
    """Paints the item exactly as normal (native text/checkbox — see
    the module note), then draws a YouTube-style duration on the
    outer edge and, next to it, green labels for the actual subtitle
    files sitting beside the video ("SRT", "ar.SRT", "en.VTT"…)."""

    DURATION_WIDTH = 56
    MAX_LABELS = 3          # more subtitle files -> "+N"
    MAX_LABELS_WIDTH = 150

    def paint(self, painter, option, index):
        super().paint(painter, option, index)

        duration = index.data(DURATION_ROLE)
        labels = list(index.data(HAS_SUBTITLE_ROLE) or [])
        if not duration and not labels:
            return

        painter.save()
        rect = option.rect
        font = painter.font()
        font.setBold(True)
        font.setPointSize(max(font.pointSize() - 1, 7))
        painter.setFont(font)

        sub_text = self._labels_text(labels)
        sub_width = 0
        if sub_text:
            sub_width = min(
                painter.fontMetrics().horizontalAdvance(sub_text) + 10,
                self.MAX_LABELS_WIDTH,
            )

        if option.direction == Qt.RightToLeft:
            # RTL mirrors the row: the checkbox sits on the RIGHT edge,
            # so duration & subtitle labels go on the LEFT edge.
            x_cursor = rect.left() + 8
            if duration:
                painter.setPen(DURATION_COLOR)
                dur_rect = QRect(x_cursor, rect.top(),
                                 self.DURATION_WIDTH, rect.height())
                painter.drawText(dur_rect, Qt.AlignVCenter | Qt.AlignLeft,
                                 _format_clock(duration))
                x_cursor += self.DURATION_WIDTH + 6
            if sub_text:
                painter.setPen(SUB_BADGE_COLOR)
                painter.drawText(
                    QRect(x_cursor, rect.top(), sub_width, rect.height()),
                    Qt.AlignVCenter | Qt.AlignLeft, sub_text)
            painter.restore()
            return

        x_right = rect.right() - 8
        if duration:
            painter.setPen(DURATION_COLOR)
            dur_rect = QRect(rect.right() - self.DURATION_WIDTH - 8,
                             rect.top(), self.DURATION_WIDTH, rect.height())
            painter.drawText(dur_rect, Qt.AlignVCenter | Qt.AlignRight,
                             _format_clock(duration))
            x_right = rect.right() - self.DURATION_WIDTH - 14

        if sub_text:
            painter.setPen(SUB_BADGE_COLOR)
            painter.drawText(
                QRect(x_right - sub_width, rect.top(),
                      sub_width, rect.height()),
                Qt.AlignVCenter | Qt.AlignRight, sub_text)
        painter.restore()

    @staticmethod
    def _labels_text(labels) -> str:
        """['SRT'] -> 'SRT';  4+ labels -> 'a b c +2'."""
        if not labels:
            return ""
        shown = labels[:VideoRowDelegate.MAX_LABELS]
        extra = len(labels) - len(shown)
        text = " ".join(shown)
        if extra > 0:
            text += f" +{extra}"
        return text

    def sizeHint(self, option, index):
        size = super().sizeHint(option, index)
        return QSize(size.width() + 200, size.height())


def _format_clock(seconds: float) -> str:
    """Compact player-style clock: 83 -> '1:23', 3723 -> '1:02:03'."""
    total = int(round(seconds or 0))
    h, rem = divmod(total, 3600)
    m, s = divmod(rem, 60)
    if h:
        return f"{h}:{m:02d}:{s:02d}"
    return f"{m}:{s:02d}"


# ------------------------------------------------------------------
# stat card (summary bar building block)
# ------------------------------------------------------------------

class StatCard(QFrame):
    """One KPI box: big value + small caption (+ optional tip)."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setProperty("card", True)
        lay = QVBoxLayout(self)
        lay.setContentsMargins(14, 10, 14, 10)
        lay.setSpacing(2)

        self.value = QLabel("0")
        vfont = QFont()
        vfont.setPointSize(15)
        vfont.setBold(True)
        self.value.setFont(vfont)
        self.value.setStyleSheet("color: #f0f2fa;")

        self.caption = QLabel("")
        self.caption.setStyleSheet("color: #8b90b5; font-size: 11px;")

        self.sub = QLabel("")
        self.sub.setStyleSheet("color: #6f7599; font-size: 10px;")

        lay.addWidget(self.value)
        lay.addWidget(self.caption)
        lay.addWidget(self.sub)

    def set_value(self, value: str, caption: str, sub: str = "",
                  color: str = ""):
        self.value.setText(value)
        self.value.setStyleSheet(
            f"color: {color or '#f0f2fa'}; font-weight: bold;")
        self.caption.setText(caption)
        self.sub.setText(sub)


# ------------------------------------------------------------------
# background ffprobe worker (durations, cached in the DB afterwards)
# ------------------------------------------------------------------

class DurationProbeWorker(QThread):
    probed = pyqtSignal(str, float)   # path, seconds

    def __init__(self, queue, parent=None):
        super().__init__(parent)
        self._queue = list(queue)
        self._cancel = False

    def cancel(self):
        self._cancel = True

    def run(self):
        for item in self._queue:
            if self._cancel:
                return
            duration = probe_duration(item.path)
            if duration and duration > 0:
                self.probed.emit(item.path, duration)


# ------------------------------------------------------------------
# main window
# ------------------------------------------------------------------

class MainWindow(QMainWindow):
    def __init__(self, playlist: PlaylistManager, settings):
        super().__init__()
        self.settings = settings
        self.playlist = playlist
        self.tray_icon = None
        self._probe_worker = None
        self._suppress_check_events = False

        self.setWindowTitle(APP_NAME)
        self.setWindowIcon(QIcon(ICON_ICO))
        self.resize(720, 760)

        geometry = settings.value("geometry")
        if geometry is not None:
            try:
                self.restoreGeometry(geometry)
            except TypeError:
                pass

        self._build_menu()
        self._build_ui()
        self._connect_signals()
        self._build_tray()
        # Safety: never let app exit kill a running probe thread.
        app = QApplication.instance()
        if app is not None:
            app.aboutToQuit.connect(self._stop_duration_probe)
        self.retranslate()
        self._restore_sort_preference()
        self._update_window_title()
        self._refresh_playlist_widget()
        self._update_stats()
        self._start_duration_probe()

    def _restore_sort_preference(self):
        """Re-apply the sort the user last chose (manual = keep the
        saved drag order exactly as is)."""
        key = self.settings.value("sort_key", SORT_MANUAL) or SORT_MANUAL
        if key != SORT_MANUAL:
            self.playlist.sort_by(key)
        idx = self.sort_combo.findData(key)
        self.sort_combo.blockSignals(True)
        if idx >= 0:
            self.sort_combo.setCurrentIndex(idx)
        self.sort_combo.blockSignals(False)

    # ------------------------------------------------------------------
    # menu bar
    # ------------------------------------------------------------------

    def _build_menu(self):
        menubar = self.menuBar()
        if sys.platform.startswith("linux"):
            # KDE/GNOME sessions with a DBus "global menu" hide the
            # in-window menubar and expect a shell applet to show it —
            # when that applet is missing, the menus vanish entirely.
            menubar.setNativeMenuBar(False)

        self.file_menu = menubar.addMenu("")
        self.open_folder_action = QAction(self)
        self.open_folder_action.setShortcut("Ctrl+O")
        self.open_folder_action.triggered.connect(self._on_open_folder)
        self.file_menu.addAction(self.open_folder_action)

        self.youtube_action = QAction(self)
        self.youtube_action.setShortcut("Ctrl+Y")
        self.youtube_action.triggered.connect(self._on_youtube_import)
        self.file_menu.addAction(self.youtube_action)

        self.file_menu.addSeparator()

        self.exit_action = QAction(self)
        self.exit_action.setShortcut("Ctrl+Q")
        self.exit_action.triggered.connect(self._quit_app)
        self.file_menu.addAction(self.exit_action)

        self.language_menu = menubar.addMenu("")
        self.lang_group = QActionGroup(self)
        self.lang_group.setExclusive(True)
        self.lang_ar_action = QAction(self, checkable=True)
        self.lang_en_action = QAction(self, checkable=True)
        for action in (self.lang_ar_action, self.lang_en_action):
            self.lang_group.addAction(action)
            self.language_menu.addAction(action)
        self.lang_ar_action.triggered.connect(lambda: self._set_language("ar"))
        self.lang_en_action.triggered.connect(lambda: self._set_language("en"))

        self.help_menu = menubar.addMenu("")
        self.about_action = QAction(self)
        self.about_action.triggered.connect(self._show_about)
        self.help_menu.addAction(self.about_action)

    # ------------------------------------------------------------------
    # UI construction
    # ------------------------------------------------------------------

    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)
        layout.setContentsMargins(16, 12, 16, 12)
        layout.setSpacing(10)

        # --- header: logo + brand + language + open folder ---
        header = QHBoxLayout()
        header.setSpacing(8)
        self.logo_label = QLabel()
        mark = QPixmap(ICON_MARK_PNG)
        if not mark.isNull():
            self.logo_label.setPixmap(
                mark.scaledToHeight(34, Qt.SmoothTransformation))
        self.brand_label = QLabel(
            '<span style="color:#f0f2fa;">Timeline</span>'
            '<span style="color:#4da3ff;">Videos</span>'
        )
        brand_font = QFont()
        brand_font.setPointSize(17)
        brand_font.setBold(True)
        self.brand_label.setFont(brand_font)
        header.addWidget(self.logo_label)
        header.addWidget(self.brand_label)
        header.addStretch()

        self.lang_btn = QPushButton()
        self.lang_btn.setFixedWidth(92)
        self.open_folder_btn = QPushButton()
        header.addWidget(self.lang_btn)
        header.addWidget(self.open_folder_btn)
        layout.addLayout(header)

        # --- folder line ---
        self.folder_label = QLabel()
        self.folder_label.setStyleSheet("color: #8b90b5; font-size: 11px;")
        layout.addWidget(self.folder_label)

        # --- toolbar: sort + actions ---
        toolbar = QHBoxLayout()
        toolbar.setSpacing(8)
        self.sort_label = QLabel()
        self.sort_combo = QComboBox()
        self.sort_combo.setMinimumWidth(170)
        self.sort_combo.setFixedHeight(34)
        self._sort_options = [
            (SORT_MANUAL, "sort.manual"),
            (SORT_HASH_NUMBER, "sort.hash_number"),
            (SORT_NUMBER, "sort.number"),
            (SORT_NAME, "sort.name"),
            (SORT_SIZE, "sort.size"),
            (SORT_ADDED_AT, "sort.added_at"),
            (SORT_LAST_OPENED, "sort.last_opened"),
        ]
        for _key, label_key in self._sort_options:
            self.sort_combo.addItem("", userData=_key)
        toolbar.addWidget(self.sort_label)
        toolbar.addWidget(self.sort_combo)
        toolbar.addStretch()

        self.add_btn = QPushButton()
        self.remove_btn = QPushButton()
        self.play_btn = QPushButton()
        self.play_btn.setProperty("accent", True)
        toolbar.addWidget(self.add_btn)
        toolbar.addWidget(self.remove_btn)
        toolbar.addWidget(self.play_btn)
        layout.addLayout(toolbar)

        # --- stats cards ---
        stats_row = QHBoxLayout()
        stats_row.setSpacing(10)
        self.total_card = StatCard()
        self.watched_card = StatCard()
        self.queued_card = StatCard()
        stats_row.addWidget(self.total_card, stretch=1)
        stats_row.addWidget(self.watched_card, stretch=1)
        stats_row.addWidget(self.queued_card, stretch=1)
        layout.addLayout(stats_row)

        # --- progress ---
        self.progress_bar = QProgressBar()
        self.progress_bar.setRange(0, 1000)
        self.progress_bar.setValue(0)
        self.progress_bar.setTextVisible(True)
        self.progress_bar.setFormat("%p%")
        self.progress_bar.setFixedHeight(18)
        layout.addWidget(self.progress_bar)

        # --- playlist ---
        self.playlist_widget = QListWidget()
        self.playlist_widget.setDragDropMode(QAbstractItemView.InternalMove)
        self.playlist_widget.setContextMenuPolicy(Qt.CustomContextMenu)
        self.playlist_widget.setItemDelegate(
            VideoRowDelegate(self.playlist_widget))
        layout.addWidget(self.playlist_widget, stretch=1)

        # --- hint ---
        self.hint_label = QLabel()
        self.hint_label.setWordWrap(True)
        self.hint_label.setStyleSheet("color: #6f7599; font-size: 11px;")
        layout.addWidget(self.hint_label)

    def _connect_signals(self):
        self.add_btn.clicked.connect(self._on_add_videos)
        self.remove_btn.clicked.connect(self._on_remove_video)
        self.play_btn.clicked.connect(self._on_open_selected)
        self.lang_btn.clicked.connect(self._toggle_language)
        self.open_folder_btn.clicked.connect(self._on_open_folder)
        self.sort_combo.currentIndexChanged.connect(self._on_sort_changed)

        self.playlist_widget.itemDoubleClicked.connect(
            self._on_item_double_clicked)
        self.playlist_widget.itemChanged.connect(self._on_item_checked_changed)
        self.playlist_widget.model().rowsMoved.connect(self._on_rows_moved)
        self.playlist_widget.customContextMenuRequested.connect(
            self._on_context_menu)

    # ------------------------------------------------------------------
    # system tray
    # ------------------------------------------------------------------

    def _build_tray(self):
        if not QSystemTrayIcon.isSystemTrayAvailable():
            self.tray_icon = None
            return

        self.tray_icon = QSystemTrayIcon(QIcon(ICON_ICO), self)

        self.tray_menu = QMenu()
        self.tray_show_action = self.tray_menu.addAction("")
        self.tray_show_action.triggered.connect(self._show_from_tray)
        self.tray_menu.addSeparator()
        self.tray_exit_action = self.tray_menu.addAction("")
        self.tray_exit_action.triggered.connect(self._quit_app)

        self.tray_icon.setContextMenu(self.tray_menu)
        self.tray_icon.activated.connect(self._on_tray_activated)
        self.tray_icon.show()

    def _on_tray_activated(self, reason):
        if reason in (QSystemTrayIcon.Trigger, QSystemTrayIcon.DoubleClick):
            self._show_from_tray()

    def _show_from_tray(self):
        self.showNormal()
        self.raise_()
        self.activateWindow()

    def _quit_app(self):
        self.settings.setValue("geometry", self.saveGeometry())
        self.settings.sync()
        self._stop_duration_probe()
        if self.tray_icon is not None:
            self.tray_icon.hide()
        QApplication.instance().quit()

    def closeEvent(self, event):
        # The window's own close button hides to the tray instead of
        # quitting; only Exit (menu or tray) really quits.
        self.settings.setValue("geometry", self.saveGeometry())
        if self.tray_icon is None:
            event.accept()
            return
        event.ignore()
        self.hide()
        self.tray_icon.showMessage(
            APP_NAME, tr("tray.still_running"),
            QSystemTrayIcon.Information, 2000,
        )

    # ------------------------------------------------------------------
    # translation
    # ------------------------------------------------------------------

    def retranslate(self):
        """Re-reads every visible string for the current language."""
        self.file_menu.setTitle(tr("menu.file"))
        self.open_folder_action.setText(tr("menu.open_folder"))
        self.youtube_action.setText(tr("menu.youtube_import"))
        self.exit_action.setText(tr("menu.exit"))
        self.language_menu.setTitle(tr("menu.language"))
        self.lang_ar_action.setText(tr("menu.lang_ar"))
        self.lang_en_action.setText(tr("menu.lang_en"))
        self.help_menu.setTitle(tr("menu.help"))
        self.about_action.setText(tr("menu.about"))

        self.lang_btn.setText(tr("btn.language"))
        self.lang_btn.setToolTip(tr("btn.language_tip"))
        self.open_folder_btn.setText(tr("btn.open_folder"))
        self.add_btn.setText(tr("btn.add"))
        self.remove_btn.setText(tr("btn.remove"))
        self.play_btn.setText(tr("btn.play"))

        self.sort_label.setText(tr("sort.label"))
        self.sort_combo.setToolTip(tr("sort.tip"))
        for i, (_key, label_key) in enumerate(self._sort_options):
            self.sort_combo.setItemText(i, tr(label_key))

        self.hint_label.setText(tr("hint.text"))
        if self.tray_icon is not None:
            self.tray_icon.setToolTip(tr("tray.tooltip"))
            self.tray_show_action.setText(tr("tray.show", app=APP_NAME))
            self.tray_exit_action.setText(tr("tray.exit"))

        self.lang_ar_action.setChecked(is_rtl())
        self.lang_en_action.setChecked(not is_rtl())

        self._update_folder_label()
        self._update_stats()

    def _set_language(self, lang: str):
        if lang == get_language():
            return
        set_language(lang)
        self.settings.setValue("language", lang)
        app = QApplication.instance()
        app.setLayoutDirection(Qt.RightToLeft if is_rtl() else Qt.LeftToRight)
        self.retranslate()

    def _toggle_language(self):
        self._set_language("en" if is_rtl() else "ar")

    # ------------------------------------------------------------------
    # about
    # ------------------------------------------------------------------

    def _show_about(self):
        QMessageBox.about(
            self, tr("about.title", app=APP_NAME),
            f"<h3>{APP_NAME} v{APP_VERSION}</h3>"
            "<p>Play. Learn. Track. Grow.</p>"
            "<p>A lightweight bilingual playlist manager that opens "
            "videos with your own default player and tracks what "
            "you've watched in a local SQLite database — YouTube-style, "
            "files stay where they are.</p>"
            "<p>Developed by <b>رجائي (Ragaey)</b><br>"
            "Alexandria, Egypt</p>"
            "<p style='color:#8b90b5; font-size:11px;'>"
            "Built with Python &amp; PyQt5 · Runs on Linux (Debian) &amp; Windows</p>"
        )

    # ------------------------------------------------------------------
    # folder workspace
    # ------------------------------------------------------------------

    def _on_open_folder(self):
        start = self.playlist.workspace_dir or os.path.expanduser("~")
        folder = QFileDialog.getExistingDirectory(self, tr("menu.open_folder"), start)
        if not folder or folder == self.playlist.workspace_dir:
            return
        self._stop_duration_probe()
        self.playlist.close()
        self.playlist = PlaylistManager(workspace_dir=folder)
        self.playlist.sync_with_folder()
        self.settings.setValue("last_folder", folder)
        self._update_window_title()
        self._refresh_playlist_widget()
        self._update_stats()
        self._start_duration_probe()

    def _update_window_title(self):
        if self.playlist.workspace_dir:
            name = os.path.basename(os.path.normpath(self.playlist.workspace_dir))
            self.setWindowTitle(f"{APP_NAME} — {name}")
        else:
            self.setWindowTitle(APP_NAME)

    def _update_folder_label(self):
        if self.playlist.workspace_dir:
            self.folder_label.setText(
                tr("folder.label", path=self.playlist.workspace_dir))
        else:
            self.folder_label.setText(tr("folder.none"))

    # ------------------------------------------------------------------
    # playlist <-> UI
    # ------------------------------------------------------------------

    def _refresh_playlist_widget(self):
        self._suppress_check_events = True
        self.playlist_widget.blockSignals(True)
        self.playlist_widget.clear()
        for item in self.playlist.items:
            row = QListWidgetItem(item.title)
            row.setFlags(row.flags() | Qt.ItemIsUserCheckable)
            row.setCheckState(Qt.Checked if item.watched else Qt.Unchecked)
            row.setData(HAS_SUBTITLE_ROLE, subtitle_labels(item.path))
            row.setData(DURATION_ROLE, item.duration_sec or None)
            row.setData(PATH_ROLE, item.path)
            if item.watched:
                fg = row.foreground()
                fg.setColor(WATCHED_TINT)
                row.setForeground(fg)
            self.playlist_widget.addItem(row)
        if 0 <= self.playlist.current_index < self.playlist_widget.count():
            self.playlist_widget.setCurrentRow(self.playlist.current_index)
        self.playlist_widget.blockSignals(False)
        self._suppress_check_events = False

    def _update_stats(self):
        s = self.playlist.stats()

        total_sub = format_duration(s["total_duration"])
        if s["unknown_duration"]:
            total_sub += " " + tr("stats.unknown_suffix", n=s["unknown_duration"])
        self.total_card.set_value(
            tr("stats.videos", n=s["total"]),
            tr("stats.total"),
            tr("stats.total_time") + ": " + total_sub,
        )
        self.watched_card.set_value(
            str(s["watched"]), tr("stats.watched"),
            format_duration(s["watched_duration"]),
            color="#2fd47a",
        )
        self.queued_card.set_value(
            str(s["queued"]), tr("stats.queued"),
            format_duration(s["remaining_duration"]),
            color="#4da3ff",
        )
        self.progress_bar.setToolTip(tr("stats.progress_tip"))
        if s["total"]:
            self.progress_bar.setValue(
                int(s["watched"] / s["total"] * 1000))
        else:
            self.progress_bar.setValue(0)

    def _row_for_path(self, path: str) -> int:
        for row in range(self.playlist_widget.count()):
            if self.playlist_widget.item(row).data(PATH_ROLE) == path:
                return row
        return -1

    # ------------------------------------------------------------------
    # actions
    # ------------------------------------------------------------------

    def _on_add_videos(self):
        start = self.playlist.workspace_dir or os.path.expanduser("~")
        paths, _ = QFileDialog.getOpenFileNames(
            self, tr("btn.add"), start,
            "Video files (*.mp4 *.mkv *.avi *.mov *.webm *.flv *.wmv *.m4v);;All files (*)"
        )
        for path in paths:
            self.playlist.add_video(path)
        if paths:
            self._refresh_playlist_widget()
            self._update_stats()
            self._start_duration_probe()

    def _on_remove_video(self):
        row = self.playlist_widget.currentRow()
        if row < 0:
            return
        self.playlist.remove_video(row)
        self._refresh_playlist_widget()
        self._update_stats()

    def _on_sort_changed(self, index):
        key = self.sort_combo.itemData(index)
        if key is None or key == SORT_MANUAL:
            self.settings.setValue("sort_key", SORT_MANUAL)
            return
        self.playlist.sort_by(key)
        self.settings.setValue("sort_key", key)
        self._refresh_playlist_widget()
        self._update_stats()

    def _select_manual_sort(self):
        """After a manual drag: reflect it in the combo without
        re-sorting anything."""
        self.sort_combo.blockSignals(True)
        idx = self.sort_combo.findData(SORT_MANUAL)
        if idx >= 0:
            self.sort_combo.setCurrentIndex(idx)
        self.sort_combo.blockSignals(False)
        self.settings.setValue("sort_key", SORT_MANUAL)

    def _on_rows_moved(self, parent, start, end, dest, dest_row):
        if dest_row > start:
            dest_row -= 1
        self.playlist.move_video(start, dest_row)
        self._select_manual_sort()
        # row indices in the model shifted — refresh to stay in sync
        self._refresh_playlist_widget()

    def _on_item_double_clicked(self, item: QListWidgetItem):
        self._open_row(self.playlist_widget.row(item))

    def _on_open_selected(self):
        row = self.playlist_widget.currentRow()
        if row < 0:
            QMessageBox.information(self, tr("msg.no_selection.t"),
                                    tr("msg.no_selection.x"))
            return
        self._open_row(row)

    def _open_row(self, row: int):
        if not (0 <= row < len(self.playlist.items)):
            return
        video = self.playlist.items[row]
        try:
            open_with_default_app(video.path)
        except FileNotFoundError:
            QMessageBox.warning(self, tr("msg.file_missing.t"),
                                tr("msg.file_missing.x", path=video.path))
            return
        except (OSError, subprocess.SubprocessError) as exc:
            QMessageBox.warning(self, tr("msg.cant_open.t"), str(exc))
            return
        # YouTube-style: opening = watched, stamped + counted.
        self.playlist.record_opened(row)
        self._refresh_row(row)
        self._update_stats()

    def _refresh_row(self, row: int):
        if not (0 <= row < len(self.playlist.items)):
            return
        item = self.playlist.items[row]
        self._suppress_check_events = True
        self.playlist_widget.blockSignals(True)
        list_item = self.playlist_widget.item(row)
        if list_item is not None:
            list_item.setCheckState(
                Qt.Checked if item.watched else Qt.Unchecked)
            list_item.setData(DURATION_ROLE, item.duration_sec or None)
            fg = list_item.foreground()
            fg.setColor(WATCHED_TINT if item.watched else
                        self.palette().color(self.foregroundRole()))
            list_item.setForeground(fg)
            # Repaint just this row (update() takes a QModelIndex,
            # NOT a plain row number — an int raises TypeError).
            model_index = self.playlist_widget.model().index(row, 0)
            self.playlist_widget.update(model_index)
        self.playlist_widget.blockSignals(False)
        self._suppress_check_events = False

    def _on_item_checked_changed(self, item: QListWidgetItem):
        if self._suppress_check_events:
            return
        row = self.playlist_widget.row(item)
        checked = item.checkState() == Qt.Checked
        ok = (self.playlist.mark_watched(row) if checked
              else self.playlist.mark_unwatched(row))
        if not ok:
            QMessageBox.warning(self, tr("msg.cant_update.t"),
                                tr("msg.cant_update.x"))
            self._refresh_playlist_widget()
            return
        self._update_stats()

    # ------------------------------------------------------------------
    # context menu
    # ------------------------------------------------------------------

    def _on_context_menu(self, pos):
        row = self.playlist_widget.row(self.playlist_widget.itemAt(pos))
        if row < 0:
            return
        menu = QMenu(self)
        play_action = menu.addAction(tr("ctx.play"))
        menu.addSeparator()
        item = self.playlist.items[row]
        if item.watched:
            toggle_action = menu.addAction(tr("ctx.mark_unwatched"))
        else:
            toggle_action = menu.addAction(tr("ctx.mark_watched"))
        remove_action = menu.addAction(tr("ctx.remove"))
        open_folder_action = menu.addAction(tr("ctx.open_folder"))

        chosen = menu.exec_(self.playlist_widget.viewport().mapToGlobal(pos))
        if chosen == play_action:
            self._open_row(row)
        elif chosen == toggle_action:
            if item.watched:
                self.playlist.mark_unwatched(row)
            else:
                self.playlist.mark_watched(row)
            self._refresh_row(row)
            self._update_stats()
        elif chosen == remove_action:
            self.playlist.remove_video(row)
            self._refresh_playlist_widget()
            self._update_stats()
        elif chosen == open_folder_action:
            try:
                open_in_file_manager(item.path)
            except (OSError, FileNotFoundError) as exc:
                QMessageBox.warning(self, tr("msg.cant_open.t"), str(exc))

    # ------------------------------------------------------------------
    # YouTube playlist import
    # ------------------------------------------------------------------

    def _on_youtube_import(self):
        local_paths = [it.path for it in self.playlist.items]
        if not local_paths:
            QMessageBox.information(self, tr("yt.title"), tr("folder.none"))
            return
        dialog = YoutubePlaylistDialog(local_paths, self)
        if dialog.exec_() != QDialog.Accepted or not dialog.ordered_paths:
            return
        matched = self.playlist.apply_playlist_order(dialog.ordered_paths)
        self._select_manual_sort()
        self._refresh_playlist_widget()
        self._update_stats()
        QMessageBox.information(self, tr("yt.title"),
                                tr("yt.matched", n=matched))

    # ------------------------------------------------------------------
    # duration probing (background, cached in the DB)
    # ------------------------------------------------------------------

    def _start_duration_probe(self):
        if not ffprobe_available():
            return
        queue = self.playlist.items_needing_duration()
        if not queue:
            return
        self._stop_duration_probe()
        self._probe_worker = DurationProbeWorker(queue, self)
        self._probe_worker.probed.connect(self._on_duration_probed)
        self._probe_worker.finished.connect(self._on_probe_finished)
        self._probe_worker.start()

    def _stop_duration_probe(self):
        if self._probe_worker is not None:
            self._probe_worker.cancel()
            self._probe_worker.wait(3000)
            self._probe_worker = None

    def _on_duration_probed(self, path: str, duration: float):
        self.playlist.set_duration(path, duration)
        row = self._row_for_path(path)
        if row >= 0:
            self._refresh_row(row)
        self._update_stats()

    def _on_probe_finished(self):
        self._probe_worker = None
