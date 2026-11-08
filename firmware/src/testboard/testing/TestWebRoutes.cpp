// EnvNode: see repository LICENSE.
#include "TestWebRoutes.h"
#include "BoardAssets.h"
#include "TestPage.h"
#include <cstring>
#include "JsonWriter.h"
#include <limits>
#include <algorithm>
#include "../TestBoardFirmwareVersion.h"
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
void TestWebRoutes::registerRoutes(WebServer& server) {
    server.on("/test", HTTP_GET, [this]() { page(); });
    server.on("/test/catalog", HTTP_GET, [this]() {
        String json("[");
        const size_t count = profiles_ ? profiles_->count() : boardCount();
        for (size_t i = 0; i < count; ++i) {
            if (i) json += ',';
            RemoteProfileEntry e{}; bool remote=false;
            if (profiles_) { if (!profiles_->entryAt(i,e,remote)) continue; }
            else {
                const auto& b=*boardAt(i);
                strncpy(e.id,b.id,sizeof(e.id)-1); strncpy(e.name,b.name,sizeof(e.name)-1);
                strncpy(e.identity,b.identityRevision,sizeof(e.identity)-1); strncpy(e.pcb,b.pcbRevision,sizeof(e.pcb)-1);
            }
            json += "{\"id\":"; appendJsonString(json,e.id);
            json += ",\"name\":"; appendJsonString(json,e.name);
            json += ",\"identityRevision\":"; appendJsonString(json,e.identity);
            json += ",\"pcbRevision\":"; appendJsonString(json,e.pcb);
            json += remote ? ",\"origin\":\"remote\"}" : ",\"origin\":\"embedded\"}";
        }
        json += ']';
        host_.server().send(200, "application/json; charset=utf-8", json);
    });
    server.on("/test/definition", HTTP_GET, [this]() {
        String id=host_.server().arg("board");
        if(id.isEmpty()) id=runner_.board()?runner_.board()->id:weatherBoard().id;
        const bool builtinActive=runner_.board() && runner_.board()==findBoard(id.c_str());
        if(profiles_ && !builtinActive) {
            const auto* p=profiles_->prepare(id.c_str(),runner_.phase()!=Phase::Running && !(hardwareBusy_ && hardwareBusy_()));
            if(p) {
                const String json=profileJson(*p);
                if(json.isEmpty()) { host_.server().send(503,"application/json","{\"error\":\"Profile buffer unavailable\"}");return; }
                host_.server().sendHeader("Cache-Control","no-store");
                host_.server().send(200,"application/json; charset=utf-8",json);return;
            }
        }
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
    server.on("/test/report", HTTP_GET, [this]() { reportContact(); });
    server.on("/test/state", HTTP_GET, [this]() { state(); });
    server.on("/test/action", HTTP_POST, [this]() { action(); });
}
void TestWebRoutes::page() {
    pages_.sendPage(pages_.webLocale() == Locale::GermanGermany ? "Anschlussprüfung" : "Connector test",
        "/test", buildTestPage(pages_.webLocale(), measurement_ && measurement_->physicalAvailable()), 200, true);
}
void TestWebRoutes::state() {
    String json;
    if (!json.reserve(1400)) {
        host_.server().send(503, "application/json", "{\"error\":\"Status buffer unavailable\"}");
        return;
    }
    json = String("{\"simulation\":")+((!measurement_||measurement_->simulated())?"true":"false");
    json += String(",\"physicalAvailable\":")+((measurement_&&measurement_->physicalAvailable())?"true":"false");
    json+=",\"fixture\":";appendJsonString(json,selection_?selection_->definition().id:fixtureRevision01().id);
    json+=",\"controllerProfile\":";appendJsonString(json,selection_?selection_->controller().id:"controller-0.1");
    json+=",\"adapterProfile\":";appendJsonString(json,selection_?selection_->adapter().id:"input-0.1");
    json+=",\"fixtureSelectable\":";json+=selection_?"true":"false";
    json+=",\"board\":";
    appendJsonString(json, runner_.board() ? runner_.board()->id : "");
    json += String(",\"generation\":") + runner_.generation()
        + ",\"phase\":" + static_cast<unsigned>(runner_.phase())
        + ",\"step\":" + static_cast<unsigned>(runner_.step())
        + ",\"completed\":" + static_cast<unsigned>(runner_.completed())
        + ",\"overall\":" + static_cast<unsigned>(runner_.overall()) + ",\"results\":\"";
    const auto* connector = runner_.connector();
    if (connector) for (size_t i = 0; i < connector->count * runner_.board()->inputCount; ++i)
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
    json += "\",\"profilesRevision\":" + String(profiles_?profiles_->revision():0);
    json += ",\"profilesStatus\":";appendJsonString(json,profiles_?profiles_->status():"embedded");
    json += ",\"measurementPolicy\":";
    appendJsonString(json,runner_.policy()?runner_.policy()->id:"");
    json += ",\"directionChecks\":[";
    if(runner_.policy())for(size_t i=0;i<runner_.policy()->count;++i) {
        if(i)json+=',';
        json+="{\"from\":";appendJsonString(json,runner_.policy()->rules[i].from);
        json+=",\"to\":";appendJsonString(json,runner_.policy()->rules[i].to);
        json+=",\"reverse\":"+String(runner_.reverseEvidence(i));
        json+=",\"reverseSource\":";appendJsonString(json,runner_.policy()->rules[i].reverseSource);
        json+="}";
    }
    json+="]}";
    host_.server().sendHeader("Cache-Control", "no-store");
    host_.server().send(200, "application/json; charset=utf-8", json);
}
#include "TestReport.inc"
void TestWebRoutes::action() {
    auto& server = host_.server(); uint32_t generation;
    if (!unsignedValue(server.arg("generation"), generation) || generation != runner_.generation()) {
        server.send(409, "application/json", "{\"error\":\"Stale session state; refresh status\"}"); return;
    }
    if (hardwareBusy_ && hardwareBusy_()) {
        server.send(409, "application/json", "{\"error\":\"Hardwarediagnose zuerst abschalten\"}"); return;
    }
    const String command = server.arg("action");
    bool ok = false;
    if(command=="fixture"&&selection_&&runner_.phase()!=Phase::Running) {
        // Validate first. A rejected profile cannot disturb the current session.
        FixtureSelection candidate;
        if(server.arg("confirm")!="yes"||!candidate.select(server.arg("controller").c_str(),server.arg("adapter").c_str())) {
            server.send(400,"application/json","{\"error\":\"Confirm disconnected target and valid hardware profiles\"}");return;
        }
        if(persistSelection_&&!persistSelection_(candidate.code())) {server.send(503,"application/json","{\"error\":\"Hardware profile could not be saved\"}");return;}
        if(candidate.code()==selection_->code()){state();return;}
        fixture_.release();if(measurement_)measurement_->release();
        ok=selection_->select(candidate.controller().id,candidate.adapter().id)&&runner_.resetForModeChange();
        if(ok&&selectionChanged_)selectionChanged_();
    } else if(command=="mode" && measurement_ && runner_.phase()!=Phase::Running) {
        const String mode=server.arg("mode");
        if(mode!="simulation"&&mode!="physical") {server.send(400,"application/json","{\"error\":\"Unknown measurement mode\"}");return;}
        ok=measurement_->setSimulation(mode=="simulation")&&runner_.resetForModeChange();
    } else if (command == "load") {
        const auto* board = findBoard(server.arg("board").c_str());
        ok = profiles_ ? profiles_->load(server.arg("board").c_str(),runner_) : board && runner_.load(*board);
    } else if (command == "start" && (runner_.phase() == Phase::AwaitingConnection || runner_.phase() == Phase::Review)) {
        const String fault = server.arg("fault");
        const bool simulation=!measurement_||measurement_->simulated();
        if(!simulation&&server.arg("confirm")!="yes") {server.send(400,"application/json","{\"error\":\"Confirm unpowered DUT, removed ESP32 and disconnected provisioning cable\"}");return;}
        if (simulation && fault != "none" && fault != "open" && fault != "short" && fault != "diode-short") {
            server.send(400, "application/json", "{\"error\":\"Unknown simulation fault\"}"); return;
        }
        if(simulation && fault=="diode-short" && !runner_.policy()) {
            server.send(400,"application/json","{\"error\":\"No reviewed directed paths for this profile\"}");return;
        }
        if(simulation) fixture_.connect(*runner_.board(), runner_.step(), fault == "open" ? SimulatedFault::OpenContact
            : fault == "short" ? SimulatedFault::ExtraConnection : fault=="diode-short"?SimulatedFault::DirectedShort:SimulatedFault::None);
        ok = runner_.start(millis());
    } else if(command=="refresh" && profiles_ && runner_.phase()!=Phase::Running) {
        profiles_->requestRefresh();ok=true;
    } else if (command == "select") {
        uint32_t connector;
        ok = unsignedValue(server.arg("connector"), connector) && runner_.selectConnector(connector);
    } else if (command == "next") ok = runner_.next();
    else if (command == "abort" && runner_.phase() != Phase::Empty && runner_.phase() != Phase::Complete && runner_.phase() != Phase::Aborted) {
        runner_.abort(); ok = true;
    }
    if (!ok) { server.send(409, "application/json", "{\"error\":\"Action not allowed in current step\"}"); return; }
    state();
}
} }
