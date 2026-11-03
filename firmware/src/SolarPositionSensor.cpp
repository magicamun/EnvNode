// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include "SolarPositionSensor.h"
#include <cmath>
namespace EnvNode {
namespace {
constexpr double Radians = 3.14159265358979323846 / 180.0;
double normalized(double degrees) {
    const double value = std::fmod(degrees, 360.0);
    return value < 0 ? value + 360.0 : value;
}
}
bool solarPosition(const GeoPosition& p, time_t utc, float& azimuth, float& elevation) {
    // USNO approximate solar coordinates; restrict to 1800..2199 UTC.
    // https://aa.usno.navy.mil/faq/sun_approx and /faq/alt_az
    const double seconds = static_cast<double>(utc);
    if (!p.latitude.configured || !p.longitude.configured || !validGeoPosition(p)
        || seconds < -5364662400.0 || seconds >= 7258118400.0) return false;
    const double days = seconds / 86400.0 + 2440587.5 - 2451545.0;
    const double anomaly = normalized(357.529 + 0.98560028 * days) * Radians;
    const double meanLongitude = normalized(280.459 + 0.98564736 * days);
    const double longitude = (meanLongitude + 1.915 * std::sin(anomaly)
        + 0.020 * std::sin(2 * anomaly)) * Radians;
    const double obliquity = (23.439 - 0.00000036 * days) * Radians;
    const double rightAscension = std::atan2(std::cos(obliquity) * std::sin(longitude), std::cos(longitude));
    const double declination = std::asin(std::sin(obliquity) * std::sin(longitude));
    const double sidereal = normalized(280.46061837 + 360.98564736629 * days);
    const double hourAngle = (sidereal + p.longitude.value) * Radians - rightAscension;
    const double latitude = p.latitude.value * Radians;
    const double east = -std::cos(declination) * std::sin(hourAngle);
    const double north = std::sin(declination) * std::cos(latitude)
        - std::cos(declination) * std::cos(hourAngle) * std::sin(latitude);
    const double up = std::sin(declination) * std::sin(latitude)
        + std::cos(declination) * std::cos(hourAngle) * std::cos(latitude);
    elevation = static_cast<float>(std::atan2(up, std::hypot(east, north)) / Radians);
    // Azimuth is undefined at zenith/nadir; never publish an arbitrary bearing.
    if (std::hypot(east, north) < 1e-10) return false;
    azimuth = static_cast<float>(normalized(std::atan2(east, north) / Radians));
    if (azimuth >= 360.0F) azimuth = 0;
    return std::isfinite(azimuth) && std::isfinite(elevation);
}
SensorOperationResult SolarPositionSensor::sample(IMeasurementSink& output) {
    float azimuth = 0, elevation = 0;
    const bool valid = time_.synchronized()
        && solarPosition(configuration_.getConfiguration().geoPosition, time_.now(), azimuth, elevation);
    state_ = valid ? SensorState::Ready : SensorState::Degraded;
    for (MeasurementType type : {MeasurementType::SolarAzimuth, MeasurementType::SolarElevation}) {
        Measurement measurement;
        measurement.type = type;
        measurement.valid = valid;
        measurement.quality = valid ? MeasurementQuality::Good : MeasurementQuality::Degraded;
        if (valid) measurement.value = MeasurementValue::floatingPoint(
            type == MeasurementType::SolarAzimuth ? azimuth : elevation);
        output.emit(measurement);
    }
    return SensorOperationResult::Completed;
}
}
