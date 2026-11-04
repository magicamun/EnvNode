// EnvNode-owned: see repository LICENSE.
#include "WebService.h"
#include "NetworkLoggingService.h"
#include "SystemDiagnostics.h"
#include "HtmlEscaping.h"
namespace EnvNode {
void WebService::handleNetworkLogging() {
    const bool de=configurationService_.getLocale()==Locale::GermanGermany;
    const auto& c=configurationService_.getConfiguration().networkLogging;
    String html="<section class='card'><h2>UDP / rsyslog</h2><form method='post' action='/network-logging/save'>";
    const auto choice=[&](const char* name,const char* label,bool checked){
        html+="<label class='choice'><input type='checkbox' name='"+String(name)+"' value='1'"+(checked?" checked":"")+">"+label+"</label>";
    };
    choice("enabled",de?"Netzwerklogging aktiv":"Enable network logging",c.enabled);
    html+="<label>"+String(de?"Empfänger IPv4":"Receiver IPv4")+"<input name='address' maxlength='15' value='"+escapeHtml(c.address)+"'></label>";
    html+="<label>UDP-Port<input name='port' type='number' min='1' max='65535' value='"+String(c.port)+"'></label>";
    choice("systemLog",de?"Systemlogs senden":"Send system logs",c.systemLog);
    choice("systemDiagnostics",de?"Erweiterte Systemdiagnose":"Extended system diagnostics",c.systemDiagnostics);
    html+="<button type='submit'>"+String(de?"Speichern":"Save")+"</button></form><p>";
    html+=de?"Sensor-, Aktor- und Controllerdiagnosen bleiben in den jeweiligen Einstellungen wählbar. UDP bestätigt keinen Empfang; Ausfälle und volle Puffer verursachen Aufzeichnungslücken.":"Select sensor, actuator and controller diagnostics in their own settings. UDP does not confirm delivery; outages and full buffers cause recording gaps.";
    html+="</p><p>"+escapeHtml(networkLoggingStatus())+"</p><p>"+escapeHtml(extendedDiagnosticsStatus())+"</p>";
    html+="<form method='post' action='/network-logging/test'><button type='submit'>"+String(de?"Diagnoseereignis erzeugen":"Generate diagnostic event")+"</button></form></section>";
    sendPage("UDP / rsyslog","/network-logging",std::move(html));
}
void WebService::handleNetworkLoggingSave() {
    if(!administrationAvailable()){server_.send(503,"text/plain","Unavailable");return;}
    NetworkLoggingConfiguration c;c.enabled=server_.hasArg("enabled");c.systemLog=server_.hasArg("systemLog");c.systemDiagnostics=server_.hasArg("systemDiagnostics");c.address=server_.arg("address");
    const String port=server_.arg("port");uint32_t n=0;bool valid=!port.isEmpty()&&port.length()<=5;
    for(size_t i=0;i<port.length();++i){if(port[i]<'0'||port[i]>'9')valid=false;else n=n*10+port[i]-'0';}
    if(!valid||n==0||n>65535||!validNetworkLogging(c)){ // Validate port below before persisting.
        server_.send(400,"text/plain","Invalid numeric IPv4 destination or UDP port");return;
    }
    c.port=static_cast<uint16_t>(n);
    if(!configurationService_.setNetworkLoggingConfiguration(c)){server_.send(503,"text/plain","Settings not saved");return;}
    server_.sendHeader("Location","/network-logging",true);server_.send(303,"text/plain","");
}
void WebService::handleNetworkLoggingTest() {
    if(!administrationAvailable()){server_.send(503,"text/plain","Unavailable");return;}
    const bool accepted=recordDiagnosticTest()==DiagnosticRecordResult::Accepted;
    server_.send(accepted?200:503,"text/plain",accepted?"Accepted into RAM queue; delivery is not confirmed":"Not accepted; enable system diagnostics and check local logger status");
}
}
