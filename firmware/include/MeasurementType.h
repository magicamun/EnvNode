#pragma once

namespace WeatherStation {

enum class MeasurementType {
    Unknown,
    Temperature,
    RelativeHumidity,
    AtmosphericPressure,
    SolarIrradiance,
    SolarCellTemperature,
    RainDetectorValue,
    RainDetectorState,
    RainGaugeTip,
};

} // namespace WeatherStation
