"""
playlist_manager.py
--------------------
The playlist data layer: video items + persistence.

Version 2 changes (Debian/Linux rewrite):
  • Persistence is now a small SQLite database stored NEXT TO the
    videos (video_db.py) — YouTube-style watched tracking, no more
    moving files into a "Watched" folder. Opening a video records:
    watched + last-played time + play count.
  • Legacy ".timeline_videos.json" files are migrated into the
    database automatically on first open (the JSON file is kept
    untouched as a backup).
  • Rich metadata per video: file size, duration (ffprobe, cached in
    the DB), date added, last played, play count.
  • Advanced sorting: name (natural), episode number, size, date
    added, last played — plus manual drag order and YouTube playlist
    order (youtube_import.py).
  • stats() powers the summary bar: totals + watched/queued counts
    and durations.
"""

import json
import os
import re
import time
from dataclasses import dataclass, field
from typing import List, Optional

try:
    import ftfy  # repairs garbled (mojibake) titles
except ImportError:  # app still works without it, just no auto-repair
    ftfy = None

from media_probe import file_size, probe_duration
from video_db import VideoDatabase, db_path_for

WORKSPACE_DATA_FILENAME = ".timeline_videos.json"  # legacy, read-only backup
LEGACY_DEFAULT_DATA_FILE = os.path.join(
    os.path.expanduser("~"), ".simple_video_player", "playlist.json"
)

VIDEO_EXTENSIONS = {".mp4", ".mkv", ".avi", ".mov", ".webm", ".flv", ".wmv", ".m4v"}
SUBTITLE_EXTENSIONS = {".srt", ".ass", ".ssa", ".vtt", ".sub"}

SORT_MANUAL = "manual"
SORT_NAME = "name"
SORT_NUMBER = "number"
SORT_HASH_NUMBER = "hash_number"
SORT_SIZE = "size"
SORT_ADDED_AT = "added_at"
SORT_LAST_OPENED = "last_opened"
SORT_KEYS = (SORT_MANUAL, SORT_NAME, SORT_NUMBER, SORT_HASH_NUMBER,
             SORT_SIZE, SORT_ADDED_AT, SORT_LAST_OPENED)

_NUMBER_RE = re.compile(r"(\d+)")
# "… Computer Science #28-(1080p24)" -> the number right after '#'
_HASH_NUMBER_RE = re.compile(r"#\s*(\d+)")
_NATURAL_CHUNK_RE = re.compile(r"(\d+)")


def extract_number(title: str) -> Optional[int]:
    """Last integer found in the title, or None. Real filenames look
    like "(1080p60)-<title> 007" — the episode number is at the END,
    so using the LAST number avoids grabbing the resolution tag."""
    matches = _NUMBER_RE.findall(title)
    return int(matches[-1]) if matches else None


def extract_hash_number(title: str) -> Optional[int]:
    """First integer that follows a '#' in the title, or None —
    e.g. "…Crash Course #28-(1080p24)" -> 28 (NOT 24 from the
    resolution tag)."""
    match = _HASH_NUMBER_RE.search(title)
    return int(match.group(1)) if match else None


def natural_key(text: str):
    """Human sort: 'Ep 2' before 'Ep 10' (digits compare as numbers)."""
    return [int(tok) if tok.isdigit() else tok.casefold()
            for tok in _NATURAL_CHUNK_RE.split(text)]


def is_video_file(path: str) -> bool:
    return os.path.splitext(path)[1].lower() in VIDEO_EXTENSIONS


def scan_folder_videos(folder: str) -> List[str]:
    """Video files directly inside `folder` (top-level only — never
    descends into sub-folders), sorted by filename."""
    try:
        entries = os.listdir(folder)
    except OSError:
        return []
    videos = [
        os.path.join(folder, name) for name in entries
        if is_video_file(name) and os.path.isfile(os.path.join(folder, name))
    ]
    videos.sort(key=lambda p: os.path.basename(p).casefold())
    return videos


