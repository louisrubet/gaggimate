# PlatformIO pre-build hook: compiles the LAN OTA receiver (LanOtaPlugin) into the display firmware when a
# password is available (GM_OTA_PASSWORD, or ~/.config/gaggimate/ota-password, see lan_ota_password.py).
# Only the MD5 of the password goes into the image (ArduinoOTA's auth scheme).
# Without a password the build is identical to a release build. See docs/lan-ota.md.
import hashlib
import sys

Import("env")  # noqa: F821  (provided by SCons)

sys.path.insert(0, env.subst("$PROJECT_DIR/scripts"))  # noqa: F821
from lan_ota_password import lan_ota_password  # noqa: E402

password = lan_ota_password()
if password:
    digest = hashlib.md5(password.encode("utf-8")).hexdigest()
    env.Append(CPPDEFINES=[("LAN_OTA_PASSWORD_MD5", env.StringifyMacro(digest))])  # noqa: F821
    print("lan_ota: LAN OTA receiver enabled")
