#include "TimeService.h"
#include <time.h>
#include <esp_sntp.h>
#include <Arduino.h>

namespace EnvNode {

namespace {
constexpr const char* DefaultTimezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";
constexpr const char* DefaultNtpServer1 = "pool.ntp.org";
constexpr const char* DefaultNtpServer2 = "time.cloudflare.com";
}

TimeService::TimeService(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService)
    : logger_(logger)
    , configurationService_(configurationService)
    , wifiService_(wifiService) {
}

void TimeService::begin() {
    syncStarted_ = false;
    synchronized_ = false;
}

void TimeService::loop() {
    if (!wifiService_.connected()) {
        return;
    }

    if (!syncStarted_) {
        startSynchronization();
        return;
    }

    if (synchronized_) {
        return;
    }

    if (isTimeValid()) {
        synchronized_ = true;
        logger_.info("Time synchronized successfully");
        logger_.debugf("UTC timestamp: %s", iso8601Utc().c_str());
        logger_.debugf("Local timestamp: %s", iso8601Local().c_str());
    }
}

bool TimeService::synchronized() const {
    return synchronized_;
}

time_t TimeService::now() const {
    return time(nullptr);
}

bool TimeService::localCivilTime(tm& localTime) const {
    const time_t timestamp = now();
    return localtime_r(&timestamp, &localTime) != nullptr;
}

String TimeService::iso8601Utc() const {
    return formatIso8601(now(), false);
}

String TimeService::iso8601Local() const {
    return formatIso8601(now(), true);
}

String TimeService::iso8601Local(time_t timestamp) const {
    return formatIso8601(timestamp, true);
}

uint32_t TimeService::epoch() const {
    return static_cast<uint32_t>(now());
}

void TimeService::startSynchronization() {
    const Configuration& cfg = configurationService_.getConfiguration();
    const char* timezone = cfg.time.timezone.isEmpty() ? DefaultTimezone : cfg.time.timezone.c_str();
    const char* ntpServer1 = cfg.time.ntpServer1.isEmpty() ? DefaultNtpServer1 : cfg.time.ntpServer1.c_str();
    const char* ntpServer2 = cfg.time.ntpServer2.isEmpty() ? DefaultNtpServer2 : cfg.time.ntpServer2.c_str();

    logger_.info("Time synchronization started");
    logger_.debugf("NTP servers: %s, %s", ntpServer1, ntpServer2);

    if (esp_sntp_enabled()) {
        esp_sntp_stop();
    }
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, ntpServer1);
    esp_sntp_setservername(1, ntpServer2);
    esp_sntp_setservername(2, nullptr);
    esp_sntp_init();

    setenv("TZ", timezone, 1);
    tzset();
    syncStarted_ = true;
}

bool TimeService::isTimeValid() const {
    time_t t = now();
    return t > 1609459200; // 2021-01-01 UTC
}

String TimeService::formatIso8601(time_t timestamp, bool local) const {
    timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    tm timeinfo;

    if (local) {
        localtime_r(&tv.tv_sec, &timeinfo);
    } else {
        gmtime_r(&tv.tv_sec, &timeinfo);
    }

    char buffer[40];
    if (local) {
        char tzOffset[8] = {0};
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d",
                 timeinfo.tm_year + 1900,
                 timeinfo.tm_mon + 1,
                 timeinfo.tm_mday,
                 timeinfo.tm_hour,
                 timeinfo.tm_min,
                 timeinfo.tm_sec);
        if (strftime(tzOffset, sizeof(tzOffset), "%z", &timeinfo) != 0) {
            // Convert +HHMM to +HH:MM if necessary
            if (strlen(tzOffset) == 5) {
                return String(buffer) + String(tzOffset[0]) + String(tzOffset[1]) + String(tzOffset[2]) + ":" + String(tzOffset[3]) + String(tzOffset[4]);
            }
            return String(buffer) + String(tzOffset);
        }
        return String(buffer);
    }

    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02dZ",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);
    return String(buffer);
}

} // namespace EnvNode
