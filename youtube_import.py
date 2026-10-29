"""
youtube_import.py
-----------------
Fetches a YouTube playlist as flat JSON (via yt-dlp, no API key, no
downloads) and matches its entries against the local video files by
title, so the local playlist can be reordered to mirror YouTube's
exact order.

    fetch_playlist(url)  -> [{"id", "title", "url"}, …]  in playlist order
    match_playlist(entries, files) -> (ordered_paths, unmatched_files, missing_entries)

Matching strategy (in order):
  1. exact match on the normalized title (quality tags like
     "(1080p60)-" and punctuation stripped, Arabic forms unified)
  2. fuzzy match via difflib ratio >= 0.82 against remaining files
Each file is consumed at most once.
"""

import difflib
import os
import re
import unicodedata
from typing import List, Tuple

# Words that appear in downloaded filenames but not in YouTube titles
_NOISE_WORDS = re.compile(
    r"\b(1080p60|1080p|720p|480p|2160p|4k|hd|fhd|sd|x264|x265|h264|h265|hevc|"
    r"aac|ac3|web-dl|webrip|bluray|dvdrip|hdtv|remux)\b",
    re.IGNORECASE,
)
_PUNCT = re.compile(r"[^\w\s]", re.UNICODE)
_SPACES = re.compile(r"\s+")

# Arabic letter-form unification so "أ/إ/آ/ٱ" == "ا" and "ة" == "ه",
# "ى" == "ي" — downloaded names and YouTube titles differ on these a lot.
_ARABIC_MAP = str.maketrans({
    "أ": "ا", "إ": "ا", "آ": "ا", "ٱ": "ا",
    "ة": "ه", "ى": "ي", "ٮ": "ب",
})

FUZZY_THRESHOLD = 0.82


def fetch_playlist(url: str) -> List[dict]:
    """Playlist entries in order, as {"id","title","url"} dicts.
    Raises whatever yt-dlp raises (network errors, private playlist…);
    the caller turns that into a friendly message.
    Imports yt-dlp lazily — it's slow to import and often missing."""
    try:
        import yt_dlp  # noqa: PLC0415 — deliberate lazy import (slow module)
    except ImportError as e:
        raise RuntimeError("yt-dlp-missing") from e

    options = {
        "extract_flat": "in_playlist",
        "skip_download": True,
        "quiet": True,
        "no_warnings": True,
        "ignoreerrors": True,
    }
    with yt_dlp.YoutubeDL(options) as ydl:
        info = ydl.extract_info(url, download=False)

    if not info:
        return []
    entries = []
    for entry in (info.get("entries") or []):
        if not entry:
            continue
        vid_id = entry.get("id") or ""
        entries.append({
            "id": vid_id,
            "title": entry.get("title") or "",
            "url": entry.get("url") or (f"https://www.youtube.com/watch?v={vid_id}" if vid_id else ""),
        })
    return entries


def normalize_title(text: str) -> str:
    """Aggressive normalization for matching: lowercase, strip the
    extension, quality/codec noise words, punctuation, Arabic form
    differences and whitespace."""
    text = os.path.splitext(text)[0].lower()
    text = text.translate(_ARABIC_MAP)
    text = unicodedata.normalize("NFKC", text)
    text = _NOISE_WORDS.sub(" ", text)
    text = _PUNCT.sub(" ", text)
    text = _SPACES.sub(" ", text).strip()
    return text


def match_playlist(entries: List[dict], file_paths: List[str]) -> Tuple[List[str], List[str], List[dict]]:
    """Match playlist entries to local files.

    Returns (ordered_paths, unmatched_files, missing_entries):
      • ordered_paths     — local paths, in the playlist's exact order
      • unmatched_files   — local files with no playlist counterpart
                            (caller keeps them after the ordered ones)
      • missing_entries   — playlist entries with no local file
    """
    norm_to_files = {}
    for path in file_paths:
        key = normalize_title(os.path.basename(path))
        norm_to_files.setdefault(key, []).append(path)

    used = set()
    ordered: List[str] = []
    missing: List[dict] = []

    # pass 1 — exact normalized match
    pending_entries = []
    for entry in entries:
        key = normalize_title(entry.get("title", ""))
        candidates = [p for p in norm_to_files.get(key, []) if p not in used]
        if candidates:
            chosen = candidates[0]
            used.add(chosen)
            ordered.append(chosen)
        else:
            pending_entries.append(entry)

    # pass 2 — fuzzy match against the files still unclaimed
    free_norms = [
        (normalize_title(os.path.basename(p)), p)
        for p in file_paths if p not in used
    ]
    for entry in pending_entries:
        key = normalize_title(entry.get("title", ""))
        best_path, best_ratio = None, 0.0
        for norm, path in free_norms:
            if path in used:
                continue
            ratio = difflib.SequenceMatcher(None, key, norm).ratio()
            if ratio > best_ratio:
                best_path, best_ratio = path, ratio
        if best_path is not None and best_ratio >= FUZZY_THRESHOLD:
            used.add(best_path)
            ordered.append(best_path)
        else:
            missing.append(entry)

    unmatched_files = [p for p in file_paths if p not in used]
    return ordered, unmatched_files, missing
