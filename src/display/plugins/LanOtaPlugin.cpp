#if defined(LAN_OTA_PASSWORD_MD5) && !defined(GAGGIMATE_SIM)

#include "LanOtaPlugin.h"

#include <ArduinoOTA.h>
#include <display/core/Controller.h>
#include <display/core/PluginManager.h>
#include <display/core/constants.h>

namespace {
constexpr const char *LOG_TAG = "LanOtaPlugin";
}

void LanOtaPlugin::setup(Controller *controller, PluginManager *pluginManager) {
    this->controller = controller;
    // Station mode only: in access point mode the display is being set up, not updated.
    pluginManager->on("controller:wifi:connect", [this](Event const &event) {
        if (event.getInt("AP") == 0)
            startPending = true;
    });
}

void LanOtaPlugin::loop() {
    if (startPending && !started) {
        ArduinoOTA.setPasswordHash(LAN_OTA_PASSWORD_MD5);
        ArduinoOTA.setMdnsEnabled(false); // mDNSPlugin owns the mDNS responder
        ArduinoOTA.onStart([]() { ESP_LOGW(LOG_TAG, "LAN OTA started, display frozen until restart"); });
        ArduinoOTA.onEnd([]() { ESP_LOGI(LOG_TAG, "LAN OTA done, restarting"); });
        ArduinoOTA.onError([](ota_error_t error) { ESP_LOGE(LOG_TAG, "LAN OTA failed: error %u", error); });
        ArduinoOTA.begin();
        started = true;
        ESP_LOGI(LOG_TAG, "LAN OTA listening on UDP 3232, accepted in standby only");
    }
    // The transfer blocks this task until the restart: only answer while the machine is in standby, so an
    // update never interrupts a shot or steam. Out of standby, espota gets no reply and gives up.
    if (started && controller->getMode() == MODE_STANDBY)
        ArduinoOTA.handle();
}

#endif // LAN_OTA_PASSWORD_MD5 && !GAGGIMATE_SIM
