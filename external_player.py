"""
external_player.py
--------------------
Opens a video file with whatever application the operating system
has associated with it — exactly what happens when you double-click
the file in the file manager.

On Linux (Debian): xdg-open resolves the default handler (mpv, VLC,
SMPlayer… depending on the user's mime associations). If xdg-open is
missing, it falls back to the first installed well-known player.
On Windows: the file association (os.startfile).
On macOS: "open".

No embedded player, no bundled engine — the OS does the work.
"""

import os
import shutil
import subprocess
import sys

# Tried in order when xdg-open isn't available (Linux only).
LINUX_FALLBACK_PLAYERS = ("mpv", "vlc", "smplayer", "audacious", "x-www-browser")


def _linux_open(path: str) -> None:
    if shutil.which("xdg-open"):
        subprocess.Popen(["xdg-open", path])
        return
    for player in LINUX_FALLBACK_PLAYERS:
        if shutil.which(player):
            subprocess.Popen([player, path])
            return
    raise OSError("No default opener found (xdg-open / mpv / vlc …)")


def open_with_default_app(path: str) -> None:
    """
    Launch `path` with the OS-default handler for its file type.
    Raises FileNotFoundError if the path doesn't exist, and
    OSError/subprocess.SubprocessError on launch failure — callers
    should catch these and show a friendly message.
    """
    if not os.path.exists(path):
        raise FileNotFoundError(path)

    if sys.platform.startswith("win"):
        os.startfile(path)  # Windows file association (MPV, in his case)
    elif sys.platform == "darwin":
        subprocess.Popen(["open", path])
    else:
        _linux_open(path)


def open_in_file_manager(path: str) -> None:
    """Reveal `path` in the OS file manager (Explorer on Windows,
    Finder on macOS, the default file manager on Linux). Used by the
    playlist's right-click 'Open file's folder' action."""
    if not os.path.exists(path):
        raise FileNotFoundError(path)
    if sys.platform.startswith("win"):
        if os.path.isfile(path):
            # /select, highlights the file inside its folder
            subprocess.Popen(["explorer", "/select,",
                              os.path.normpath(path)])
        else:
            os.startfile(path)
    elif sys.platform == "darwin":
        target = path if os.path.isdir(path) else os.path.dirname(path)
        subprocess.Popen(["open", target])
    else:
        target = path if os.path.isdir(path) else os.path.dirname(path)
        if shutil.which("xdg-open"):
            subprocess.Popen(["xdg-open", target])
        else:
            raise OSError("xdg-open not found")
