# video_player.spec
#
# PyInstaller build spec for Simple Video Player.
# Must be run ON WINDOWS (PyInstaller does not cross-compile).
#
# No external engine to bundle anymore — the app just launches
# whatever player Windows already has associated with video files,
# so this spec is now a plain PyInstaller build.
#
# Paths below are built from SPECPATH (a variable PyInstaller injects
# automatically = the folder this .spec file lives in) instead of bare
# relative strings, so the build works no matter what directory
# pyinstaller was invoked from.

import os

block_cipher = None

ASSETS_DIR = os.path.join(SPECPATH, "assets")
ICON_PATH = os.path.join(ASSETS_DIR, "logo.ico")

# yt-dlp ships data files & hidden modules PyInstaller can't trace on
# its own (it's imported lazily inside youtube_import.py) — collect
# the whole package. Optional: if yt-dlp isn't installed the app still
# builds (the YouTube import dialog then asks the user to install it).
_ytdlp_datas, _ytdlp_binaries, _ytdlp_hidden = [], [], []
try:
    from PyInstaller.utils.hooks import collect_all
    _ytdlp_datas, _ytdlp_binaries, _ytdlp_hidden = collect_all("yt_dlp")
except Exception as exc:  # pragma: no cover - build-time only
    print(f"NOTE: yt-dlp not bundled ({exc}) — YouTube import will be "
          f"unavailable in this build.")

if not os.path.isdir(ASSETS_DIR):
    raise SystemExit(
        f"\nERROR: couldn't find the assets folder at:\n  {ASSETS_DIR}\n"
        f"Make sure 'assets' (with logo.ico and icon_mark.png inside) sits "
        f"in the same folder as this .spec file.\n"
    )

a = Analysis(
    ["main.py"],
    pathex=[],
    binaries=list(_ytdlp_binaries),
    datas=[(ASSETS_DIR, "assets")] + list(_ytdlp_datas),
    hiddenimports=list(_ytdlp_hidden),
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    cipher=block_cipher,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name="TimelineVideos",
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,       # windowed app, no terminal popup
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=ICON_PATH,
)
