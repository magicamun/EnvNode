#include "CompactCborCodec.h"

#include <cstring>
#include <limits>

namespace EnvNode {

CompactCborWriter::CompactCborWriter(uint8_t* output, size_t capacity)
    : output_(output), capacity_(capacity) {
    if (output == nullptr || capacity == 0) {
        status_ = CompactCborStatus::InvalidArgument;
    }
}

bool CompactCborWriter::append(const uint8_t* data, size_t size) {
    if (status_ != CompactCborStatus::Success) return false;
    if (size == 0) return true;
    if (data == nullptr || size > capacity_ - size_) {
        status_ = data == nullptr
            ? CompactCborStatus::InvalidArgument
            : CompactCborStatus::OutputTooSmall;
        return false;
    }
    std::memcpy(output_ + size_, data, size);
    size_ += size;
    return true;
}

bool CompactCborWriter::writeHead(uint8_t majorType, uint64_t value) {
    uint8_t encoded[9];
    size_t count = 1;
    if (value < 24) {
        encoded[0] = static_cast<uint8_t>((majorType << 5) | value);
    } else if (value <= UINT8_MAX) {
        encoded[0] = static_cast<uint8_t>((majorType << 5) | 24);
        encoded[1] = static_cast<uint8_t>(value);
        count = 2;
    } else if (value <= UINT16_MAX) {
        encoded[0] = static_cast<uint8_t>((majorType << 5) | 25);
        encoded[1] = static_cast<uint8_t>(value >> 8);
        encoded[2] = static_cast<uint8_t>(value);
        count = 3;
    } else if (value <= UINT32_MAX) {
        encoded[0] = static_cast<uint8_t>((majorType << 5) | 26);
        encoded[1] = static_cast<uint8_t>(value >> 24);
        encoded[2] = static_cast<uint8_t>(value >> 16);
        encoded[3] = static_cast<uint8_t>(value >> 8);
        encoded[4] = static_cast<uint8_t>(value);
        count = 5;
    } else {
        encoded[0] = static_cast<uint8_t>((majorType << 5) | 27);
        for (size_t index = 0; index < 8; ++index) {
            encoded[index + 1] = static_cast<uint8_t>(value >> (56 - index * 8));
        }
        count = 9;
    }
    return append(encoded, count);
}

bool CompactCborWriter::writeUnsigned(uint64_t value) {
    return writeHead(0, value);
}

bool CompactCborWriter::writeInteger(int64_t value) {
    return value >= 0
        ? writeUnsigned(static_cast<uint64_t>(value))
        : writeHead(1, static_cast<uint64_t>(-(value + 1)));
}

bool CompactCborWriter::writeByteString(const uint8_t* value, size_t size) {
    return (value != nullptr || size == 0)
        && writeHead(2, size) && append(value, size);
}

bool CompactCborWriter::writeText(const char* value) {
    if (value == nullptr) {
        status_ = CompactCborStatus::InvalidArgument;
        return false;
    }
    return writeText(value, std::strlen(value));
}

bool CompactCborWriter::writeText(const char* value, size_t size) {
    return value != nullptr
        && writeHead(3, size)
        && append(reinterpret_cast<const uint8_t*>(value), size);
}

bool CompactCborWriter::beginArray(size_t itemCount) {
    return writeHead(4, itemCount);
}

bool CompactCborWriter::beginMap(size_t pairCount) {
    return writeHead(5, pairCount);
}

bool CompactCborWriter::writeBoolean(bool value) {
    const uint8_t encoded = value ? 0xF5 : 0xF4;
    return append(&encoded, 1);
}

bool CompactCborWriter::writeNull() {
    const uint8_t encoded = 0xF6;
    return append(&encoded, 1);
}

bool CompactCborWriter::writeDouble(double value) {
    uint64_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value), "unexpected double size");
    std::memcpy(&bits, &value, sizeof(bits));
    uint8_t encoded[9] = {0xFB};
    for (size_t index = 0; index < 8; ++index) {
        encoded[index + 1] = static_cast<uint8_t>(bits >> (56 - index * 8));
    }
    return append(encoded, sizeof(encoded));
}

CompactCborStatus CompactCborWriter::status() const {
    return status_;
}

size_t CompactCborWriter::size() const {
    return size_;
}

CompactCborReader::CompactCborReader(const uint8_t* input, size_t size)
    : input_(input), size_(size) {
    if (input == nullptr || size == 0) {
        status_ = CompactCborStatus::InvalidArgument;
    }
}

