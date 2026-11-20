#ifndef STANDBY_HAIKU_PLUGIN_H
#define STANDBY_HAIKU_PLUGIN_H

#include <atomic>
#include <display/core/Plugin.h>

class Settings;

// What the standby screen shows in place of the GAGGIMATE logo. Value of "standbyHaiku:display:change".
enum class StandbyHaikuDisplay : int { Off = 0, BuiltIn = 1, OfTheDay = 2 };

// Turns the standby haiku on or off (setting "standbyHaikuActive") and picks its source ("standbyHaikuOnline":
// haiku of the day from haiku.rubet.fr, else the built-in haikus). The plugin only owns the state; the UI task
// applies it on "standbyHaiku:display:change", because LVGL must not be touched from the controller task.
// Online, it downloads the haiku of the day (HaikuOfTheDay.h) once per UTC day, in a short-lived task so
// neither the controller nor the UI task waits on the network.
class StandbyHaikuPlugin : public Plugin {
  public:
    void setup(Controller *controller, PluginManager *pluginManager) override;
    void loop() override;

  private:
    void scheduleFetch();
    void runFetch(long day);
    static void fetchTask(void *arg);

    PluginManager *pluginManager = nullptr;
    const Settings *settings = nullptr;
    int published = -1; // -1: nothing published yet, so the first loop always publishes the boot state

    std::atomic<bool> fetching{false};
    std::atomic<long> fetchedDay{-1};
    std::atomic<long> pendingDay{-1};
    std::atomic<bool> lastFetchFailed{false};
    std::atomic<unsigned long> lastFetchMs{0};
};

#endif // STANDBY_HAIKU_PLUGIN_H
