# PlatformIO pre-build hook: compiles the LAN OTA receiver (LanOtaPlugin) into the display firmware when
# GM_OTA_PASSWORD is set. Only the MD5 of the password goes into the image (ArduinoOTA's auth scheme).
# Without the variable the build is identical to a release build. See docs/lan-ota.md.
import hashlib
import os

Import("env")  # noqa: F821  (provided by SCons)

password = os.environ.get("GM_OTA_PASSWORD", "")
if password:
    digest = hashlib.md5(password.encode("utf-8")).hexdigest()
    env.Append(CPPDEFINES=[("LAN_OTA_PASSWORD_MD5", env.StringifyMacro(digest))])  # noqa: F821
    print("lan_ota: LAN OTA receiver enabled")
