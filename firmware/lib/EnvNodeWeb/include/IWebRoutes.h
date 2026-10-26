// EnvNode: see repository LICENSE.
#pragma once
#include <WebServer.h>
namespace EnvNode {
class IWebRoutes {
public:
    virtual ~IWebRoutes() = default;
    virtual void registerRoutes(WebServer& server) = 0;
};
}
