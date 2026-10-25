// EnvNode: see repository LICENSE.
#pragma once
#include "IWebPagePresenter.h"
namespace EnvNode {
void sendWebResult(IWebPagePresenter& pages, const char* title, const char* route, const char* message, bool success);
}
