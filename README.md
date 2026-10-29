# Timeline Videos

A lightweight, **bilingual (Arabic/English) playlist manager** built
with PyQt5, running on **Linux (Debian/Ubuntu)** and Windows. It
doesn't play video itself — double-clicking a video (or "Play
Selected") launches it in whatever the OS has set as the default
player (MPV, VLC…), exactly like double-clicking it in the file
manager.

Watched status is tracked **YouTube-style in a small SQLite database
stored next to the videos** — files are never moved around.

## Features

- **File > Open Folder** — opens a folder as its own separate
  workspace. Its playlist, order, and watched state are all stored
  inside that folder itself (a hidden `.timeline_videos.db`), so
  reopening it later restores everything exactly as you left it, and
  different folders never mix. The last folder reopens automatically
  on startup.
- **Arabic / English** — one-click switch from the header button or
  the Language menu; Arabic flips the whole layout to RTL. The
  choice is remembered.
- **Modern dark theme** — layered cards, accent colors, rounded
  corners, YouTube-style progress bar.
- **Loading screen** — heavy startup work (UI imports, database,
  folder scan, tool checks) runs behind a splash screen, so the app
  never freezes at launch.
- **Summary bar** — total videos + total duration, watched count &
  time, queued count & remaining time, and a watched-ratio progress
  bar.
- **Advanced sorting** — by number after `#` (e.g. `…Computer
  Science #28-(1080p24)` sorts by `28`, ignoring the resolution
  tag), by the last number in the title, natural name order, file
  size, date added, last played — plus manual drag-and-drop
  (choosing drag sets the sort back to "Manual").
- **YouTube playlist order** — paste a YouTube playlist link
  (Ctrl+Y): the playlist is fetched as JSON via `yt-dlp`, matched to
  your local files by title, and the list is reordered **exactly
  like the YouTube playlist**. Unmatched files keep their place;
  playlist videos you don't have are listed in the summary.
- **Watched tracking, YouTube-style** — opening a video records it
  as watched, stamps the last-played time and bumps a play counter
  (all in the database; the file stays where it is). The checkbox
  and the right-click menu toggle watched manually anytime.
- **Subtitle files shown per video** — every subtitle file sitting
  next to a video is listed in green on its row: plain names show
  as `SRT` / `VTT`, language-tagged yt-dlp names show as
  `ar.SRT`, `en.VTT`… The video's duration appears on the row too
  (read once with `ffprobe`, then cached in the database).
- **System tray** — closing the window minimizes to the tray;
  right-click the tray icon for Show/Exit.

## Requirements

- Python 3.10+
- `ffprobe` (for durations): `sudo apt install ffmpeg`
- `yt-dlp` (for YouTube playlist import): installed automatically
  into the app's virtualenv by `run.sh`

```bash
pip install -r requirements.txt   # PyQt5, ftfy, yt-dlp
```

## Run (Linux / Debian)

```bash
./run.sh          # creates .venv on first run, then starts the app
```

### Install as a .deb

```bash
./build_deb.sh                        # → dist/deb/timeline-videos_2.1-1_all.deb
sudo apt install ./dist/deb/*.deb      # pulls python3-pyqt5 automatically
timeline-videos                       # launches from your app menu / terminal
```

Notes on the package:
- **Architecture: all** — one file for every Debian/Ubuntu/derivatives
  machine (x86_64, ARM…).
- `ftfy` + `wcwidth` are **bundled inside the package** (Debian doesn't
  ship ftfy), so garbled-title repair works out of the box.
- `ffmpeg` (durations) and `yt-dlp` (YouTube playlist import) are
  auto-installed by apt as *Recommends*; if your distro's yt-dlp is
  too old for YouTube, `pip install -U yt-dlp` gets you the latest.

### Install via your own APT repository (GitHub Pages)

`make_apt_repo.sh` builds a static APT repo in `deb_repo/` that you can
host on GitHub Pages, so users can `apt update && apt install
timeline-videos` straight from your repo:

```bash
./make_apt_repo.sh                      # unsigned build
GPG_KEY=<keyid> ./make_apt_repo.sh      # signed build (+ exports key.gpg)
```

Then push the contents of `deb_repo/` to a `gh-pages` branch and users add:

