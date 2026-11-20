#include "StandbyHaiku.h"

#include "BuiltInHaikus.h"

#include <display/plugins/HaikuOfTheDay.h>
#include <initializer_list>

namespace {
constexpr lv_coord_t HAIKU_WIDTH = 400; // stays inside the round panel over the height of the haiku block
constexpr uint32_t FADE_MS = 800;
} // namespace

void StandbyHaiku::setup(lv_obj_t *screen, lv_obj_t *logo, lv_color_t color) {
    if (screen == nullptr || box != nullptr)
        return;
    this->logo = logo;

    // Transparent container that never takes a touch, so the standby screen still wakes the machine up.
    box = lv_obj_create(screen);
    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, HAIKU_WIDTH, LV_SIZE_CONTENT);
    lv_obj_align(box, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(box, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(box, 14, LV_PART_MAIN);

    haikuLabel = lv_label_create(box);
    lv_obj_set_width(haikuLabel, HAIKU_WIDTH);
    lv_label_set_long_mode(haikuLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(haikuLabel, &standby_haiku_font_26, LV_PART_MAIN);
    lv_obj_set_style_text_align(haikuLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(haikuLabel, 6, LV_PART_MAIN);

    authorLabel = lv_label_create(box);
    lv_obj_set_style_text_font(authorLabel, &standby_haiku_font_18, LV_PART_MAIN);
    lv_obj_set_style_text_opa(authorLabel, LV_OPA_60, LV_PART_MAIN);

    setColor(color);
}

void StandbyHaiku::setDisplay(StandbyHaikuDisplay newDisplay) {
    if (box == nullptr || newDisplay == display)
        return;
    display = newDisplay;

    if (display == StandbyHaikuDisplay::Off) {
        lv_obj_add_flag(box, LV_OBJ_FLAG_HIDDEN);
        if (logo != nullptr)
            lv_obj_clear_flag(logo, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    if (logo != nullptr)
        lv_obj_add_flag(logo, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_HIDDEN);
    wasOnStandby = false; // next tick on the standby screen shows a haiku
    shownOfTheDayVersion = 0;
    triedOfTheDayVersion = 0;
}

void StandbyHaiku::tick(bool onStandby, unsigned long now) {
    const bool entering = onStandby && !wasOnStandby;
    wasOnStandby = onStandby;
    if (!onStandby || display == StandbyHaikuDisplay::Off)
        return;
    if (display == StandbyHaikuDisplay::OfTheDay) {
        // A new download shows at once; the haiku of the day then stays until the next one.
        const uint32_t version = HaikuOfTheDay::version();
        if (version != triedOfTheDayVersion) {
            triedOfTheDayVersion = version;
            if (showHaikuOfTheDay())
                return;
        }
        if (shownOfTheDayVersion != 0)
            return;
    }
    if (entering || now - lastChange >= STANDBY_HAIKU_INTERVAL_MS)
        showNextBuiltIn(now);
}

void StandbyHaiku::setColor(lv_color_t color) {
    for (lv_obj_t *obj : {haikuLabel, authorLabel}) {
        if (obj != nullptr)
            lv_obj_set_style_text_color(obj, color, LV_PART_MAIN);
    }
}

void StandbyHaiku::showNextBuiltIn(unsigned long now) {
    builtInIndex = (builtInIndex + 1) % BUILT_IN_HAIKU_COUNT;
    lastChange = now;
    shownOfTheDayVersion = 0;
    show(BUILT_IN_HAIKUS[builtInIndex].text, BUILT_IN_HAIKUS[builtInIndex].author);
}

bool StandbyHaiku::showHaikuOfTheDay() {
    const uint32_t version = HaikuOfTheDay::version();
    HaikuOfTheDayEntry entry;
    if (!HaikuOfTheDay::get(entry) || !fontCovers(entry.text.c_str()) || !fontCovers(entry.author.c_str()))
        return false; // keep the built-in haikus rather than draw missing glyphs
    shownOfTheDayVersion = version;
    show(entry.text.c_str(), entry.author.c_str());
    return true;
}

void StandbyHaiku::show(const char *text, const char *author) {
    // Verses are never wrapped by choice: if the widest one does not fit at 26 px, the whole haiku goes to 18 px,
    // so all its lines keep the same size. LV_LABEL_LONG_WRAP stays only as a last resort.
    lv_point_t size;
    lv_txt_get_size(&size, text, &standby_haiku_font_26, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    const lv_font_t *font = size.x <= HAIKU_WIDTH ? &standby_haiku_font_26 : &standby_haiku_font_18;
    lv_obj_set_style_text_font(haikuLabel, font, LV_PART_MAIN);
    lv_label_set_text(haikuLabel, text); // copies: a downloaded text does not outlive the next download
    lv_label_set_text(authorLabel, author);
    lv_obj_fade_in(box, FADE_MS, 0);
}

// Both haiku fonts cover the same characters.
bool StandbyHaiku::fontCovers(const char *text) {
    uint32_t i = 0;
    while (text[i] != '\0') {
        const uint32_t letter = _lv_txt_encoded_next(text, &i);
        lv_font_glyph_dsc_t glyph;
        if (letter != '\n' && !lv_font_get_glyph_dsc(&standby_haiku_font_18, &glyph, letter, 0))
            return false;
    }
    return true;
}
