#ifndef HAIKU_OF_THE_DAY_H
#define HAIKU_OF_THE_DAY_H

#include <cstdint>
#include <string>

// Haiku of the day from haiku.rubet.fr, a static JSON API (source: github.com/louisrubet/haiku).
// The client picks the item itself: today() modulo the count published for the language.
constexpr const char *HAIKU_API_BASE = "https://haiku.rubet.fr/v1/";
constexpr const char *HAIKU_LANGUAGE = "fr-Latn"; // the standby fonts only cover the Latin script

struct HaikuOfTheDayEntry {
    std::string text; // verses separated by '\n'
    std::string author;
    long day = -1; // today() it was fetched for
};

namespace HaikuOfTheDay {
// Days since 1970-01-01 in the configured time zone, so the haiku changes at local midnight;
// -1 while the clock is not synced.
long today();

// Blocking download over HTTPS (firmware CA bundle). Never call it from the UI task.
bool fetch(long day, HaikuOfTheDayEntry &out);

// Slot shared by the fetcher (writer) and the UI task (reader).
void publish(const HaikuOfTheDayEntry &entry);
uint32_t version(); // incremented by each publish; 0 while nothing was published
bool get(HaikuOfTheDayEntry &out);
} // namespace HaikuOfTheDay

#endif // HAIKU_OF_THE_DAY_H