def subtitle_files(video_path: str) -> List[str]:
    """Subtitle files belonging to this video, sorted. Matches both
    plain ("Video.srt") and language-tagged ("Video.en.vtt",
    "Video.ar.srt" — the yt-dlp style) names: any file in the folder
    that starts with "<video base>." and ends with a subtitle
    extension. The trailing dot keeps "Episode 1" from matching
    "Episode 10.srt"."""
    folder = os.path.dirname(video_path) or "."
    prefix = os.path.basename(os.path.splitext(video_path)[0]) + "."
    found = []
    try:
        names = os.listdir(folder)
    except OSError:
        return []
    for name in names:
        if not name.startswith(prefix):
            continue
        if os.path.splitext(name)[1].lower() in SUBTITLE_EXTENSIONS:
            found.append(os.path.join(folder, name))
    return sorted(found)


def subtitle_labels(video_path: str) -> List[str]:
    """Short display labels for the subtitle files, derived from the
    part between the video's base name and the extension:
        "Video.srt"          -> ["SRT"]
        "Video.ar.srt"       -> ["ar.SRT"]
        "Video.en.vtt" + …   -> ["en.VTT", …]
    An empty list means no subtitle (no SUB/label is drawn)."""
    base = os.path.splitext(video_path)[0]
    labels = []
    for path in subtitle_files(video_path):
        suffix = os.path.splitext(path)[0][len(base):].lstrip(".")
        ext = os.path.splitext(path)[1][1:].upper()
        labels.append(f"{suffix}.{ext}" if suffix else ext)
    return labels


def has_subtitle(video_path: str) -> bool:
    """True if a subtitle file with the same base name sits next to the video."""
    return bool(subtitle_files(video_path))


# ---------- mojibake repair (garbled Arabic filenames) ----------

def _looks_like_cp1256_mojibake(text: str) -> bool:
    """Signature of UTF-8 Arabic bytes wrongly decoded as Windows-1256:
    nearly every Arabic char becomes just 'ط' or 'ظ' (the only two
    possible UTF-8 lead bytes of the Arabic block). Real Arabic never
    concentrates like that."""
    arabic_chars = [c for c in text if "\u0600" <= c <= "\u06FF"]
    if len(arabic_chars) < 6:
        return False
    marker = sum(1 for c in arabic_chars if c in ("ط", "ظ"))
    return (marker / len(arabic_chars)) > 0.35


def fix_mojibake(text: str) -> str:
    """Repairs titles garbled by an encoding mismatch upstream (very
    common with Arabic filenames from download tools). Handles the
    Windows-1250..1254 style via ftfy (when installed) and the
    Windows-1256 Arabic style explicitly. Correct text is untouched.
    """
    fixed = text
    if ftfy is not None:
        try:
            fixed = ftfy.fix_text(text)
        except Exception:
            fixed = text

    for candidate in (fixed, text):
        if _looks_like_cp1256_mojibake(candidate):
            try:
                reversed_text = candidate.encode("cp1256").decode("utf-8")
                if not _looks_like_cp1256_mojibake(reversed_text):
                    return reversed_text
            except (UnicodeEncodeError, UnicodeDecodeError):
                pass
    return fixed


@dataclass
class VideoItem:
    path: str
    title: str
    watched: bool = False
    added_at: float = field(default_factory=time.time)
    last_opened_at: Optional[float] = None
    size_bytes: int = 0
    duration_sec: float = 0.0   # 0 = unknown (not probed yet / no ffprobe)
    play_count: int = 0
    position: int = 0           # transient: written to the DB on save

    def __post_init__(self):
        self.title = fix_mojibake(self.title)

    @staticmethod
    def from_path(path: str) -> "VideoItem":
        title = os.path.splitext(os.path.basename(path))[0]
        return VideoItem(path=path, title=title,
                         size_bytes=file_size(path))


