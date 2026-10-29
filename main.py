#!/usr/bin/env python3
"""
Entry point for Timeline Videos (Linux/Debian + Windows).

Startup sequence:
  1. QApplication + theme + language — fast, before anything else.
  2. A splash screen appears immediately.
  3. A background worker performs the heavy lifting behind it:
       • imports the whole UI module chain (PyQt widgets, ftfy…)
       • opens the folder's SQLite database (+ migrates any legacy
         .timeline_videos.json playlist)
       • scans the workspace folder for videos
       • warms up yt-dlp / checks ffprobe
  4. Only when everything is loaded does the main window appear —
     so the app never freezes at launch.

Run:
    ./run.sh          (Linux — creates .venv on first run)
    python3 main.py
"""

import os
import sys
import traceback

# Bundled pure-python deps shipped by the .deb package (Debian doesn't
# package ftfy, so build_deb.sh puts ftfy + wcwidth under ./vendor).
# Harmless everywhere else: the directory simply won't exist.
_VENDOR_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "vendor")
if os.path.isdir(_VENDOR_DIR) and _VENDOR_DIR not in sys.path:
    sys.path.insert(0, _VENDOR_DIR)

from PyQt5.QtCore import QSettings, Qt
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QApplication, QMessageBox

from i18n import is_rtl, set_language
from resources import ICON_ICO
from splash import SplashWindow, SplashWorker
from theme import MODERN_STYLESHEET

APP_ORG = "TimelineVideos"
APP_ID = "TimelineVideos"


def _install_excepthook():
    """PyQt5's default handler ABORTS the process (core dump) on any
    unhandled exception raised inside a slot. Print the traceback
    and show it in a dialog instead, so a single bad signal never
    kills the whole app silently."""

    def hook(exc_type, exc_value, exc_tb):
        if issubclass(exc_type, KeyboardInterrupt):
            sys.__excepthook__(exc_type, exc_value, exc_tb)
            return
        traceback.print_exception(exc_type, exc_value, exc_tb)
        if QApplication.instance() is None:
            return
        message = "".join(
            traceback.format_exception(exc_type, exc_value, exc_tb))
        QMessageBox.warning(
            None, "Timeline Videos — unexpected error",
            f"An unexpected error occurred:\n\n{message[-1600:]}",
        )

    sys.excepthook = hook


def main():
    _install_excepthook()
    app = QApplication(sys.argv)
    app.setApplicationName(APP_ID)
    app.setOrganizationName(APP_ORG)
    app.setWindowIcon(QIcon(ICON_ICO))
    app.setStyleSheet(MODERN_STYLESHEET)
    # Closing the window hides it to the system tray instead of
    # quitting (see MainWindow.closeEvent) — only "Exit" quits.
    app.setQuitOnLastWindowClosed(False)

    settings = QSettings(APP_ORG, APP_ID)
    set_language(settings.value("language", "ar"))
    app.setLayoutDirection(Qt.RightToLeft if is_rtl() else Qt.LeftToRight)

    splash = SplashWindow()
    splash.show()
    app.processEvents()

    # A remembered folder that no longer exists must not be re-created.
    last_folder = settings.value("last_folder", "") or ""
    if last_folder and not os.path.isdir(last_folder):
        last_folder = ""

    ctx = {}

    # ---------- heavy startup steps (run on the worker thread) ----------

    def load_ui_modules():
        import main_window  # noqa: PLC0415 — deliberately behind the splash
        return main_window

    def open_database():
        from playlist_manager import PlaylistManager  # noqa: PLC0415
        playlist = PlaylistManager(workspace_dir=last_folder or None)
        ctx["playlist"] = playlist
        return playlist

    def scan_folder():
        playlist = ctx.get("playlist")
        if playlist is not None and playlist.workspace_dir:
            return playlist.sync_with_folder()
        return 0

    def warm_media_tools():
        from media_probe import ffprobe_available  # noqa: PLC0415
        info = {"ffprobe": ffprobe_available()}
        try:
            import yt_dlp  # noqa: F401,PLC0415 — warm import (~1.5 s)
            info["ytdlp"] = True
        except ImportError:
            info["ytdlp"] = False
        return info

    worker = SplashWorker([
        ("splash.step.ui", load_ui_modules),
        ("splash.step.database", open_database),
        ("splash.step.scan", scan_folder),
        ("splash.step.probe", warm_media_tools),
    ])
    holder = {"worker": worker, "window": None}

    def on_ready(results):
        main_window = results.get("splash.step.ui")
        playlist = results.get("splash.step.database")
        if main_window is None or playlist is None:
            error = (results.get("splash.step.ui_error")
                     or results.get("splash.step.database_error")
                     or "unknown startup error")
            splash.hide()
            QMessageBox.critical(None, "Timeline Videos",
                                 f"Startup failed:\n{error}")
            app.quit()
            return
        window = main_window.MainWindow(playlist=playlist, settings=settings)
        holder["window"] = window
        window.show()
        splash.close()

    worker.done.connect(on_ready)
    worker.finished.connect(worker.deleteLater)
    worker.start()

    exit_code = app.exec_()

    # If we got here without a window (startup failure), release the DB.
    if holder["window"] is None:
        playlist = ctx.get("playlist")
        if playlist is not None:
            playlist.close()
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
