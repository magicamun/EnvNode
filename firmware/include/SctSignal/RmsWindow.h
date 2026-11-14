// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
#pragma once
#include <cmath>
#include <cstdint>

// Welford variance: AC RMS after removing the window's measured DC offset.
struct RmsWindow {
    uint32_t count = 0;
    double mean = 0, m2 = 0;
    double minimum = 1e9, maximum = -1e9;
    void add(double mv) {
        ++count;
        const double delta = mv - mean;
        mean += delta / count;
        m2 += delta * (mv - mean);
        if (mv < minimum) minimum = mv;
        if (mv > maximum) maximum = mv;
    }
    double acRmsMv() const { return count ? std::sqrt(m2 / count) : 0; }
};
