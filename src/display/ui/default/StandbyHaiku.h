#ifndef STANDBYHAIKU_H
#define STANDBYHAIKU_H

#include <cstdint>
#include <display/plugins/StandbyHaikuPlugin.h>
#include <lvgl.h>

// Haiku shown on the standby screen in place of the GAGGIMATE logo, one per local calendar day: the haiku of
// the day downloaded by StandbyHaikuPlugin. The logo stays while there is none for today.
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
    // Off: logo shown. OfTheDay: today's haiku once downloaded, the logo until then.
    void setDisplay(StandbyHaikuDisplay display);
    // Call regularly with whether the standby screen is the current one; follows downloads and the day.
    void tick(bool onStandby);
    void setColor(lv_color_t color);

  private:
    void showLogo();
    void show(const char *text, const char *author);
    static bool fontCovers(const char *text);

    lv_obj_t *logo = nullptr;
    lv_obj_t *box = nullptr;
    lv_obj_t *haikuLabel = nullptr;
    lv_obj_t *authorLabel = nullptr;
    StandbyHaikuDisplay display = StandbyHaikuDisplay::Off;
    uint32_t checkedVersion = 0; // HaikuOfTheDay::version() last checked, so a tick does nothing most of the time
    long checkedDay = -1;        // HaikuOfTheDay::today() last checked
};

#endif // STANDBYHAIKU_H
