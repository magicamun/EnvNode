// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
#pragma once
#include <algorithm>
#include <cstddef>
#include "RmsWindow.h"

// Scratch space is owned by the caller; acquisition order remains intact.
template <size_t N> struct RawDiagnostics {
    static_assert(N > 0, "A diagnostic window must contain samples");
    uint16_t sorted[N];
    RmsWindow raw;
    uint16_t p01 = 0, p50 = 0, p99 = 0;
    uint32_t lowRail = 0, highRail = 0, beyond20 = 0, beyond50 = 0;
    void calculate(const uint16_t (&codes)[N], const uint16_t (&mv)[N], double meanMv) {
        raw = RmsWindow{};
        lowRail = highRail = beyond20 = beyond50 = 0;
        for (size_t i = 0; i < N; ++i) {
            raw.add(codes[i]);
            sorted[i] = codes[i];
            lowRail += codes[i] == 0;
            highRail += codes[i] == 4095;
            const double deviation = std::fabs(mv[i] - meanMv);
            beyond20 += deviation > 20;
            beyond50 += deviation > 50;
        }
        std::sort(sorted, sorted + N);
        // Nearest-rank percentiles.
        p01 = sorted[(N + 99) / 100 - 1];
        p50 = sorted[(50 * N + 99) / 100 - 1];
        p99 = sorted[(99 * N + 99) / 100 - 1];
    }
};
