// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See LICENSE. Presentation-only adapter; never use for controller inputs.
#pragma once
#include "IPropertyReader.h"
#include "Configuration.h"
#include "UnitConverter.h"
#include <cmath>
namespace EnvNode {
class PropertyPresentationReader : public IPropertyReader {
public:
    PropertyPresentationReader(const IPropertyReader& canonical, const PresentationConfiguration& configuration)
        : canonical_(canonical), configuration_(configuration) {}
    bool describe(const PropertyReference& reference, PropertyDescription& result) const override {
        if (!canonical_.describe(reference, result)) { result = PropertyDescription{}; return false; }
        if (converts(result)) result.presentationUnit = UnitConverter::resolvePresentationUnit(
            result.measurementType, configuration_.unitFor(result.measurementType));
        return true;
    }
    PropertyReadResult read(const PropertyReference& reference, PropertySnapshot& result) const override {
        const auto status = canonical_.read(reference, result);
        if (status != PropertyReadResult::Available) { result = PropertySnapshot{}; return status; }
        if (!result.valid) return status;
        PropertyDescription description;
        if (!canonical_.describe(reference, description)) { result = PropertySnapshot{}; return PropertyReadResult::UnknownReference; }
        if (!converts(description)) return status;
        float value = 0, presented = 0;
        const auto unit = UnitConverter::resolvePresentationUnit(description.measurementType,
            configuration_.unitFor(description.measurementType));
        if (!result.value.tryGetFloatingPoint(value) || !std::isfinite(value)
            || !UnitConverter::convert(description.measurementType, value, unit, presented)
            || !std::isfinite(presented)) {
            result.valid = false;
        } else result.value = MeasurementValue::floatingPoint(presented);
        return status;
    }
private:
    static bool converts(const PropertyDescription& description) {
        return description.valueKind == PropertyValueKind::FloatingPoint
            && description.measurementType != MeasurementType::Unknown
            && description.presentationUnit == PresentationUnit::None;
    }
    const IPropertyReader& canonical_;
    const PresentationConfiguration& configuration_;
};
}
