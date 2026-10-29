"""
video_db.py
-----------
Small SQLite database stored NEXT TO the videos themselves (one per
workspace folder), YouTube-style watched tracking:

    <folder>/.timeline_videos.db

…holds, for every video: path, title, order position, watched flag,
date added, last played, play count, file size and duration.

The default (no folder opened) list lives in
~/.simple_video_player/playlist.db instead.

Thread-safety: a single connection is shared with the background
duration-probing thread; every access goes through one lock.
"""

import os
import sqlite3
import threading
from typing import List, Optional

DB_FILENAME = ".timeline_videos.db"
DEFAULT_DB_FILE = os.path.join(
    os.path.expanduser("~"), ".simple_video_player", "playlist.db"
)

_SCHEMA = """
CREATE TABLE IF NOT EXISTS videos (
    path          TEXT PRIMARY KEY,
    title         TEXT NOT NULL,
    position      INTEGER NOT NULL DEFAULT 0,
    watched       INTEGER NOT NULL DEFAULT 0,
    added_at      REAL NOT NULL,
    last_opened_at REAL,
    size_bytes    INTEGER NOT NULL DEFAULT 0,
    duration_sec  REAL NOT NULL DEFAULT 0,
    play_count    INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX IF NOT EXISTS idx_videos_position ON videos(position);

CREATE TABLE IF NOT EXISTS meta (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL
);
"""


def db_path_for(workspace_dir: Optional[str]) -> str:
    if workspace_dir:
        return os.path.join(workspace_dir, DB_FILENAME)
    return DEFAULT_DB_FILE


class VideoDatabase:
    """Thin SQLite wrapper for one playlist workspace."""

    def __init__(self, path: str):
        self.path = path
        parent = os.path.dirname(path)
        if parent:
            os.makedirs(parent, exist_ok=True)
        self._lock = threading.RLock()
        self._conn = sqlite3.connect(path, check_same_thread=False)
        self._conn.row_factory = sqlite3.Row
        with self._lock:
            self._conn.executescript(_SCHEMA)
            self._conn.commit()

    # ---------- reads ----------

    def load_items(self) -> List[dict]:
        """All rows ordered by playlist position (path as tiebreak)."""
        with self._lock:
            rows = self._conn.execute(
                "SELECT * FROM videos ORDER BY position ASC, path ASC"
            ).fetchall()
        return [dict(row) for row in rows]

    def get_meta(self, key: str, default: Optional[str] = None) -> Optional[str]:
        with self._lock:
            row = self._conn.execute(
                "SELECT value FROM meta WHERE key = ?", (key,)
            ).fetchone()
        return row["value"] if row else default

    def set_meta(self, key: str, value: str):
        with self._lock:
            self._conn.execute(
                "INSERT INTO meta (key, value) VALUES (?, ?) "
                "ON CONFLICT(key) DO UPDATE SET value = excluded.value",
                (key, str(value)),
            )
            self._conn.commit()

    # ---------- writes ----------

    def upsert_item(self, item) -> None:
        """Insert or update one VideoItem (by path)."""
        with self._lock:
            self._conn.execute(
                "INSERT INTO videos (path, title, position, watched, added_at, "
                "last_opened_at, size_bytes, duration_sec, play_count) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
                "ON CONFLICT(path) DO UPDATE SET "
                "title = excluded.title, position = excluded.position, "
                "watched = excluded.watched, added_at = excluded.added_at, "
                "last_opened_at = excluded.last_opened_at, "
                "size_bytes = excluded.size_bytes, "
                "duration_sec = excluded.duration_sec, "
                "play_count = excluded.play_count",
                (
                    item.path, item.title, item.position,
                    1 if item.watched else 0, item.added_at,
                    item.last_opened_at, item.size_bytes,
                    item.duration_sec, item.play_count,
                ),
            )
            self._conn.commit()

    def delete_item(self, path: str):
        with self._lock:
            self._conn.execute("DELETE FROM videos WHERE path = ?", (path,))
            self._conn.commit()

    def rename_path(self, old_path: str, new_path: str):
        """Re-point a row to a new file path (used when a workspace
        folder was moved between machines, e.g. E:\\… -> /mnt/…).
        Callers must ensure no other row holds new_path already."""
        with self._lock:
            self._conn.execute(
                "UPDATE videos SET path = ? WHERE path = ?",
                (new_path, old_path),
            )
            self._conn.commit()

    def set_positions(self, path_order: List[str]):
        """Rewrite playlist order from a list of paths."""
        with self._lock:
            self._conn.executemany(
                "UPDATE videos SET position = ? WHERE path = ?",
                [(i, p) for i, p in enumerate(path_order)],
            )
            self._conn.commit()

    def set_watched(self, path: str, watched: bool):
        with self._lock:
            self._conn.execute(
                "UPDATE videos SET watched = ? WHERE path = ?",
                (1 if watched else 0, path),
            )
            self._conn.commit()

    def record_play(self, path: str, when: float):
        """YouTube-style: opening a video marks it watched + stamps
        the play time and bumps the play counter."""
        with self._lock:
            self._conn.execute(
                "UPDATE videos SET watched = 1, last_opened_at = ?, "
                "play_count = play_count + 1 WHERE path = ?",
                (when, path),
            )
            self._conn.commit()

    def set_metadata(self, path: str, size_bytes: Optional[int] = None,
                     duration_sec: Optional[float] = None):
        sets, params = [], []
        if size_bytes is not None:
            sets.append("size_bytes = ?")
            params.append(size_bytes)
        if duration_sec is not None:
            sets.append("duration_sec = ?")
            params.append(duration_sec)
        if not sets:
            return
        params.append(path)
        with self._lock:
            self._conn.execute(
                f"UPDATE videos SET {', '.join(sets)} WHERE path = ?", params
            )
            self._conn.commit()

    def close(self):
        with self._lock:
            try:
                self._conn.close()
            except sqlite3.Error:
                pass
