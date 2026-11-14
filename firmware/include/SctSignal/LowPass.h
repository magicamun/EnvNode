// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
#pragma once
#include <cmath>
// Second-order Butterworth, bilinear transform. New instance per window.
class LowPass {
    double b0, b1, b2, a1, a2;
    double x1=0, x2=0, y1=0, y2=0;
public:
    LowPass(double cutoff, double sampleRate) {
        const double k=std::tan(3.14159265358979323846*cutoff/sampleRate);
        const double norm=1/(1+std::sqrt(2.0)*k+k*k);
        b0=k*k*norm; b1=2*b0; b2=b0;
        a1=2*(k*k-1)*norm; a2=(1-std::sqrt(2.0)*k+k*k)*norm;
    }
    double add(double x) {
        const double y=b0*x+b1*x1+b2*x2-a1*y1-a2*y2;
        x2=x1; x1=x; y2=y1; y1=y;
        return y;
    }
};
