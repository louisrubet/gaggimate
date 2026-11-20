# PlatformIO post hook: passes GM_OTA_PASSWORD to espota when uploading over the network
# (`pio run -e display -t upload --upload-port <ip or name.local>` switches the upload to espota).
# The platform builder does not forward upload_flags to espota, hence this hook. See docs/lan-ota.md.
import os

Import("env")  # noqa: F821  (provided by SCons)

password = os.environ.get("GM_OTA_PASSWORD", "")
if env.subst("$UPLOADER").endswith("espota.py"):  # noqa: F821
    if not password:
        print("lan_ota: GM_OTA_PASSWORD is not set, the device will refuse the upload")
    else:
        env.Append(UPLOADERFLAGS=["--auth=" + password])  # noqa: F821