void CompactCborReader::fail(CompactCborStatus status) {
    if (status_ == CompactCborStatus::Success) status_ = status;
}

bool CompactCborReader::require(size_t size) {
    if (status_ != CompactCborStatus::Success) return false;
    if (size > size_ - offset_) {
        fail(CompactCborStatus::EndOfInput);
        return false;
    }
    return true;
}

bool CompactCborReader::decodeHead(
    uint8_t& majorType,
    uint64_t& value,
    size_t& headSize) {
    if (!require(1)) return false;
    const uint8_t first = input_[offset_];
    majorType = first >> 5;
    const uint8_t additional = first & 0x1F;
    if (additional < 24) {
        value = additional;
        headSize = 1;
        return true;
    }
    size_t byteCount = 0;
    if (additional == 24) byteCount = 1;
    else if (additional == 25) byteCount = 2;
    else if (additional == 26) byteCount = 4;
    else if (additional == 27) byteCount = 8;
    else {
        fail(CompactCborStatus::UnsupportedValue);
        return false;
    }
    if (!require(1 + byteCount)) return false;
    value = 0;
    for (size_t index = 0; index < byteCount; ++index) {
        value = (value << 8) | input_[offset_ + 1 + index];
    }
    if ((byteCount == 1 && value < 24)
        || (byteCount == 2 && value <= UINT8_MAX)
        || (byteCount == 4 && value <= UINT16_MAX)
        || (byteCount == 8 && value <= UINT32_MAX)) {
        fail(CompactCborStatus::NonCanonical);
        return false;
    }
    headSize = 1 + byteCount;
    return true;
}

bool CompactCborReader::readHead(uint8_t expectedMajorType, uint64_t& value) {
    uint8_t majorType = 0;
    size_t headSize = 0;
    if (!decodeHead(majorType, value, headSize)) return false;
    if (majorType != expectedMajorType) {
        fail(CompactCborStatus::TypeMismatch);
        return false;
    }
    offset_ += headSize;
    return true;
}

bool CompactCborReader::readUnsigned(uint64_t& value) {
    return readHead(0, value);
}

bool CompactCborReader::readInteger(int64_t& value) {
    uint8_t majorType = 0;
    uint64_t encoded = 0;
    size_t headSize = 0;
    if (!decodeHead(majorType, encoded, headSize)) return false;
    if ((majorType != 0 && majorType != 1)
        || encoded > static_cast<uint64_t>(INT64_MAX)) {
        fail(majorType > 1
            ? CompactCborStatus::TypeMismatch
            : CompactCborStatus::UnsupportedValue);
        return false;
    }
    value = majorType == 0
        ? static_cast<int64_t>(encoded)
        : -1 - static_cast<int64_t>(encoded);
    offset_ += headSize;
    return true;
}

bool CompactCborReader::readByteString(const uint8_t*& value, size_t& size) {
    uint64_t encodedSize = 0;
    if (!readHead(2, encodedSize)) return false;
    if (encodedSize > std::numeric_limits<size_t>::max()
        || !require(static_cast<size_t>(encodedSize))) {
        if (status_ == CompactCborStatus::Success) fail(CompactCborStatus::UnsupportedValue);
        return false;
    }
    size = static_cast<size_t>(encodedSize);
    value = input_ + offset_;
    offset_ += size;
    return true;
}

bool CompactCborReader::readText(const char*& value, size_t& size) {
    uint64_t encodedSize = 0;
    if (!readHead(3, encodedSize)) return false;
    if (encodedSize > std::numeric_limits<size_t>::max()
        || !require(static_cast<size_t>(encodedSize))) {
        if (status_ == CompactCborStatus::Success) fail(CompactCborStatus::UnsupportedValue);
        return false;
    }
    size = static_cast<size_t>(encodedSize);
    value = reinterpret_cast<const char*>(input_ + offset_);
    offset_ += size;
    return true;
}

bool CompactCborReader::enterArray(size_t& itemCount) {
    uint64_t encodedCount = 0;
    if (!readHead(4, encodedCount)) return false;
    if (encodedCount > std::numeric_limits<size_t>::max()) {
        fail(CompactCborStatus::UnsupportedValue);
        return false;
    }
    itemCount = static_cast<size_t>(encodedCount);
    return true;
}

bool CompactCborReader::enterMap(size_t& pairCount) {
    uint64_t encodedCount = 0;
    if (!readHead(5, encodedCount)) return false;
    if (encodedCount > std::numeric_limits<size_t>::max()) {
        fail(CompactCborStatus::UnsupportedValue);
        return false;
    }
    pairCount = static_cast<size_t>(encodedCount);
    return true;
}

