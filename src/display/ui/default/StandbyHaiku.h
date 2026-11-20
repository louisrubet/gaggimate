#ifndef STANDBYHAIKU_H
#define STANDBYHAIKU_H

#include <cstdint>
#include <display/plugins/StandbyHaikuPlugin.h>
#include <lvgl.h>

// Haiku shown on the standby screen in place of the GAGGIMATE logo, one per local calendar day: the haiku of
// the day downloaded by StandbyHaikuPlugin, or else the built-in haiku of the day (BuiltInHaikus.h).
// Driven by StandbyHaikuPlugin; call from the UI task only.

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
    // Call regularly with whether the standby screen is the current one; changes the haiku when the day changes.
    void tick(bool onStandby);
    void setColor(lv_color_t color);

  private:
    void showBuiltIn(long day);
    bool showHaikuOfTheDay(long day);
    void show(const char *text, const char *author);
    static bool fontCovers(const char *text);

    lv_obj_t *logo = nullptr;
    lv_obj_t *box = nullptr;
    lv_obj_t *haikuLabel = nullptr;
    lv_obj_t *authorLabel = nullptr;
    StandbyHaikuDisplay display = StandbyHaikuDisplay::Off;
    bool shown = false;                // a haiku is on screen
    bool shownOfTheDay = false;        // it is the downloaded one
    long shownDay = -1;                // HaikuOfTheDay::today() when it was shown (-1: clock not synced)
    uint32_t triedOfTheDayVersion = 0; // last version checked, so a rejected download is not retried every tick
};

#endif // STANDBYHAIKU_H
