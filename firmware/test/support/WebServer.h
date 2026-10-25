// EnvNode host test double: see repository LICENSE.
#pragma once
#include "Arduino.h"
#include <functional>
#include <string>
#include <vector>
enum HTTPMethod { HTTP_GET, HTTP_POST };
class WebServer {
public:
    struct Route { std::string path; HTTPMethod method; std::function<void()> handler; };
    uint16_t port;
    std::vector<Route> routes;
    bool started = false;
    int handled = 0, responseStatus = 0;
    size_t declaredLength = 0;
    std::string responseType, responseBody, cacheControl;
    std::vector<std::string> chunks;
    explicit WebServer(uint16_t value) : port(value) {}
    void on(const char* path, HTTPMethod method, std::function<void()> handler) {
        routes.push_back({path, method, handler});
    }
    void begin() { started = true; }
    void handleClient() { ++handled; }
    void setContentLength(size_t size) { declaredLength = size; }
    void send(int status, const char* type, const String& body) {
        responseStatus = status; responseType = type; responseBody = body.c_str();
        chunks.clear(); chunks.emplace_back(body.c_str());
    }
    void sendContent(const String& body) { responseBody += body.c_str(); chunks.emplace_back(body.c_str()); }
    void sendContent(const char* body, size_t size) { responseBody.append(body, size); chunks.emplace_back(body, size); }
    void sendHeader(const char* key, const char* value, bool = false) {
        if (std::string(key) == "Cache-Control") cacheControl = value;
    }
    void send_P(int status, const char* type, const char* body, size_t size) {
        responseStatus = status; responseType = type; responseBody.assign(body, size);
    }
    bool request(const char* path, HTTPMethod method) {
        for (auto& route : routes) if (route.path == path && route.method == method) { route.handler(); return true; }
        return false;
    }
};
