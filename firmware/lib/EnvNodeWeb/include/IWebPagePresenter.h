// EnvNode: see repository LICENSE.
#pragma once
#include <Arduino.h>
#include <functional>
#include "EnvNode/Locale.h"
namespace EnvNode {
class IWebPagePresenter {
public:
    virtual ~IWebPagePresenter() = default;
    virtual Locale webLocale() const = 0;
    // Synchronous presenters complete immediately; deferred hosts override this.
    virtual void afterResponse(std::function<void()> action) {action();}
    virtual void sendPage(const char* title, const char* activeRoute, const String& content,
        int status = 200, bool wideContent = false) = 0;
};
}
