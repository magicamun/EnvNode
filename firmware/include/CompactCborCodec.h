#pragma once

#include <cstddef>
#include <cstdint>

namespace EnvNode {

enum class CompactCborStatus : uint8_t {
    Success,
    InvalidArgument,
    OutputTooSmall,
    EndOfInput,
    TypeMismatch,
    NonCanonical,
    UnsupportedValue,
    NestingTooDeep,
};

class CompactCborWriter {
public:
    CompactCborWriter(uint8_t* output, size_t capacity);

    bool writeUnsigned(uint64_t value);
    bool writeInteger(int64_t value);
    bool writeByteString(const uint8_t* value, size_t size);
    bool writeText(const char* value);
    bool writeText(const char* value, size_t size);
    bool beginArray(size_t itemCount);
    bool beginMap(size_t pairCount);
    bool writeBoolean(bool value);
    bool writeNull();
    bool writeDouble(double value);

    CompactCborStatus status() const;
    size_t size() const;

private:
    bool writeHead(uint8_t majorType, uint64_t value);
    bool append(const uint8_t* data, size_t size);

    uint8_t* output_;
    size_t capacity_;
    size_t size_ = 0;
    CompactCborStatus status_ = CompactCborStatus::Success;
};

class CompactCborReader {
public:
    static constexpr size_t MaximumNestingDepth = 16;

    CompactCborReader(const uint8_t* input, size_t size);

    bool readUnsigned(uint64_t& value);
    bool readInteger(int64_t& value);
    bool readByteString(const uint8_t*& value, size_t& size);
    bool readText(const char*& value, size_t& size);
    bool enterArray(size_t& itemCount);
    bool enterMap(size_t& pairCount);
    bool readBoolean(bool& value);
    bool readNull();
    bool readDouble(double& value);
    bool skip();

    CompactCborStatus status() const;
    size_t consumed() const;
    size_t remaining() const;

private:
    bool readHead(uint8_t expectedMajorType, uint64_t& value);
    bool decodeHead(uint8_t& majorType, uint64_t& value, size_t& headSize);
    bool skipValue(size_t depth);
    bool require(size_t size);
    void fail(CompactCborStatus status);

    const uint8_t* input_;
    size_t size_;
    size_t offset_ = 0;
    CompactCborStatus status_ = CompactCborStatus::Success;
};

const char* compactCborStatusName(CompactCborStatus status);

} // namespace EnvNode
