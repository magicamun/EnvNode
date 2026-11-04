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
    // Borrowed static/flash content must outlive the response. False: unsupported.
    virtual bool sendStaticContent(const char*, const char*) {return false;}
    virtual void sendPage(const char* title, const char* activeRoute, String&& content,
        int status = 200, bool wideContent = false) {
        sendPage(title,activeRoute,static_cast<const String&>(content),status,wideContent);
    }
    // Synchronous presenters complete immediately; deferred hosts override this.
    virtual void afterResponse(std::function<void()> action) {action();}
    virtual void sendPage(const char* title, const char* activeRoute, const String& content,
        int status = 200, bool wideContent = false) = 0;
};
}
