#ifndef LAN_OTA_PLUGIN_H
#define LAN_OTA_PLUGIN_H

#include <atomic>
#include <display/core/Plugin.h>

// Receives display firmware over the local network (ArduinoOTA / espota), so a custom build can be
// flashed without USB: `GM_OTA_PASSWORD=... pio run -e display -t upload --upload-port <ip>`.
// Only built when GM_OTA_PASSWORD is set at build time (scripts/lan_ota_pre.py defines
// LAN_OTA_PASSWORD_MD5); release builds do not contain it. See docs/lan-ota.md.
class LanOtaPlugin : public Plugin {
  public:
    void setup(Controller *controller, PluginManager *pluginManager) override;
    void loop() override;

  private:
    Controller *controller = nullptr;
    std::atomic<bool> startPending{false};
    bool started = false;
};

#endif // LAN_OTA_PLUGIN_H
