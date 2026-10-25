// EnvNode: see repository LICENSE.
#pragma once
#include <Arduino.h>
#include "EnvNode/Locale.h"
namespace EnvNode {
class IWebPagePresenter {
public:
    virtual ~IWebPagePresenter() = default;
    virtual Locale webLocale() const = 0;
    virtual void sendPage(const char* title, const char* activeRoute, const String& content,
        int status = 200, bool wideContent = false) = 0;
};
}
