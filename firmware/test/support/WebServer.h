// EnvNode host test double: see repository LICENSE.
#pragma once
#include "Arduino.h"
#include <functional>
#include <string>
#include <vector>
#include <map>
enum HTTPMethod { HTTP_GET, HTTP_POST };
enum HTTPUploadStatus { UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END, UPLOAD_FILE_ABORTED };
struct HTTPUpload { HTTPUploadStatus status = UPLOAD_FILE_START; String name, filename; uint8_t buf[2048] = {}; size_t currentSize = 0; };
class WebServer {
public:
    struct Route { std::string path; HTTPMethod method; std::function<void()> handler; std::function<void()> uploadHandler; };
    struct Client { int flushes = 0; void flush() { ++flushes; } } clientState;
    Client& client() { return clientState; }
    HTTPUpload uploadState;
    HTTPUpload& upload() { return uploadState; }
    void uploadEvent(const char* path) { for (auto& route : routes) if (route.path == path && route.uploadHandler) route.uploadHandler(); }
    void on(const char* path, HTTPMethod method, std::function<void()> handler, std::function<void()> uploadHandler) { routes.push_back({path, method, handler, uploadHandler}); }
    std::map<std::string, String> arguments;
    bool hasArg(const char* name) const { return arguments.count(name) != 0; }
    String arg(const char* name) const {
        const auto found = arguments.find(name);
        return found == arguments.end() ? String() : found->second;
    }
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