bool CompactCborReader::readBoolean(bool& value) {
    if (!require(1)) return false;
    if (input_[offset_] != 0xF4 && input_[offset_] != 0xF5) {
        fail(CompactCborStatus::TypeMismatch);
        return false;
    }
    value = input_[offset_++] == 0xF5;
    return true;
}

bool CompactCborReader::readNull() {
    if (!require(1)) return false;
    if (input_[offset_] != 0xF6) {
        fail(CompactCborStatus::TypeMismatch);
        return false;
    }
    ++offset_;
    return true;
}

bool CompactCborReader::readDouble(double& value) {
    if (!require(9)) return false;
    if (input_[offset_] != 0xFB) {
        fail(CompactCborStatus::TypeMismatch);
        return false;
    }
    uint64_t bits = 0;
    for (size_t index = 0; index < 8; ++index) {
        bits = (bits << 8) | input_[offset_ + 1 + index];
    }
    std::memcpy(&value, &bits, sizeof(value));
    offset_ += 9;
    return true;
}

bool CompactCborReader::skipValue(size_t depth) {
    if (depth >= MaximumNestingDepth) {
        fail(CompactCborStatus::NestingTooDeep);
        return false;
    }
    if (!require(1)) return false;
    const uint8_t majorType = input_[offset_] >> 5;
    const uint8_t additional = input_[offset_] & 0x1F;
    if (majorType == 7) {
        size_t encodedSize = 0;
        if (additional < 24) encodedSize = 1;
        else if (additional == 24) {
            if (!require(2)) return false;
            if (input_[offset_ + 1] < 32) {
                fail(CompactCborStatus::NonCanonical);
                return false;
            }
            encodedSize = 2;
        } else if (additional == 25) encodedSize = 3;
        else if (additional == 26) encodedSize = 5;
        else if (additional == 27) encodedSize = 9;
        else {
            fail(CompactCborStatus::UnsupportedValue);
            return false;
        }
        if (!require(encodedSize)) return false;
        offset_ += encodedSize;
        return true;
    }

    uint8_t decodedMajorType = 0;
    uint64_t value = 0;
    size_t headSize = 0;
    if (!decodeHead(decodedMajorType, value, headSize)) return false;
    offset_ += headSize;
    if (decodedMajorType == 0 || decodedMajorType == 1) return true;
    if (decodedMajorType == 2 || decodedMajorType == 3) {
        if (value > std::numeric_limits<size_t>::max()
            || !require(static_cast<size_t>(value))) {
            if (status_ == CompactCborStatus::Success) fail(CompactCborStatus::UnsupportedValue);
            return false;
        }
        offset_ += static_cast<size_t>(value);
        return true;
    }
    if (decodedMajorType == 4 || decodedMajorType == 5) {
        if (decodedMajorType == 5 && value > UINT64_MAX / 2) {
            fail(CompactCborStatus::UnsupportedValue);
            return false;
        }
        const uint64_t itemCount = decodedMajorType == 5 ? value * 2 : value;
        for (uint64_t index = 0; index < itemCount; ++index) {
            if (!skipValue(depth + 1)) return false;
        }
        return true;
    }
    if (decodedMajorType == 6) return skipValue(depth + 1);
    fail(CompactCborStatus::UnsupportedValue);
    return false;
}

bool CompactCborReader::skip() {
    return skipValue(0);
}

bool CompactCborReader::peekMajorType(uint8_t& majorType) const {
    if (status_ != CompactCborStatus::Success || offset_ >= size_) return false;
    majorType = input_[offset_] >> 5;
    return true;
}

CompactCborStatus CompactCborReader::status() const {
    return status_;
}

size_t CompactCborReader::consumed() const {
    return offset_;
}

size_t CompactCborReader::remaining() const {
    return size_ - offset_;
}

const char* compactCborStatusName(CompactCborStatus status) {
    switch (status) {
        case CompactCborStatus::Success: return "Success";
        case CompactCborStatus::InvalidArgument: return "InvalidArgument";
        case CompactCborStatus::OutputTooSmall: return "OutputTooSmall";
        case CompactCborStatus::EndOfInput: return "EndOfInput";
        case CompactCborStatus::TypeMismatch: return "TypeMismatch";
        case CompactCborStatus::NonCanonical: return "NonCanonical";
        case CompactCborStatus::UnsupportedValue: return "UnsupportedValue";
        case CompactCborStatus::NestingTooDeep: return "NestingTooDeep";
        default: return "InvalidArgument";
    }
}

} // namespace EnvNode
