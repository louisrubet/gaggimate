#include "HaikuOfTheDay.h"

#include <ArduinoJson.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <time.h>
#include <version.h>

#ifndef GAGGIMATE_SIM
#include <EspHttpTransport.h>
#endif

namespace {
constexpr size_t MAX_BODY = 2048;
constexpr size_t MAX_TEXT = 400;
constexpr size_t MAX_AUTHOR = 80;
constexpr int MAX_VERSES = 5;
constexpr int TIMEOUT_MS = 10000;
constexpr time_t MIN_VALID_TIME = 1700000000; // before this the clock has not been synced yet

std::mutex slotMutex;
HaikuOfTheDayEntry slot;
std::atomic<uint32_t> slotVersion{0};

#ifdef GAGGIMATE_SIM
// The simulator has no outbound HTTP stack; curl does the HTTPS request on the host.
bool httpGet(const std::string &url, std::string &body) {
    const std::string command = "curl -fsS --proto =https --max-time 10 '" + url + "'";
    FILE *pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
        return false;
    char buffer[256];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), pipe)) > 0 && body.size() <= MAX_BODY)
        body.append(buffer, n);
    return pclose(pipe) == 0 && body.size() <= MAX_BODY;
}
#else
bool httpGet(const std::string &url, std::string &body) {
    EspHttpTransport http(TIMEOUT_MS); // HTTPS only, roots from lib/OTA/certs/ca_roots.pem
    if (!http.begin(url))
        return false;
    http.setHeader("User-Agent", "GaggiMate/" BUILD_GIT_VERSION " (standby haiku)");
    int status = 0;
    int contentLength = 0;
    if (!http.open(status, contentLength) || status != 200 || contentLength > static_cast<int>(MAX_BODY)) {
        http.end();
        return false;
    }
    uint8_t buffer[256];
    int n;
    while ((n = http.read(buffer, sizeof(buffer))) > 0 && body.size() <= MAX_BODY)
        body.append(reinterpret_cast<char *>(buffer), n);
    const bool ok = n == 0 && http.isComplete() && body.size() <= MAX_BODY;
    http.end();
    return ok;
}
#endif

bool getJson(const std::string &url, JsonDocument &doc) {
    std::string body;
    return httpGet(url, body) && deserializeJson(doc, body.data(), body.size()) == DeserializationError::Ok;
}
} // namespace

namespace {
// Day number of a civil date (H. Hinnant), valid for the proleptic Gregorian calendar.
long daysFromCivil(long y, long m, long d) {
    y -= m <= 2 ? 1 : 0;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const long yoe = y - era * 400;
    const long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

// Inverse of daysFromCivil: month 1-12 and day 1-31 of a day number.
void civilFromDays(long z, long &m, long &d) {
    z += 719468;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const long doe = z - era * 146097;
    const long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const long mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp < 10 ? mp + 3 : mp - 9;
}
} // namespace

long HaikuOfTheDay::today() {
    const time_t now = time(nullptr);
    if (now < MIN_VALID_TIME)
        return -1;
    tm local{};
    localtime_r(&now, &local);
    return daysFromCivil(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
}

const char *HaikuOfTheDay::season(long day, StandbyHaikuCalendar calendar) {
    long m, d;
    civilFromDays(day, m, d);
    const int md = static_cast<int>(m * 100 + d); // e.g. 204 for February 4
    if (md <= 115)
        return "new-year";
    // First day of spring, summer, autumn and winter.
    const int japanese[] = {204, 506, 808, 1107};
    const int meteorological[] = {301, 601, 901, 1201};
    const int *starts = calendar == StandbyHaikuCalendar::Meteorological ? meteorological : japanese;
    if (md < starts[0] || md >= starts[3])
        return "winter";
    if (md < starts[1])
        return "spring";
    if (md < starts[2])
        return "summer";
    return "autumn";
}

const char *HaikuOfTheDay::languageTag(StandbyHaikuLanguage language) {
    return language == StandbyHaikuLanguage::English ? "en-Latn" : "fr-Latn";
}

uint32_t HaikuOfTheDay::requestKey(long day, StandbyHaikuLanguage language, StandbyHaikuCalendar calendar) {
    return static_cast<uint32_t>(day) * 4u + static_cast<uint32_t>(language) * 2u + static_cast<uint32_t>(calendar);
}

bool HaikuOfTheDay::fetch(long day, StandbyHaikuLanguage language, StandbyHaikuCalendar calendar, HaikuOfTheDayEntry &out) {
    if (day < 0)
        return false;
    const std::string base = std::string(HAIKU_API_BASE) + languageTag(language) + "/" + season(day, calendar) + "/";

    JsonDocument countDoc;
    if (!getJson(base + "count.json", countDoc))
        return false;
    const long count = countDoc["count"] | 0L;
    if (count <= 0)
        return false;

    char name[16];
    snprintf(name, sizeof(name), "%04ld.json", day % count);
    JsonDocument item;
    if (!getJson(base + name, item))
        return false;
    const char *text = item["text"] | "";
    const char *author = item["author"] | "";
    out.text = text;
    out.author = author;
    out.day = day;
    out.request = requestKey(day, language, calendar);
    const int verses = 1 + static_cast<int>(std::count(out.text.begin(), out.text.end(), '\n'));
    return !out.text.empty() && out.text.size() <= MAX_TEXT && out.author.size() <= MAX_AUTHOR && verses <= MAX_VERSES;
}

void HaikuOfTheDay::publish(const HaikuOfTheDayEntry &entry) {
    std::lock_guard<std::mutex> lock(slotMutex);
    slot = entry;
    slotVersion.fetch_add(1);
}

void HaikuOfTheDay::clear() {
    std::lock_guard<std::mutex> lock(slotMutex);
    slot = HaikuOfTheDayEntry{};
    slotVersion.fetch_add(1);
}

uint32_t HaikuOfTheDay::version() { return slotVersion.load(); }

bool HaikuOfTheDay::get(HaikuOfTheDayEntry &out) {
    std::lock_guard<std::mutex> lock(slotMutex);
    if (slot.day < 0)
        return false;
    out = slot;
    return true;
}