def _item_from_row(row: dict) -> VideoItem:
    return VideoItem(
        path=row["path"],
        title=row["title"],
        watched=bool(row["watched"]),
        added_at=row["added_at"],
        last_opened_at=row["last_opened_at"],
        size_bytes=row["size_bytes"],
        duration_sec=row["duration_sec"],
        play_count=row["play_count"],
    )


class PlaylistManager:
    """One playlist workspace, persisted in SQLite."""

    def __init__(self, workspace_dir: Optional[str] = None):
        """workspace_dir=None -> the default (global) list in
        ~/.simple_video_player/playlist.db; <path> -> that folder's
        own database, stored inside it."""
        self.workspace_dir = workspace_dir
        self.db = VideoDatabase(db_path_for(workspace_dir))
        self.items: List[VideoItem] = []
        self.current_index: int = -1
        self._load()

    # ---------- persistence ----------

    def _load(self):
        rows = self.db.load_items()
        self.items = [_item_from_row(r) for r in rows]
        if self.items:
            raw = self.db.get_meta("current_index", "-1")
            try:
                self.current_index = int(raw)
            except (TypeError, ValueError):
                self.current_index = -1
            self._clamp_current()
            return
        # Fresh database — try to inherit an old JSON playlist.
        if self._migrate_legacy_json():
            self._save_all()

    def _migrate_legacy_json(self) -> bool:
        """One-time import from a pre-DB ".timeline_videos.json"
        (folder workspace or the old global playlist.json). The JSON
        file is left untouched as a backup. Returns True if anything
        was imported."""
        candidates = []
        if self.workspace_dir:
            candidates.append(os.path.join(self.workspace_dir,
                                           WORKSPACE_DATA_FILENAME))
        candidates.append(LEGACY_DEFAULT_DATA_FILE)

        for candidate in candidates:
            if not os.path.isfile(candidate):
                continue
            try:
                with open(candidate, "r", encoding="utf-8") as f:
                    raw = json.load(f)
            except (OSError, json.JSONDecodeError):
                continue
            raw_items = raw.get("items", [])
            for i, data in enumerate(raw_items):
                try:
                    item = VideoItem(
                        path=data["path"],
                        title=data.get("title")
                              or os.path.splitext(os.path.basename(data["path"]))[0],
                        watched=bool(data.get("watched", False)),
                        added_at=data.get("added_at") or time.time(),
                        last_opened_at=data.get("last_opened_at"),
                    )
                except (KeyError, TypeError):
                    continue
                item.size_bytes = file_size(item.path)
                item.position = i  # noqa: B010 — see VideoItem.position
                self.items.append(item)
            try:
                self.current_index = int(raw.get("current_index", -1))
            except (TypeError, ValueError):
                self.current_index = -1
            self._clamp_current()
            return bool(self.items)
        return False

    def _save_all(self):
        """Rewrite every row + order positions (used after structural
        changes: add/remove/reorder/sort/import)."""
        for i, item in enumerate(self.items):
            item.position = i  # noqa: B010 — transient, used by upsert
            self.db.upsert_item(item)
        self.db.set_positions([it.path for it in self.items])
        self.db.set_meta("current_index", str(self.current_index))

    def _save_order(self):
        self.db.set_positions([it.path for it in self.items])
        self.db.set_meta("current_index", str(self.current_index))

    def _clamp_current(self):
        if not (-1 <= self.current_index < len(self.items)):
            self.current_index = 0 if self.items else -1

    # ---------- folder scanning ----------

    def _item_for_path(self, path: str) -> Optional[VideoItem]:
        for item in self.items:
            if item.path == path:
                return item
        return None

    def _reconcile_dead_paths(self) -> bool:
        """A workspace folder can be moved between machines (e.g.
        E:\\… on Windows -> /mnt/… on Linux). The playlist then holds
        rows whose paths no longer exist, while the same videos sit
        right here under new paths — they'd show up as duplicates.

        Every row whose file is gone is matched against the videos
        physically in the folder by FILE NAME:
          • no live row for that file yet -> the row is re-pointed to
            the local file (keeping its watched/order/history data);
          • a live row already covers it  -> the dead row's watched /
            play-count / last-played history is merged into the live
            row and the dead row is deleted.
        Returns True if anything changed."""
        if not self.workspace_dir:
            return False
        local_by_name = {}
        for path in scan_folder_videos(self.workspace_dir):
            local_by_name.setdefault(os.path.basename(path).casefold(), path)

        def basename_any(path: str) -> str:
            # Legacy Windows paths use mixed separators
            # ("E:/a/b\c.webm") — POSIX basename would keep the
            # backslash part in the name, so split on both.
            return os.path.basename(path.replace("\\", "/")).casefold()

        changed = False
        for item in list(self.items):
            if os.path.exists(item.path):
                continue
            local = local_by_name.get(basename_any(item.path))
            if local is None:
                continue  # file genuinely gone — keep the row as is
            live = self._item_for_path(local)
            if live is None:
                self.db.rename_path(item.path, local)
                item.path = local
                item.size_bytes = file_size(local)
            else:
                live.watched = live.watched or item.watched
                live.play_count = max(live.play_count, item.play_count)
                if item.last_opened_at and (
                        live.last_opened_at is None
                        or item.last_opened_at > live.last_opened_at):
                    live.last_opened_at = item.last_opened_at
                self.db.delete_item(item.path)
                self.items.remove(item)
            changed = True
        if changed:
            self._save_all()  # re-packs positions 0..n-1 after removals
        return changed

    def sync_with_folder(self) -> int:
        """Reconcile moved/renamed paths first (see
        _reconcile_dead_paths), then add any video physically in the
        folder that isn't in the list yet; refresh cached sizes of
        existing ones. Never removes or reorders live entries.
        Returns how many new videos were added."""
        if not self.workspace_dir:
            return 0
        self._reconcile_dead_paths()
        existing = {os.path.normcase(os.path.abspath(it.path)) for it in self.items}
        added = 0
        for path in scan_folder_videos(self.workspace_dir):
            norm = os.path.normcase(os.path.abspath(path))
            if norm not in existing:
                self.add_video(path)
                existing.add(norm)
                added += 1
        for item in self.items:
            if os.path.exists(item.path):
                size = file_size(item.path)
                if size and size != item.size_bytes:
                    item.size_bytes = size
                    self.db.set_metadata(item.path, size_bytes=size)
        if added:
            self._save_order()
        return added

    # ---------- playlist operations ----------

    def add_video(self, path: str) -> VideoItem:
        item = VideoItem.from_path(path)
        self.items.append(item)
        self.db.upsert_item(item)
        if self.current_index == -1:
            self.current_index = 0
        self._save_order()
        return item

    def remove_video(self, index: int):
        if 0 <= index < len(self.items):
            removed = self.items.pop(index)
            self.db.delete_item(removed.path)
            self._clamp_current()
            self._save_order()

    def move_video(self, from_index: int, to_index: int):
        if 0 <= from_index < len(self.items) and 0 <= to_index < len(self.items):
            item = self.items.pop(from_index)
            self.items.insert(to_index, item)
            if self.current_index == from_index:
                self.current_index = to_index
            self._save_order()

    # ---------- sorting ----------

    def sort_by(self, key: str):
        """Reorder items by `key` (see SORT_* constants). 'manual' is
        a no-op — the user's drag order is already saved."""
        if key == SORT_MANUAL or key not in SORT_KEYS:
            return
        current_item = self.get_current()

        if key == SORT_NAME:
            order = sorted(range(len(self.items)),
                           key=lambda i: natural_key(self.items[i].title))
        elif key == SORT_NUMBER:
            def num_key(item: VideoItem):
                n = extract_number(item.title)
                return (0, n) if n is not None else (1, 0)
            order = sorted(range(len(self.items)),
                           key=lambda i: num_key(self.items[i]))
        elif key == SORT_HASH_NUMBER:
            # Sort by the number right after '#' (e.g. "#28" -> 28);
            # titles without '#' keep relative order and go last.
            def hash_key(item: VideoItem):
                n = extract_hash_number(item.title)
                return (0, n) if n is not None else (1, 0)
            order = sorted(range(len(self.items)),
                           key=lambda i: hash_key(self.items[i]))
        elif key == SORT_SIZE:
            order = sorted(range(len(self.items)),
                           key=lambda i: self.items[i].size_bytes,
                           reverse=True)
        elif key == SORT_ADDED_AT:
            order = sorted(range(len(self.items)),
                           key=lambda i: self.items[i].added_at)
        else:  # SORT_LAST_OPENED — most recent first, never-played last
            order = sorted(range(len(self.items)),
                           key=lambda i: (self.items[i].last_opened_at is not None,
                                          self.items[i].last_opened_at or 0.0),
                           reverse=True)

        self.items = [self.items[i] for i in order]
        if current_item is not None:
            try:
                self.current_index = self.items.index(current_item)
            except ValueError:
                self.current_index = -1
        self._save_order()

    # ---------- current item ----------

    def get_current(self) -> Optional[VideoItem]:
        if 0 <= self.current_index < len(self.items):
            return self.items[self.current_index]
        return None

    def set_current(self, index: int):
        if 0 <= index < len(self.items):
            self.current_index = index
            self.db.set_meta("current_index", str(index))

    # ---------- watched tracking (YouTube-style, DB only) ----------

    def record_opened(self, index: int):
        """Opening a video = watched (like YouTube starting a video),
        stamped with the play time and an incremented play counter."""
        if not (0 <= index < len(self.items)):
            return
        item = self.items[index]
        now = time.time()
        item.watched = True
        item.last_opened_at = now
        item.play_count += 1
        self.db.record_play(item.path, now)
        self.current_index = index
        self.db.set_meta("current_index", str(index))

    def mark_watched(self, index: int) -> bool:
        if not (0 <= index < len(self.items)):
            return False
        item = self.items[index]
        item.watched = True
        self.db.set_watched(item.path, True)
        return True

    def mark_unwatched(self, index: int) -> bool:
        if not (0 <= index < len(self.items)):
            return False
        item = self.items[index]
        item.watched = False
        self.db.set_watched(item.path, False)
        return True

    def set_duration(self, path: str, seconds: float):
        """Called by the background prober once ffprobe reports a
        duration (also cached in the DB, so it's a one-time cost)."""
        for item in self.items:
            if item.path == path:
                item.duration_sec = seconds
                self.db.set_metadata(path, duration_sec=seconds)
                return

    def items_needing_duration(self) -> List[VideoItem]:
        """Videos with unknown duration whose file still exists —
        the queue for the background ffprobe thread."""
        return [it for it in self.items
                if it.duration_sec <= 0 and os.path.isfile(it.path)]

    # ---------- YouTube playlist order ----------

    def apply_playlist_order(self, ordered_paths: List[str]) -> int:
        """Put `ordered_paths` (playlist order) at the front of the
        list; everything unmatched keeps its relative order after.
        Returns how many items were matched & placed."""
        by_path = {it.path: it for it in self.items}
        placed, matched = [], 0
        for path in ordered_paths:
            item = by_path.pop(path, None)
            if item is not None and item not in placed:
                placed.append(item)
                matched += 1
        remaining = [it for it in self.items if it not in placed]
        self.items = placed + remaining
        self._save_order()
        return matched

    # ---------- stats (summary bar) ----------

    def stats(self) -> dict:
        total = len(self.items)
        watched = sum(1 for it in self.items if it.watched)
        total_duration = sum(it.duration_sec for it in self.items)
        watched_duration = sum(it.duration_sec for it in self.items if it.watched)
        unknown = sum(1 for it in self.items if it.duration_sec <= 0)
        return {
            "total": total,
            "watched": watched,
            "queued": total - watched,
            "total_duration": total_duration,
            "watched_duration": watched_duration,
            "remaining_duration": max(total_duration - watched_duration, 0.0),
            "unknown_duration": unknown,
        }

    def close(self):
        self.db.close()
