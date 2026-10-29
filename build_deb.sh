#!/usr/bin/env bash
# ============================================================
# Timeline Videos — Debian package (.deb) builder
#
# Produces:  dist/deb/timeline-videos_<version>-1_all.deb
#
# The package installs:
#   /opt/timeline-videos/            app code + assets (root-owned)
#   /usr/bin/timeline-videos         launcher
#   /usr/share/applications/         menu entry
#   /usr/share/pixmaps/              icon
#   /usr/share/doc/timeline-videos/  README + copyright
#
# Install with:   sudo dpkg -i dist/deb/*.deb
# (or:            sudo apt install ./dist/deb/*.deb  → pulls Depends)
# ============================================================
set -e
cd "$(dirname "$0")"

PKG="timeline-videos"

# version comes straight from main_window.py (single source of truth)
VERSION="$(sed -n 's/^APP_VERSION = "\(.*\)"/\1/p' main_window.py)"
[ -n "$VERSION" ] || { echo "ERROR: couldn't read APP_VERSION"; exit 1; }
DEB_VERSION="${VERSION}-1"
STAGE="build/deb/${PKG}"
OUT_DIR="dist/deb"

echo "=== Building ${PKG} ${DEB_VERSION} (Architecture: all) ==="

# ---------- bundled pure-python deps ----------
# Debian doesn't package ftfy, so ship it (+ its only dep, wcwidth)
# inside the app dir; main.py puts ./vendor on sys.path when present.
stage_vendor() {
    local vendor="$STAGE/opt/${PKG}/vendor"
    mkdir -p "$vendor"
    local py=".venv/bin/python"
    [ -x "$py" ] || py="python3"

    if "$py" -c "import ftfy, wcwidth" 2>/dev/null; then
        local sp
        sp="$("$py" -c 'import ftfy, os; print(os.path.dirname(ftfy.__file__))')"
        cp -r "$sp" "$vendor/"
        sp="$("$py" -c 'import wcwidth, os; print(os.path.dirname(wcwidth.__file__))')"
        cp -r "$sp" "$vendor/"
        find "$vendor" -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null || true
        echo "→ vendor: ftfy + wcwidth copied from local environment"
        return 0
    fi

    local tmp
    tmp="$(mktemp -d)"
    if python3 -m pip download --no-deps --only-binary=:all: \
            -d "$tmp" ftfy wcwidth >/dev/null 2>&1; then
        for whl in "$tmp"/*.whl; do
            python3 -c "import sys, zipfile; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" \
                "$whl" "$vendor"
        done
        rm -rf "$tmp"
        echo "→ vendor: ftfy + wcwidth wheels downloaded"
        return 0
    fi
    rm -rf "$tmp"
    echo "WARNING: couldn't bundle ftfy — the app still works, but"
    echo "         automatic garbled-title (mojibake) repair will be limited."
}

# ---------- stage the tree ----------
rm -rf "$STAGE"
mkdir -p "$STAGE/DEBIAN" \
         "$STAGE/opt/${PKG}" \
         "$STAGE/usr/bin" \
         "$STAGE/usr/share/applications" \
         "$STAGE/usr/share/pixmaps" \
         "$STAGE/usr/share/doc/${PKG}"

# application code (plain .py files only — no caches, no venvs)
for f in main.py main_window.py playlist_manager.py video_db.py \
         media_probe.py youtube_import.py youtube_dialog.py \
         splash.py i18n.py theme.py external_player.py resources.py; do
    [ -f "$f" ] || { echo "ERROR: missing source file $f"; exit 1; }
    cp "$f" "$STAGE/opt/${PKG}/"
done
cp -r assets "$STAGE/opt/${PKG}/assets"
stage_vendor

# documentation (Debian policy: copyright file required)
cp README.md "$STAGE/usr/share/doc/${PKG}/README.md"
cat > "$STAGE/usr/share/doc/${PKG}/copyright" <<'EOF'
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: Timeline Videos
Source: https://github.com/RaGAEIDOS/TimelineVideos

Files: *
Copyright: 2025-2026 Ragaey
License: MIT
 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED.
EOF

# ---------- launcher ----------
cat > "$STAGE/usr/bin/${PKG}" <<'EOF'
#!/bin/sh
# Timeline Videos launcher (installed via .deb)
exec python3 /opt/timeline-videos/main.py "$@"
EOF
chmod 755 "$STAGE/usr/bin/${PKG}"

# ---------- desktop entry + icon ----------
cat > "$STAGE/usr/share/applications/${PKG}.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Timeline Videos
Name[ar]=تايم لاين فيديوهات
GenericName=Video Playlist Manager
GenericName[ar]=مدير قوائم الفيديو
Comment=Track watched videos in a local database — YouTube-style
Comment[ar]=تابع الفيديوهات المشاهدة في قاعدة بيانات محلية — زي يوتيوب
Exec=${PKG}
Icon=${PKG}
Terminal=false
Categories=AudioVideo;Video;
Keywords=video;playlist;watched;youtube;
StartupWMClass=main.py
EOF
cp assets/icon_mark.png "$STAGE/usr/share/pixmaps/${PKG}.png"

# ---------- control ----------
INSTALLED_SIZE="$(du -sk "$STAGE" | cut -f1)"
cat > "$STAGE/DEBIAN/control" <<EOF
Package: ${PKG}
Version: ${DEB_VERSION}
Section: video
Priority: optional
Architecture: all
Installed-Size: ${INSTALLED_SIZE}
Depends: python3 (>= 3.10), python3-pyqt5
Recommends: yt-dlp, ffmpeg
Maintainer: Ragaey <ragaeidos@users.noreply.github.com>
Homepage: https://github.com/RaGAEIDOS/TimelineVideos
Description: Bilingual (Arabic/English) video playlist manager
 Timeline Videos scans a folder of videos, sorts them (episode
 number, name, size, date, YouTube playlist order) and tracks what
 you watched in a small per-folder SQLite database — YouTube-style,
 without ever moving the files. Playback is handed to the system's
 default player (mpv, VLC, ...). Subtitle files (SRT/VTT) are
 listed next to each video; durations are read with ffprobe.
 .
 The interface is Arabic (RTL) and English, switchable at runtime.
EOF
chmod 755 "$STAGE/DEBIAN"
chmod 644 "$STAGE/DEBIAN/control"

# ---------- build ----------
mkdir -p "$OUT_DIR"
OUT_DEB="$OUT_DIR/${PKG}_${DEB_VERSION}_all.deb"
dpkg-deb --build --root-owner-group "$STAGE" "$OUT_DEB"

echo
echo "Done → $OUT_DEB"
echo "Install:   sudo apt install ./$OUT_DEB"
echo "           (or: sudo dpkg -i $OUT_DEB)"
