// EnvNode: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include "RainDetectorEvaluation.h"
int main() {
    EnvNode::RainDetectorEvaluation core;
    std::cerr << "core_bytes=" << sizeof(core) << '\n';
    std::cout << std::fixed << std::setprecision(3);
    std::string line;
    while(std::getline(std::cin,line)) {
        std::replace(line.begin(),line.end(),',',' ');
        std::istringstream input(line);
        uint64_t timeMs;std::string frequency;unsigned level;
        if(!(input>>timeMs>>frequency>>level)) return 1;
        float hz;
        try { hz=std::stof(frequency); } catch(...) { return 2; }
        const auto result=core.update(timeMs,hz,level);
        std::cout << timeMs << ',' << result.fastHz << ',' << result.slowHz << ','
                  << result.trendHzPerSecond << ',' << result.valid << ','
                  << EnvNode::rainEstimateName(result.estimate) << ',' << result.newWetting << '\n';
    }
}
