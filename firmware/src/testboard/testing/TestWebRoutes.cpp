// EnvNode: see repository LICENSE.
#include "TestWebRoutes.h"
#include "BoardAssets.h"
#include "TestPage.h"
#include <cstring>
#include "JsonWriter.h"
#include <limits>
namespace EnvNode { namespace Testing {
#include "TestPageScript.inc"
namespace {
const BoardAsset* requestedAsset(WebServer& server, const TestRunner& runner) {
    String id = server.arg("board");
    if (id.isEmpty()) id = runner.board() ? runner.board()->id : weatherBoard().id;
    for (const auto& asset : BoardAssets) if (id == asset.id) return &asset;
    return nullptr;
}
bool unsignedValue(const String& input, uint32_t& value) {
    if (!input.length()) return false;
    value = 0;
    for (size_t i = 0; i < input.length(); ++i) {
        const char c = input[i];
        if (c < '0' || c > '9' || value > (UINT32_MAX - uint32_t(c - '0')) / 10) return false;
        value = value * 10 + uint32_t(c - '0');
    }
    return true;
}
}
TestWebRoutes::TestWebRoutes(WebHost& host, IWebPagePresenter& pages, TestRunner& runner, SimulatedFixture& fixture)
    : host_(host), pages_(pages), fixture_(fixture), runner_(runner) {}
// Automatic reconnection is deliberately confined to this simulated fixture adapter.
void TestWebRoutes::tick(uint32_t now) {
    runner_.tick(now);
    if (!batch_) return;
    if (runner_.phase() == Phase::Review) runner_.next();
    if (runner_.phase() == Phase::AwaitingConnection) {
        fixture_.connect(*runner_.board(), runner_.step(), batchFault_);
        if (!runner_.start(now)) { runner_.abort(); batch_ = false; }
    }
    if (runner_.phase() == Phase::Complete || runner_.phase() == Phase::Aborted) batch_ = false;
}
void TestWebRoutes::registerRoutes(WebServer& server) {
    server.on("/test", HTTP_GET, [this]() { page(); });
    server.on("/test/catalog", HTTP_GET, [this]() {
        String json("[");
        for (size_t i = 0; i < boardCount(); ++i) {
            if (i) json += ',';
            const auto& board = *boardAt(i);
            json += "{\"id\":"; appendJsonString(json, board.id);
            json += ",\"name\":"; appendJsonString(json, board.name);
            json += ",\"identityRevision\":"; appendJsonString(json, board.identityRevision);
            json += ",\"pcbRevision\":"; appendJsonString(json, board.pcbRevision);
            json += '}';
        }
        json += ']';
        host_.server().send(200, "application/json; charset=utf-8", json);
    });
    server.on("/test/definition", HTTP_GET, [this]() {
        const auto* asset = requestedAsset(host_.server(), runner_);
        if (!asset) { host_.server().send(404, "application/json", "{\"error\":\"Unknown board\"}"); return; }
        host_.server().sendHeader("Cache-Control", "no-store");
        host_.server().send_P(200, "application/json; charset=utf-8", asset->json, asset->jsonSize);
    });
    server.on("/test/board.svg", HTTP_GET, [this]() {
        const auto* asset = requestedAsset(host_.server(), runner_);
        if (!asset) { host_.server().send(404, "text/plain", "Unknown board"); return; }
        host_.server().sendHeader("Content-Encoding", "gzip");
        host_.server().sendHeader("Cache-Control", "no-cache");
        host_.server().send_P(200, "image/svg+xml", reinterpret_cast<const char*>(asset->svg), asset->svgSize);
    });
    server.on("/test/ui.js", HTTP_GET, [this]() {
        host_.server().sendHeader("Cache-Control", "no-cache");
        host_.server().send_P(200, "application/javascript; charset=utf-8", TestPageScript, sizeof(TestPageScript) - 1);
    });
    server.on("/test/state", HTTP_GET, [this]() { state(); });
    server.on("/test/action", HTTP_POST, [this]() { action(); });
}
void TestWebRoutes::page() {
    pages_.sendPage(pages_.webLocale() == Locale::GermanGermany ? "Anschlussprüfung" : "Connector test",
        "/test", buildTestPage(pages_.webLocale()), 200, true);
}
void TestWebRoutes::state() {
    String json;
    if (!json.reserve(1400)) {
        host_.server().send(503, "application/json", "{\"error\":\"Status buffer unavailable\"}");
        return;
    }
    json = "{\"simulation\":true,\"board\":";
    appendJsonString(json, runner_.board() ? runner_.board()->id : "");
    json += String(",\"batch\":") + (batch_ ? "true" : "false");
    json += String(",\"generation\":") + runner_.generation()
        + ",\"phase\":" + static_cast<unsigned>(runner_.phase())
        + ",\"step\":" + static_cast<unsigned>(runner_.step())
        + ",\"completed\":" + static_cast<unsigned>(runner_.completed())
        + ",\"overall\":" + static_cast<unsigned>(runner_.overall()) + ",\"results\":\"";
    const auto* connector = runner_.connector();
    if (connector) for (size_t i = 0; i < connector->count * SocketPins; ++i)
        json += static_cast<char>('0' + static_cast<unsigned>(runner_.result(i)));
    json += "\",\"verdicts\":[";
    if (runner_.board()) for (size_t i = 0; i < runner_.board()->count; ++i) {
        if (i) json += ',';
        json += String(static_cast<unsigned>(runner_.verdict(i)));
    }
    json += "],\"contacts\":\"";
    if (runner_.board()) for (size_t i = 0; i < runner_.board()->count; ++i)
        for (size_t p = 0; p < MaxContacts; ++p)
            json += static_cast<char>('0' + static_cast<unsigned>(runner_.contactVerdict(i, p)));
    json += "\"}";
    host_.server().sendHeader("Cache-Control", "no-store");
    host_.server().send(200, "application/json; charset=utf-8", json);
}
void TestWebRoutes::action() {
    auto& server = host_.server(); uint32_t generation;
    if (!unsignedValue(server.arg("generation"), generation) || generation != runner_.generation()) {
        server.send(409, "application/json", "{\"error\":\"Stale session state; refresh status\"}"); return;
    }
    const String command = server.arg("action");
    bool ok = false;
    if (batch_ && command != "abort") {
        server.send(409, "application/json", "{\"error\":\"Simulation batch is running\"}"); return;
    }
    if (command == "all" && runner_.board() && runner_.phase() != Phase::Running) {
        const String fault = server.arg("fault");
        if (fault != "none" && fault != "open" && fault != "short") {
            server.send(400, "application/json", "{\"error\":\"Unknown simulation fault\"}"); return;
        }
        batchFault_ = fault == "open" ? SimulatedFault::OpenContact
            : fault == "short" ? SimulatedFault::ExtraConnection : SimulatedFault::None;
        const auto* board = runner_.board();
        ok = runner_.load(*board);
        batch_ = ok;
        if (ok) tick(millis());
    } else if (command == "load") {
        const auto* board = findBoard(server.arg("board").c_str());
        ok = board && runner_.load(*board);
    } else if (command == "start" && (runner_.phase() == Phase::AwaitingConnection || runner_.phase() == Phase::Review)) {
        const String fault = server.arg("fault");
        if (fault != "none" && fault != "open" && fault != "short") {
            server.send(400, "application/json", "{\"error\":\"Unknown simulation fault\"}"); return;
        }
        fixture_.connect(*runner_.board(), runner_.step(), fault == "open" ? SimulatedFault::OpenContact
            : fault == "short" ? SimulatedFault::ExtraConnection : SimulatedFault::None);
        ok = runner_.start(millis());
    } else if (command == "select") {
        uint32_t connector;
        ok = unsignedValue(server.arg("connector"), connector) && runner_.selectConnector(connector);
    } else if (command == "next") ok = runner_.next();
    else if (command == "abort" && runner_.phase() != Phase::Empty && runner_.phase() != Phase::Complete && runner_.phase() != Phase::Aborted) {
        batch_ = false; runner_.abort(); ok = true;
    }
    if (!ok) { server.send(409, "application/json", "{\"error\":\"Action not allowed in current step\"}"); return; }
    state();
}
} }
