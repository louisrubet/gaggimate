#include "StandbyHaiku.h"

#include <cmath>
#include <cstring>
#include <display/plugins/HaikuOfTheDay.h>
#include <initializer_list>

namespace {
constexpr lv_coord_t HAIKU_WIDTH = 400; // label width before a layout is computed
constexpr lv_coord_t SAFE_RADIUS = 228; // 480 px round panel minus a 12 px margin
constexpr lv_coord_t LINE_SPACE = 6;    // between verses
constexpr lv_coord_t AUTHOR_GAP = 14;   // between the haiku and its author
constexpr int MAX_VERSES = 8;
constexpr uint32_t FADE_MS = 800;

// Width of the round panel at a vertical distance dy from its center, inside the margin.
lv_coord_t chordWidth(lv_coord_t dy) {
    if (dy >= SAFE_RADIUS)
        return 0;
    return static_cast<lv_coord_t>(2.0f * sqrtf(static_cast<float>(SAFE_RADIUS * SAFE_RADIUS - dy * dy)));
}

// Fits each verse of text in font against the panel width at its own height, the haiku and author block
// being centered vertically. Returns the widest verse, or -1 if one does not fit.
lv_coord_t layoutWidth(const char *text, const lv_font_t *font) {
    lv_coord_t widths[MAX_VERSES];
    int count = 0;
    const char *line = text;
    while (count < MAX_VERSES) {
        const char *end = strchr(line, '\n');
        const uint32_t len = end ? static_cast<uint32_t>(end - line) : static_cast<uint32_t>(strlen(line));
        widths[count++] = lv_txt_get_width(line, len, font, 0, LV_TEXT_FLAG_NONE);
        if (end == nullptr)
            break;
        line = end + 1;
    }
    const lv_coord_t lineHeight = lv_font_get_line_height(font);
    const lv_coord_t textHeight = count * lineHeight + (count - 1) * LINE_SPACE;
    const lv_coord_t blockHeight = textHeight + AUTHOR_GAP + lv_font_get_line_height(&standby_haiku_font_18);
    lv_coord_t widest = 0;
    for (int i = 0; i < count; ++i) {
        const lv_coord_t top = -blockHeight / 2 + i * (lineHeight + LINE_SPACE);
        const lv_coord_t bottom = top + lineHeight;
        const lv_coord_t farthest = LV_MAX(LV_ABS(top), LV_ABS(bottom));
        if (widths[i] > chordWidth(farthest))
            return -1;
        widest = LV_MAX(widest, widths[i]);
    }
    return widest;
}
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
    lv_obj_set_style_pad_row(box, AUTHOR_GAP, LV_PART_MAIN);

    haikuLabel = lv_label_create(box);
    lv_obj_set_width(haikuLabel, HAIKU_WIDTH);
    lv_label_set_long_mode(haikuLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(haikuLabel, &standby_haiku_font_26, LV_PART_MAIN);
    lv_obj_set_style_text_align(haikuLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(haikuLabel, LINE_SPACE, LV_PART_MAIN);

    authorLabel = lv_label_create(box);
    lv_obj_set_style_text_font(authorLabel, &standby_haiku_font_18, LV_PART_MAIN);
    lv_obj_set_style_text_opa(authorLabel, LV_OPA_60, LV_PART_MAIN);

    setColor(color);
}

void StandbyHaiku::setDisplay(StandbyHaikuDisplay newDisplay) {
    if (box == nullptr || newDisplay == display)
        return;
    display = newDisplay;
    showLogo();
    checkedVersion = 0; // next tick on the standby screen looks for today's haiku
    checkedDay = -1;
}

void StandbyHaiku::tick(bool onStandby) {
    if (!onStandby || display == StandbyHaikuDisplay::Off)
        return;
    // A download for today shows at once and stays all day; at midnight or on a setting change the plugin clears
    // it, and the logo comes back until the next download.
    const long day = HaikuOfTheDay::today();
    const uint32_t version = HaikuOfTheDay::version();
    if (version == checkedVersion && day == checkedDay)
        return;
    checkedVersion = version;
    checkedDay = day;
    HaikuOfTheDayEntry entry;
    if (HaikuOfTheDay::get(entry) && entry.day == day && fontCovers(entry.text.c_str()) && fontCovers(entry.author.c_str()))
        show(entry.text.c_str(), entry.author.c_str());
    else
        showLogo(); // nothing yet, yesterday's, or glyphs the fonts lack
}

void StandbyHaiku::setColor(lv_color_t color) {
    for (lv_obj_t *obj : {haikuLabel, authorLabel}) {
        if (obj != nullptr)
            lv_obj_set_style_text_color(obj, color, LV_PART_MAIN);
    }
}

void StandbyHaiku::showLogo() {
    lv_obj_add_flag(box, LV_OBJ_FLAG_HIDDEN);
    if (logo != nullptr)
        lv_obj_clear_flag(logo, LV_OBJ_FLAG_HIDDEN);
}

void StandbyHaiku::show(const char *text, const char *author) {
    // Verses are never wrapped by choice: each one must fit the round panel at its own height. If one does not fit
    // at 26 px, the whole haiku goes to 18 px, so all its lines keep the same size. Wrapping at the width of the
    // panel's middle stays only as a last resort.
    const lv_font_t *font = &standby_haiku_font_26;
    lv_coord_t width = layoutWidth(text, font);
    if (width < 0) {
        font = &standby_haiku_font_18;
        width = layoutWidth(text, font);
    }
    if (width < 0)
        width = HAIKU_WIDTH;
    else
        width += 2; // rounding slack, so that LVGL does not wrap a verse measured at the exact width
    width = LV_MAX(width, lv_txt_get_width(author, strlen(author), &standby_haiku_font_18, 0, LV_TEXT_FLAG_NONE));
    lv_obj_set_width(box, width);
    lv_obj_set_width(haikuLabel, width);
    lv_obj_set_style_text_font(haikuLabel, font, LV_PART_MAIN);
    lv_label_set_text(haikuLabel, text); // copies: a downloaded text does not outlive the next download
    lv_label_set_text(authorLabel, author);
    if (logo != nullptr)
        lv_obj_add_flag(logo, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_HIDDEN);
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
