"""
youtube_dialog.py
-----------------
Dialog for "reorder my local playlist exactly like a YouTube
playlist": paste the playlist URL → fetch its JSON order via yt-dlp
(no API key, no downloads) → match entries against the local files
by title → preview the summary → apply the order.

Network + matching run on a background thread so the UI never
freezes, even on huge playlists.
"""

from typing import List, Optional

from PyQt5.QtCore import Qt, QThread, pyqtSignal
from PyQt5.QtWidgets import (
    QDialog, QHBoxLayout, QLabel, QLineEdit, QPlainTextEdit,
    QPushButton, QVBoxLayout,
)

import youtube_import
from i18n import tr


class _FetchWorker(QThread):
    """Fetches the playlist AND matches it against local files off
    the GUI thread. Emits (ordered_paths, unmatched_files,
    missing_entries) or raises into .error."""

    finished_ok = pyqtSignal(object, object, object)
    failed = pyqtSignal(str)   # "ytdlp" | "<error text>"

    def __init__(self, url: str, local_paths: List[str], parent=None):
        super().__init__(parent)
        self._url = url
        self._local = local_paths

    def run(self):
        try:
            entries = youtube_import.fetch_playlist(self._url)
        except RuntimeError as exc:
            if str(exc) == "yt-dlp-missing":
                self.failed.emit("ytdlp")
            else:
                self.failed.emit(str(exc))
            return
        except Exception as exc:  # network, bad URL, private playlist…
            self.failed.emit(str(exc))
            return
        ordered, unmatched, missing = youtube_import.match_playlist(
            entries, self._local
        )
        self.finished_ok.emit(ordered, unmatched, missing)


class YoutubePlaylistDialog(QDialog):
    """URL input → fetch → summary preview → Apply."""

    def __init__(self, local_paths: List[str], parent=None):
        super().__init__(parent)
        self.setWindowTitle(tr("yt.title"))
        self.setMinimumWidth(560)
        self.ordered_paths: Optional[List[str]] = None
        self._worker: Optional[_FetchWorker] = None
        self._local = local_paths

        root = QVBoxLayout(self)
        root.setContentsMargins(20, 20, 20, 16)
        root.setSpacing(12)

        url_row = QHBoxLayout()
        url_label = QLabel(tr("yt.url_label"))
        self.url_edit = QLineEdit()
        self.url_edit.setPlaceholderText(tr("yt.url_placeholder"))
        self.fetch_btn = QPushButton(tr("yt.fetch"))
        self.fetch_btn.setProperty("accent", True)
        url_row.addWidget(url_label)
        url_row.addWidget(self.url_edit, stretch=1)
        url_row.addWidget(self.fetch_btn)
        root.addLayout(url_row)

        self.status = QLabel("")
        self.status.setWordWrap(True)
        self.status.setStyleSheet("color: #8b90b5;")
        root.addWidget(self.status)

        self.preview = QPlainTextEdit()
        self.preview.setReadOnly(True)
        self.preview.setMinimumHeight(170)
        root.addWidget(self.preview, stretch=1)

        hint = QLabel(tr("yt.hint"))
        hint.setWordWrap(True)
        hint.setStyleSheet("color: #8b90b5; font-size: 11px;")
        root.addWidget(hint)

        buttons = QHBoxLayout()
        buttons.addStretch()
        self.cancel_btn = QPushButton(tr("yt.cancel"))
        self.apply_btn = QPushButton(tr("yt.apply"))
        self.apply_btn.setProperty("accent", True)
        self.apply_btn.setEnabled(False)
        buttons.addWidget(self.cancel_btn)
        buttons.addWidget(self.apply_btn)
        root.addLayout(buttons)

        self.fetch_btn.clicked.connect(self._on_fetch)
        self.url_edit.returnPressed.connect(self._on_fetch)
        self.cancel_btn.clicked.connect(self.reject)
        self.apply_btn.clicked.connect(self._on_apply)

    # ---------- behavior ----------

    def _on_fetch(self):
        url = self.url_edit.text().strip()
        if not url:
            return
        self.fetch_btn.setEnabled(False)
        self.apply_btn.setEnabled(False)
        self.status.setText(tr("yt.fetching"))
        self.preview.setPlainText("")

        self._worker = _FetchWorker(url, self._local, self)
        self._worker.finished_ok.connect(self._on_fetched)
        self._worker.failed.connect(self._on_failed)
        self._worker.start()

    def _on_failed(self, kind: str):
        self.fetch_btn.setEnabled(True)
        if kind == "ytdlp":
            self.status.setText(tr("yt.need_ytdlp"))
        else:
            self.status.setText(tr("yt.error", err=kind))

    def _on_fetched(self, ordered, unmatched, missing):
        self.fetch_btn.setEnabled(True)
        if not ordered and not missing:
            # nothing matched AND nothing missing → empty/bad link
            self.status.setText(tr("yt.empty"))
            return
        self.ordered_paths = list(ordered)
        lines = [
            tr("yt.matched", n=len(ordered)),
            tr("yt.unmatched_files", n=len(unmatched)),
            tr("yt.missing_entries", n=len(missing)),
            "",
        ]
        if missing:
            lines.append("— " + tr("yt.missing_entries", n=len(missing)) + ":")
            lines += [f"   • {e.get('title', '?')}" for e in missing[:25]]
            if len(missing) > 25:
                lines.append(f"   … +{len(missing) - 25}")
        self.preview.setPlainText("\n".join(lines))
        self.status.setText("")
        self.apply_btn.setEnabled(True)

    def _on_apply(self):
        if self.ordered_paths:
            self.accept()
