// EnvNode: see repository LICENSE.
#pragma once
#include <Arduino.h>
namespace EnvNode {
class IWebPageHeaderPresenter {
public:
    virtual ~IWebPageHeaderPresenter() = default;
    virtual String renderPageHeader(const char* title, const char* active, bool wideContent) const = 0;
};
}
