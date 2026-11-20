"""LAN OTA password lookup shared by lan_ota_pre.py and lan_ota_post.py. See docs/lan-ota.md.

Order: GM_OTA_PASSWORD environment variable, then the file named by GM_OTA_PASSWORD_FILE, by default
~/.config/gaggimate/ota-password (first line). Empty when neither is set: no LAN OTA receiver is built.
"""
import os
import stat
from pathlib import Path

DEFAULT_FILE = Path.home() / ".config" / "gaggimate" / "ota-password"


def lan_ota_password() -> str:
    password = os.environ.get("GM_OTA_PASSWORD", "")
    if password:
        return password
    path = Path(os.environ.get("GM_OTA_PASSWORD_FILE", DEFAULT_FILE)).expanduser()
    if not path.is_file():
        return ""
    if path.stat().st_mode & (stat.S_IRWXG | stat.S_IRWXO):
        print(f"lan_ota: warning: {path} is readable by other users, run: chmod 600 {path}")
    lines = path.read_text(encoding="utf-8").splitlines()
    return lines[0].strip() if lines else ""
