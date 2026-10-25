#include "HardwareDescriptor.h"

#include <cstring>

namespace EnvNode {

bool DescriptorTextView::equals(const char* value) const {
    if (value == nullptr || data == nullptr) return false;
    const size_t valueSize = std::strlen(value);
    return size == valueSize && std::memcmp(data, value, size) == 0;
}

} // namespace EnvNode
