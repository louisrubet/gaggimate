# PlatformIO post hook: passes the LAN OTA password (see lan_ota_password.py) to espota when uploading over
# the network (`pio run -e display -t upload --upload-port <ip or name.local>` switches the upload to espota).
# The platform builder does not forward upload_flags to espota, hence this hook. See docs/lan-ota.md.
import sys

Import("env")  # noqa: F821  (provided by SCons)

sys.path.insert(0, env.subst("$PROJECT_DIR/scripts"))  # noqa: F821
from lan_ota_password import lan_ota_password  # noqa: E402

if env.subst("$UPLOADER").endswith("espota.py"):  # noqa: F821
    password = lan_ota_password()
    if not password:
        print("lan_ota: no LAN OTA password (GM_OTA_PASSWORD or ~/.config/gaggimate/ota-password), upload refused")
    else:
        env.Append(UPLOADERFLAGS=["--auth=" + password])  # noqa: F821