```bash
echo 'deb [trusted=yes] https://RAGAEIDOS.github.io/TimelineVideos/ ./' | \
    sudo tee /etc/apt/sources.list.d/timeline-videos.list
sudo apt update && sudo apt install timeline-videos
```

(With a GPG-signed build, replace `[trusted=yes]` with
`[signed-by=/usr/share/keyrings/timeline-videos.gpg]` after fetching
`key.gpg`.)

About the **official Debian repositories**: that route means filing an
ITP bug, passing the NEW queue and Debian Policy review, and maintaining
the package long-term — realistically months of process meant for
software with a wide audience. For a personal project the .deb + your
own APT repo (above) gives users the exact same `apt install` experience.

Optional — add it to your application menu (source install):

```bash
cp timeline-videos.desktop ~/.local/share/applications/
update-desktop-database ~/.local/share/applications
```

## Run (Windows)

```bash
python main.py
```

## Project layout

```
video_player/
├── main.py                # entry point: theme + splash + startup worker
├── main_window.py         # PyQt5 UI: header, toolbar, stats bar,
│                          #   playlist, tray, delegate, probe thread
├── playlist_manager.py    # data layer: sorting, watched tracking,
│                          #   stats, legacy JSON migration
├── video_db.py            # SQLite database (stored per folder)
├── media_probe.py         # ffprobe duration + file size
├── youtube_import.py      # yt-dlp playlist JSON + title matching
├── youtube_dialog.py      # playlist import dialog (background fetch)
├── splash.py              # loading screen + startup worker thread
├── i18n.py                # Arabic/English translations + RTL + formats
├── theme.py               # modern dark QSS stylesheet
├── external_player.py     # opens files via xdg-open (Linux) / default app
├── resources.py           # asset paths (source or PyInstaller-frozen)
├── assets/
│   ├── logo.ico           # app / window / tray icon
│   └── icon_mark.png      # header thumbnail
├── run.sh                 # Linux launcher (builds .venv on first run)
├── build_linux.sh         # Linux PyInstaller builder → dist/TimelineVideos
├── build_deb.sh           # Debian package builder → dist/deb/*.deb
├── make_apt_repo.sh       # static APT repo (GitHub Pages) → deb_repo/
├── build_windows.bat      # Windows PyInstaller builder → .exe
├── timeline-videos.desktop# app-menu entry for Linux desktops
├── .gitignore
├── requirements.txt
└── video_player.spec      # PyInstaller build config
```

## Building a standalone binary

- **Linux**: `./build_linux.sh` → `dist/TimelineVideos`
- **Windows**: double-click `build_windows.bat` (must run on
  Windows — PyInstaller can't cross-compile). The exe keeps the
  Timeline Videos icon in Explorer, the taskbar, and the tray.

## How tracking data is stored

- Each opened folder gets a hidden `.timeline_videos.db` (SQLite)
  **inside the folder** — playlist order, watched flags, play
  counts, last-played times, sizes, and durations. Move the folder
  and everything travels with it; delete the file and the folder is
  simply re-scanned fresh.
- The default list (no folder opened) lives in
  `~/.simple_video_player/playlist.db`.
- **Old versions** of the app (which used a `.timeline_videos.json`
  file and moved watched files into a `Watched` subfolder) are
  migrated automatically on first open: the JSON playlist is
  imported into the database and the JSON file is kept untouched as
  a backup. Files that were already moved into `Watched/` stay
  where they are — the app just keeps tracking them.
- **Folders moved between machines** (e.g. `E:\…` on Windows →
  `/mnt/…` on Linux) are handled automatically: rows whose old
  paths no longer exist are matched to the videos actually in the
  folder by file name and re-pointed (or merged into an existing
  row, keeping watched/play history) — no more duplicate entries
  after copying a library to another OS.

## Notes on garbled (mojibake) titles

Some downloaded videos (especially Arabic titles) end up with
filenames mangled by an encoding mismatch. Both the common
Windows-1252 style (fixed by `ftfy`) and the Arabic Windows-1256
style (detected by its signature and reversed) are auto-repaired
the moment a title is loaded. Correct titles are left untouched.
This only fixes the displayed title — the file on disk keeps its
original name.

## Credits

Developed by **رجائي (Ragaey)** — Alexandria, Egypt.
Play. Learn. Track. Grow.
