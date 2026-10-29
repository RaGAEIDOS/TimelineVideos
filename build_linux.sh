#!/usr/bin/env bash
# ============================================================
# Timeline Videos — Linux (PyInstaller) builder
#
# Produces:  dist/TimelineVideos   (single self-contained binary)
#
# Run on the machine that will run the app:
#   ./build_linux.sh
# ============================================================
set -e
cd "$(dirname "$0")"

echo "=== Timeline Videos - Linux builder ==="

if [ ! -f assets/logo.ico ] || [ ! -f assets/icon_mark.png ]; then
    echo "ERROR: assets/ (logo.ico + icon_mark.png) must sit next to this script."
    exit 1
fi

VENV=".venv"
if [ ! -x "$VENV/bin/python" ]; then
    python3 -m venv --system-site-packages "$VENV"
fi

"$VENV/bin/pip" install --quiet --upgrade pip
"$VENV/bin/pip" install --quiet -r requirements.txt
"$VENV/bin/pip" install --quiet pyinstaller

"$VENV/bin/pyinstaller" --noconfirm --clean --onefile --windowed \
    --name TimelineVideos \
    --add-data "assets:assets" \
    main.py

echo
echo "Done → dist/TimelineVideos"
echo "Run it with:  ./dist/TimelineVideos"
