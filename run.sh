#!/usr/bin/env bash
# ============================================================
# Timeline Videos — Linux launcher (Debian/Ubuntu and friends)
#
# Usage:  ./run.sh
#
# Creates the local .venv on first run (with system PyQt5),
# installs whatever's missing, then starts the app.
# ============================================================
set -e
cd "$(dirname "$0")"

VENV=".venv"

if [ ! -x "$VENV/bin/python" ]; then
    echo "=== First run: creating virtualenv ==="
    python3 -m venv --system-site-packages "$VENV"
fi

# Install missing deps quietly (idempotent, fast when already present)
"$VENV/bin/pip" install --quiet --upgrade pip > /dev/null 2>&1 || true
"$VENV/bin/pip" install --quiet -r requirements.txt > /dev/null 2>&1 || true

exec "$VENV/bin/python" main.py "$@"
