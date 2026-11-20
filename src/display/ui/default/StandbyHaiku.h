#ifndef STANDBYHAIKU_H
#define STANDBYHAIKU_H

#include <cstddef>
#include <cstdint>
#include <display/plugins/StandbyHaikuPlugin.h>
#include <lvgl.h>

// Haiku shown on the standby screen in place of the GAGGIMATE logo: the haiku of the day downloaded by
// StandbyHaikuPlugin, or the built-in haikus (BuiltInHaikus.h), changed at each standby entry and then every
// STANDBY_HAIKU_INTERVAL_MS while no haiku of the day is available.
// Driven by StandbyHaikuPlugin; call from the UI task only.
#ifndef STANDBY_HAIKU_INTERVAL_MS
#define STANDBY_HAIKU_INTERVAL_MS (15UL * 60UL * 1000UL) // a fixed text must not stay for hours on an AMOLED panel
#endif

extern "C" {
// Generated C files, see fonts/README.md
LV_FONT_DECLARE(standby_haiku_font_26);
LV_FONT_DECLARE(standby_haiku_font_18);
}

class StandbyHaiku {
  public:
    // Builds the haiku block, hidden, at the place of `logo` inside `screen`.
    void setup(lv_obj_t *screen, lv_obj_t *logo, lv_color_t color);
    // Off: logo shown. Otherwise: logo hidden, haiku shown.
    void setDisplay(StandbyHaikuDisplay display);
    // Call regularly with whether the standby screen is the current one; changes the haiku.
    void tick(bool onStandby, unsigned long now);
    void setColor(lv_color_t color);

  private:
    void showNextBuiltIn(unsigned long now);
    bool showHaikuOfTheDay();
    void show(const char *text, const char *author);
    static bool fontCovers(const char *text);

    lv_obj_t *logo = nullptr;
    lv_obj_t *box = nullptr;
    lv_obj_t *haikuLabel = nullptr;
    lv_obj_t *authorLabel = nullptr;
    StandbyHaikuDisplay display = StandbyHaikuDisplay::Off;
    size_t builtInIndex = SIZE_MAX; // next one shown is the first
    unsigned long lastChange = 0;
    bool wasOnStandby = false;
    uint32_t shownOfTheDayVersion = 0; // HaikuOfTheDay::version() on screen, 0 when a built-in haiku is shown
    uint32_t triedOfTheDayVersion = 0; // last version checked, so a rejected download is not retried every tick
};

#endif // STANDBYHAIKU_H
