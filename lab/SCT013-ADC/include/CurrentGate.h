// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
#pragma once

struct CurrentGate {
    int offMa = 60, onMa = 80;
    bool active = false;
    double apply(double amps) {
        if (amps > onMa / 1000.0) active = true;
        if (amps < offMa / 1000.0) active = false;
        return active ? amps : 0.0;
    }
};
