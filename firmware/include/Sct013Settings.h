// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
#pragma once
#include <cstdint>
#include <cmath>
namespace EnvNode {
struct Sct013Settings {
    float ratedA = 5, cal = 1;
    uint32_t ratedMv = 1000, filterHz = 200, offMa = 0, onMa = 0;
    bool valid() const {
        return std::isfinite(ratedA) && ratedA > 0 && ratedA <= 1000 &&
            (ratedMv == 333 || ratedMv == 1000) && std::isfinite(cal) && cal >= 0.1F && cal <= 10 &&
            (filterHz == 0 || filterHz == 100 || filterHz == 200 || filterHz == 300 || filterHz == 500) &&
            ((!offMa && !onMa) || (offMa > 0 && offMa < onMa && onMa <= ratedA * 1000));
    }
    bool operator==(const Sct013Settings& b) const {
        return ratedA==b.ratedA && ratedMv==b.ratedMv && cal==b.cal && filterHz==b.filterHz &&
            offMa==b.offMa && onMa==b.onMa;
    }
};
}
