#include "StandbyHaikuPlugin.h"

#include "HaikuOfTheDay.h"
#include <display/core/Controller.h>
#include <display/core/PluginManager.h>
#include <display/core/Settings.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>

namespace {
constexpr const char *LOG_TAG = "StandbyHaiku";
constexpr time_t MIN_VALID_TIME = 1700000000;            // before this the clock has not been synced yet
constexpr unsigned long RETRY_MS = 60UL * 60UL * 1000UL; // after a failed download
constexpr long SECONDS_PER_DAY = 24L * 60L * 60L;
constexpr uint32_t FETCH_STACK = 8192; // TLS handshake
} // namespace

void StandbyHaikuPlugin::setup(Controller *controller, PluginManager *pluginManager) {
    this->pluginManager = pluginManager;
    settings = &controller->getSettings();
    // A new station connection may bring the network back: retry without waiting for RETRY_MS.
    pluginManager->on("controller:wifi:connect", [this](Event const &event) {
        if (event.getInt("AP") == 0)
            lastFetchFailed = false;
    });
}

// Polled rather than driven by "settings:changed": the first loop runs once every listener is registered,
// and a change from the web UI applies without a restart.
void StandbyHaikuPlugin::loop() {
    StandbyHaikuDisplay display = StandbyHaikuDisplay::Off;
    if (settings->isStandbyHaikuActive())
        display = settings->isStandbyHaikuOnline() ? StandbyHaikuDisplay::OfTheDay : StandbyHaikuDisplay::BuiltIn;
    if (display == StandbyHaikuDisplay::OfTheDay)
        scheduleFetch();

    const int value = static_cast<int>(display);
    if (value == published)
        return;
    published = value;
    pluginManager->trigger("standbyHaiku:display:change", "value", value);
}

void StandbyHaikuPlugin::scheduleFetch() {
    if (fetching)
        return;
    const time_t now = time(nullptr);
    if (now < MIN_VALID_TIME)
        return;
    const long day = static_cast<long>(now / SECONDS_PER_DAY);
    if (day == fetchedDay)
        return;
    if (lastFetchFailed && millis() - lastFetchMs < RETRY_MS)
        return;

    lastFetchMs = millis();
#ifdef GAGGIMATE_SIM
    runFetch(day); // the simulator runs no FreeRTOS task; one blocking request a day is fine there
#else
    pendingDay = day;
    fetching = true;
    if (xTaskCreatePinnedToCore(fetchTask, "Haiku::fetch", FETCH_STACK, this, 1, nullptr, 0) != pdPASS) {
        fetching = false;
        lastFetchFailed = true;
    }
#endif
}

void StandbyHaikuPlugin::fetchTask(void *arg) {
    auto *self = static_cast<StandbyHaikuPlugin *>(arg);
    self->runFetch(self->pendingDay);
    self->fetching = false;
    vTaskDelete(nullptr);
}

void StandbyHaikuPlugin::runFetch(long day) {
    HaikuOfTheDayEntry entry;
    if (HaikuOfTheDay::fetch(day, entry)) {
        HaikuOfTheDay::publish(entry);
        fetchedDay = day;
        lastFetchFailed = false;
        ESP_LOGI(LOG_TAG, "Haiku of the day %ld fetched", day);
    } else {
        lastFetchFailed = true;
        ESP_LOGW(LOG_TAG, "Haiku of the day %ld: download failed, retrying in 1 h", day);
    }
}
