#include "TimeService.h"
#include <time.h>
#include <lwip/apps/sntp.h>
#include <Arduino.h>

namespace WeatherStation {

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
    syncAttemptInProgress_ = false;
    synchronized_ = false;
    lastSyncAttemptMs_ = 0;
}

void TimeService::loop() {
    if (!wifiService_.connected()) {
        return;
    }

    if (synchronized_) {
        return;
    }

    if (!syncAttemptInProgress_) {
        startSynchronization();
        return;
    }

    if (isTimeValid()) {
        synchronized_ = true;
        logger_.println("Time synchronized successfully");
        logger_.printf("UTC timestamp: %s\n", iso8601Utc().c_str());
        logger_.printf("Local timestamp: %s\n", iso8601Local().c_str());
        return;
    }

    unsigned long now = millis();
    if (now - lastSyncAttemptMs_ >= SyncRetryIntervalMs) {
        startSynchronization();
    }
}

bool TimeService::synchronized() const {
    return synchronized_;
}

time_t TimeService::now() const {
    return time(nullptr);
}

String TimeService::iso8601Utc() const {
    return formatIso8601(now(), false);
}

String TimeService::iso8601Local() const {
    return formatIso8601(now(), true);
}

uint32_t TimeService::epoch() const {
    return static_cast<uint32_t>(now());
}

void TimeService::startSynchronization() {
    const Configuration& cfg = configurationService_.getConfiguration();
    const String timezone = cfg.timezone.isEmpty() ? String(DefaultTimezone) : cfg.timezone;
    const String ntpServer1 = cfg.ntpServer1.isEmpty() ? String(DefaultNtpServer1) : cfg.ntpServer1;
    const String ntpServer2 = cfg.ntpServer2.isEmpty() ? String(DefaultNtpServer2) : cfg.ntpServer2;

    logger_.println("Time synchronization started");
    logger_.printf("NTP server used: %s, %s\n", ntpServer1.c_str(), ntpServer2.c_str());

    configTzTime(timezone.c_str(), ntpServer1.c_str(), ntpServer2.c_str());
    tzset();
    syncAttemptInProgress_ = true;
    lastSyncAttemptMs_ = millis();
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

} // namespace WeatherStation
