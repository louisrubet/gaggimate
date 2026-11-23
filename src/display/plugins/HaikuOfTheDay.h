#ifndef HAIKU_OF_THE_DAY_H
#define HAIKU_OF_THE_DAY_H

#include <cstdint>
#include <string>

// Haiku of the day from haiku.rubet.fr, a static JSON API (source: github.com/louisrubet/haiku).
// The client picks the list of the current season itself, then the item: today() modulo the count of that list.
constexpr const char *HAIKU_API_BASE = "https://haiku.rubet.fr/v1/";

// Values of the setting "standbyHaikuLanguage". The standby fonts only cover the Latin script.
enum class StandbyHaikuLanguage : int { French = 0, English = 1 };
// Values of the setting "standbyHaikuCalendar". Both have a New Year period from January 1 to 15.
enum class StandbyHaikuCalendar : int {
    Japanese = 0,      // seasons of the kigo: spring from Feb 4, summer May 6, autumn Aug 8, winter Nov 7
    Meteorological = 1 // spring from Mar 1, summer Jun 1, autumn Sep 1, winter Dec 1
};

struct HaikuOfTheDayEntry {
    std::string text; // verses separated by '\n'
    std::string author;
    long day = -1;        // today() it was fetched for
    uint32_t request = 0; // HaikuOfTheDay::requestKey() it was fetched for
};

namespace HaikuOfTheDay {
// Days since 1970-01-01 in the configured time zone, so the haiku changes at local midnight;
// -1 while the clock is not synced.
long today();

// Season of a day (northern hemisphere), as named by the API: new-year, spring, summer, autumn, winter.
const char *season(long day, StandbyHaikuCalendar calendar);

const char *languageTag(StandbyHaikuLanguage language);

// Identifies what to download for a day; a change of setting asks for a new download the same day.
uint32_t requestKey(long day, StandbyHaikuLanguage language, StandbyHaikuCalendar calendar);

// Blocking download over HTTPS (firmware CA bundle). Never call it from the UI task.
bool fetch(long day, StandbyHaikuLanguage language, StandbyHaikuCalendar calendar, HaikuOfTheDayEntry &out);

// Slot shared by the fetcher (writer) and the UI task (reader).
void publish(const HaikuOfTheDayEntry &entry);
void clear();       // nothing to show: the logo stays
uint32_t version(); // incremented by each publish or clear; 0 while nothing was published
bool get(HaikuOfTheDayEntry &out);
} // namespace HaikuOfTheDay

#endif // HAIKU_OF_THE_DAY_H
