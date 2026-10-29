"""
resources.py
-------------
Finds bundled files (icons, images) regardless of whether the app is
running from source or as a PyInstaller-built .exe. When frozen,
PyInstaller extracts data files under sys._MEIPASS at runtime.
"""

import os
import sys


def resource_path(*relative_parts: str) -> str:
    if getattr(sys, "frozen", False):
        base_path = sys._MEIPASS
    else:
        base_path = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(base_path, *relative_parts)


ICON_ICO = resource_path("assets", "logo.ico")
ICON_MARK_PNG = resource_path("assets", "icon_mark.png")
