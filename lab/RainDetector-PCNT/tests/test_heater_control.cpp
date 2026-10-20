// EnvNode: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <assert.h>
#include <initializer_list>
#include "HeaterControl.h"

using namespace RainLab;

int main() {
    for (const char* text : {"HEAT 0 30", "HEAT 101 30", "HEAT 25 0",
             "HEAT 25 121", "HEAT -1 30", "HEAT 25 -1", "HEAT 25",
             "HEAT 25 30 extra", "HEAT 25.5 30", "HEAT 25 1.5", "OFF 1",
             "HEAT 999999999999999999999 30", "HEAT 25 9999999999999999",
             "", "heat 25 30", "HEAT25 30", "HEAT ON 30", "HEAT OFF 1",
             "HEAT 101 ON", "HEAT 0 ON", "HEAT 50 ONLY", "HEAT 50 ON extra"}) {
        assert(parseCommand(text).kind == CommandKind::Invalid);
    }
    auto command = parseCommand("  HEAT 25 30\t");
    assert(command.kind == CommandKind::Heat);
    assert(command.percent == 25 && command.seconds == 30);
    command = parseCommand("HEAT 100 120");
    assert(command.kind == CommandKind::Heat);
    assert(command.percent == 100 && command.seconds == 120);
    assert(parseCommand("OFF").kind == CommandKind::Off);
    assert(parseCommand("STATUS").kind == CommandKind::Status);
    assert(parseCommand("HELP").kind == CommandKind::Help);

    assert(parseCommand("HEAT 50 ON").kind == CommandKind::HeatContinuous);
    assert(parseCommand("HEAT 50 ON").percent == 50);
    assert(parseCommand("HEAT ON").kind == CommandKind::Resume);
    assert(parseCommand("HEAT OFF").kind == CommandKind::Off);
    HeaterControl heater;
    assert(!heater.resume());
    assert(heater.percent() == 0 && !heater.expire(0));
    // Check deadlines beyond the 32-bit millis rollover as well as at the boundary.
    const int64_t nowUs = 5000000000000LL;
    heater.start(25, 30, nowUs);
    assert(heater.remainingUs(nowUs) == 30000000);
    assert(!heater.expire(nowUs + 29999999));
    assert(heater.percent() == 25);
    assert(heater.expire(nowUs + 30000000));
    assert(heater.percent() == 0 && heater.remainingUs(nowUs) == 0);
    assert(!heater.expire(nowUs + 31000000));
    heater.start(100, 120, nowUs);
    heater.off();
    assert(heater.percent() == 0 && !heater.expire(nowUs + 120000000));
    heater.start(10, 1, nowUs);
    heater.start(20, 2, nowUs + 500000);
    assert(!heater.expire(nowUs + 1000000));
    assert(heater.expire(nowUs + 2500000));
    // Continuous mode cancels a previous deadline, but explicit OFF still works.
    heater.start(25, 1, nowUs);
    heater.startContinuous(50);
    assert(heater.continuous() && !heater.expire(nowUs + 999000000));
    heater.off();
    assert(!heater.continuous() && heater.percent() == 0);
    assert(heater.resume() && heater.percent() == 50 && heater.continuous());
    // Timed operation replaces continuous mode and updates the remembered level.
    heater.start(10, 1, nowUs);
    assert(!heater.continuous() && heater.expire(nowUs + 1000000));
    assert(heater.resume() && heater.percent() == 10 && heater.continuous());
    HeaterControl afterReset;
    assert(afterReset.percent() == 0 && !afterReset.resume());

    for (const char* text : {"INTERVALL 0 60", "INTERVALL 30 60", "INTERVALL 10 0",
             "INTERVALL 10 301", "INTERVALL 10 60 extra", "INTERVALL 10.5 60",
             "INTERVALL 101 60", "INTERVALL 1060", "INTERVALL 10 -1"})
        assert(parseCommand(text).kind == CommandKind::Invalid);
    command = parseCommand("INTERVALL 10 60");
    assert(command.kind == CommandKind::Interval && command.percent == 10 && command.seconds == 60);
    assert(parseCommand("INTERVALL 100 300").kind == CommandKind::Interval);
    heater.startInterval(10, 60, nowUs);
    assert(heater.interval() && !heater.continuous() && heater.percent() == 0);
    assert(heater.remainingUs(nowUs) == 60000000);
    assert(!heater.advanceInterval(nowUs + 59999999));
    for (unsigned phase = 1; phase <= 40; ++phase) {
        const int64_t planned = nowUs + phase * 60000000LL;
        assert(heater.advanceInterval(planned + 1234));
        const unsigned triangle = phase % 20;
        assert(heater.percent() == (triangle <= 10 ? triangle : 20 - triangle) * 10);
        assert(heater.phase() == phase);
        assert(heater.deadlineUs() == planned + 60000000);
        assert(!heater.expire(planned + 1234));
        assert(!heater.advanceInterval(planned + 1234));
    }
    heater.off();
    assert(!heater.interval() && heater.percent() == 0);
    assert(!heater.advanceInterval(nowUs + 9999999999LL));
    heater.startInterval(100, 300, nowUs);
    assert(heater.advanceInterval(nowUs + 300000000));
    assert(heater.percent() == 100 && !heater.ascending());
    assert(heater.advanceInterval(nowUs + 600000000));
    assert(heater.percent() == 0 && heater.ascending());
    heater.start(25, 1, nowUs);
    assert(!heater.interval() && heater.expire(nowUs + 1000000));
    heater.startInterval(10, 60, nowUs);
    heater.startContinuous(50);
    assert(!heater.interval() && heater.continuous());

}
