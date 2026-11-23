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
constexpr unsigned long RETRY_MS = 60UL * 60UL * 1000UL; // after a failed download
constexpr uint32_t FETCH_STACK = 8192;                   // TLS handshake
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
    const StandbyHaikuDisplay display =
        settings->isStandbyHaikuActive() ? StandbyHaikuDisplay::OfTheDay : StandbyHaikuDisplay::Off;
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
    const long day = HaikuOfTheDay::today();
    if (day < 0)
        return;
    const auto language = static_cast<StandbyHaikuLanguage>(settings->getStandbyHaikuLanguage() == 1 ? 1 : 0);
    const auto calendar = static_cast<StandbyHaikuCalendar>(settings->getStandbyHaikuCalendar() == 1 ? 1 : 0);
    const uint32_t key = HaikuOfTheDay::requestKey(day, language, calendar);
    if (key != wantedKey) {
        // New day or new settings: the haiku on screen no longer applies, the logo shows until the download.
        wantedKey = key;
        lastFetchFailed = false;
        HaikuOfTheDay::clear();
    }
    if (key == fetchedKey)
        return;
    if (lastFetchFailed && millis() - lastFetchMs < RETRY_MS)
        return;

    lastFetchMs = millis();
    pendingDay = day;
    pendingLanguage = static_cast<int>(language);
    pendingCalendar = static_cast<int>(calendar);
#ifdef GAGGIMATE_SIM
    runFetch(); // the simulator runs no FreeRTOS task; one blocking request a day is fine there
#else
    fetching = true;
    if (xTaskCreatePinnedToCore(fetchTask, "Haiku::fetch", FETCH_STACK, this, 1, nullptr, 0) != pdPASS) {
        fetching = false;
        lastFetchFailed = true;
    }
#endif
}

void StandbyHaikuPlugin::fetchTask(void *arg) {
    auto *self = static_cast<StandbyHaikuPlugin *>(arg);
    self->runFetch();
    self->fetching = false;
    vTaskDelete(nullptr);
}

void StandbyHaikuPlugin::runFetch() {
    const long day = pendingDay;
    const auto language = static_cast<StandbyHaikuLanguage>(pendingLanguage.load());
    const auto calendar = static_cast<StandbyHaikuCalendar>(pendingCalendar.load());
    HaikuOfTheDayEntry entry;
    if (HaikuOfTheDay::fetch(day, language, calendar, entry)) {
        fetchedKey = entry.request;
        lastFetchFailed = false;
        if (entry.request == wantedKey) // settings unchanged during the download
            HaikuOfTheDay::publish(entry);
        ESP_LOGI(LOG_TAG, "Haiku of the day %ld (%s, %s) fetched", day, HaikuOfTheDay::languageTag(language),
                 HaikuOfTheDay::season(day, calendar));
    } else {
        lastFetchFailed = true;
        ESP_LOGW(LOG_TAG, "Haiku of the day %ld: download failed, retrying in 1 h", day);
    }
}
