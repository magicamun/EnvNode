// EnvNode: see repository LICENSE.
#pragma once
#include "EnvNode/Locale.h"
namespace EnvNode {
class ILocaleConfiguration {
public:
    virtual ~ILocaleConfiguration() = default;
    virtual Locale getLocale() const = 0;
};
}
