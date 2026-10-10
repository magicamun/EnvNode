// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository LICENSE.
#pragma once
#include <cstddef>
#include <cstdint>
namespace EnvNode {
// Owns command framing per display. The sink completes one bounded I2C packet
// including STOP; a failed packet suppresses the rest of the operation.
class IOledPacketSink {
public:
    virtual ~IOledPacketSink() = default;
    virtual bool sendOledPacket(const uint8_t* bytes, size_t length) = 0;
};
class OledCommandTransport {
public:
    explicit OledCommandTransport(IOledPacketSink& sink) : sink_(sink) {}
    void reset() { used_ = 0; failed_ = false; }
    bool failed() const { return failed_; }
    bool command(uint8_t value) {
        if (!flush()) return false;
        packet_[0] = 0x00; packet_[1] = value; used_ = 2;
        return true;
    }
    bool argument(uint8_t value) {
        if (failed_) return false;
        if (used_ < 2 || used_ == sizeof(packet_)) { failed_ = true; used_ = 0; return false; }
        packet_[used_++] = value;
        return true;
    }
    bool flush() {
        if (failed_) return false;
        if (!used_) return true;
        const size_t count = used_; used_ = 0;
        failed_ = !sink_.sendOledPacket(packet_, count);
        return !failed_;
    }
    bool data(const uint8_t* bytes, size_t length) {
        if (!flush()) return false;
        while (length) {
            const size_t count = length > 24 ? 24 : length;
            packet_[0] = 0x40;
            for (size_t i = 0; i < count; ++i) packet_[i + 1] = bytes[i];
            if (!sink_.sendOledPacket(packet_, count + 1)) { failed_ = true; return false; }
            bytes += count; length -= count;
        }
        return true;
    }
private:
    IOledPacketSink& sink_;
    uint8_t packet_[32] = {};
    size_t used_ = 0;
    bool failed_ = false;
};
}
