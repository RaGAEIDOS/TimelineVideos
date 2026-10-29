"""
media_probe.py
--------------
Reads a video's duration using ffprobe (ships with ffmpeg, already
installed on this Debian box):

    ffprobe -v error -show_entries format=duration -of csv=p=0 file.mp4

Durations are cached in the folder's SQLite DB by the caller, so
each file is probed only once ever.
"""

import os
import shutil
import subprocess
from typing import Optional

FFPROBE_TIMEOUT = 20  # seconds per file — enough even for huge files


def ffprobe_available() -> bool:
    return shutil.which("ffprobe") is not None


def probe_duration(path: str) -> Optional[float]:
    """Duration in seconds, or None if it can't be read (missing
    ffprobe, corrupted file, unreadable…). Callers treat None/0 as
    'unknown'."""
    if not ffprobe_available() or not os.path.isfile(path):
        return None
    try:
        out = subprocess.run(
            [
                "ffprobe", "-v", "error",
                "-show_entries", "format=duration",
                "-of", "csv=p=0",
                path,
            ],
            capture_output=True, text=True, timeout=FFPROBE_TIMEOUT,
        )
    except (subprocess.SubprocessError, OSError):
        return None
    text = (out.stdout or "").strip()
    if not text or out.returncode != 0:
        return None
    try:
        value = float(text)
    except ValueError:
        return None
    if value != value or value < 0:  # NaN / negative guard
        return None
    return value


def file_size(path: str) -> int:
    try:
        return os.path.getsize(path)
    except OSError:
        return 0
