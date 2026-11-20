#include "HaikuOfTheDay.h"

#include <ArduinoJson.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <mutex>
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

bool HaikuOfTheDay::fetch(long day, HaikuOfTheDayEntry &out) {
    const std::string base = std::string(HAIKU_API_BASE) + HAIKU_LANGUAGE + "/";

    JsonDocument countDoc;
    if (!getJson(base + "count.json", countDoc))
        return false;
    const long count = countDoc["count"] | 0L;
    if (count <= 0 || day < 0)
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
    const int verses = 1 + static_cast<int>(std::count(out.text.begin(), out.text.end(), '\n'));
    return !out.text.empty() && out.text.size() <= MAX_TEXT && out.author.size() <= MAX_AUTHOR && verses <= MAX_VERSES;
}

void HaikuOfTheDay::publish(const HaikuOfTheDayEntry &entry) {
    std::lock_guard<std::mutex> lock(slotMutex);
    slot = entry;
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
