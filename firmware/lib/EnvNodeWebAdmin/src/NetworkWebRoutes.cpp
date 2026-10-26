// EnvNode: see repository LICENSE.
#include "NetworkWebRoutes.h"
#include "HtmlEscaping.h"
namespace EnvNode {
NetworkWebRoutes::NetworkWebRoutes(WebHost& host, INetworkSettings& settings,
    IWiFiService& wifi, IWebPagePresenter& pages, INetworkSaveObserver& observer)
    : host_(host), settings_(settings), wifiService_(wifi), pages_(pages), observer_(observer) {}
void NetworkWebRoutes::registerRoutes(WebServer& server) {
    server.on("/network", HTTP_GET, [this]() { handleNetwork(); });
    server.on("/network/save", HTTP_POST, [this]() { handleNetworkSave(); });
}
const char* NetworkWebRoutes::uiText(TextId id) const { return webText(pages_.webLocale(), id); }
void NetworkWebRoutes::handleNetwork() {
    const NetworkConfiguration& n = settings_.getNetworkConfiguration();
    const bool isStatic = n.addressMode == NetworkAddressMode::Static;
    String c;
    c.reserve(2200);
    c = (String("<section class='card'><h2>") + uiText(TextId::NetworkConfiguration) + "</h2><form method='post' action='/network/save' autocomplete='off'>");
    c += (String("<label>") + uiText(TextId::Hostname) + "<input name='hostname' required value='") + escapeHtml(n.hostname) + (String("'></label><label>") + uiText(TextId::WiFiSSID) + "<input name='wifiSSID' required value='") + escapeHtml(n.wifiSSID) + "'></label>";
    c += (String("<label class='choice'><input type='checkbox' name='changeWifiPassword' value='1'>") + uiText(TextId::ChangeWiFiPassword) + "</label><label>" + uiText(TextId::NewWiFiPassword) + "<input type='password' name='wifiPassword' autocomplete='new-password'></label>");
    c += (String("<label>") + uiText(TextId::AddressMode) + "<select name='addressMode' id='addressMode' onchange='toggleStatic()'><option value='dhcp'") + String(!isStatic?" selected":"") + ">DHCP</option><option value='static'" + String(isStatic?" selected":"") + (String(">") + uiText(TextId::StaticIPv4) + "</option></select></label>");
    const NetworkAddressSnapshot assigned = wifiService_.stationAddresses();
    c += "<div id='dhcpFields'>";
    c += String("<p class='help'>") + uiText(TextId::ActiveNetworkAddresses) + "</p>";
    const auto assignedField = [&](TextId label, const String& value) {
        c += String("<label>") + uiText(label) + "<input readonly value='";
        c += value.isEmpty() || value == "0.0.0.0" ? String("—") : escapeHtml(value);
        c += "'></label>";
    };
    assignedField(TextId::IPv4Address, assigned.ipAddress);
    assignedField(TextId::SubnetMask, assigned.subnetMask);
    assignedField(TextId::DefaultGateway, assigned.gateway);
    assignedField(TextId::PrimaryDNS, assigned.dns1);
    assignedField(TextId::SecondaryDNS, assigned.dns2);
    c += "</div>";
    c += (String("<div id='staticFields'><label>") + uiText(TextId::IPv4Address) + "<input name='ipv4Address' value='") + escapeHtml(n.ipv4Address) + (String("'></label><label>") + uiText(TextId::SubnetMask) + "<input name='subnetMask' value='") + escapeHtml(n.subnetMask) + (String("'></label><label>") + uiText(TextId::DefaultGateway) + "<input name='gateway' value='") + escapeHtml(n.gateway) + (String("'></label><label>") + uiText(TextId::PrimaryDNS) + "<input name='dns1' value='") + escapeHtml(n.dns1) + (String("'></label><label>") + uiText(TextId::SecondaryDNS) + " " + "<span class='help'>" + uiText(TextId::Optional) + "</span><input name='dns2' value='") + escapeHtml(n.dns2) + (String("'></label></div><div class='actions'><button>") + uiText(TextId::SaveNetworkSettings) + "</button></div></form></section>");
    c += "<script>function toggleStatic(){var s=document.getElementById('addressMode').value==='static',d=document.getElementById('staticFields');d.hidden=!s;document.getElementById('dhcpFields').hidden=s;d.querySelectorAll('input').forEach(function(i){i.disabled=!s})}toggleStatic()</script>";
    pages_.sendPage(uiText(TextId::Network), "/network", c);
}
void NetworkWebRoutes::handleNetworkSave() {
    NetworkConfiguration n = settings_.getNetworkConfiguration();
    n.hostname=host_.server().arg("hostname"); n.wifiSSID=host_.server().arg("wifiSSID"); n.addressMode=host_.server().arg("addressMode")=="static"?NetworkAddressMode::Static:NetworkAddressMode::Dhcp;
    n.ipv4Address=host_.server().arg("ipv4Address"); n.subnetMask=host_.server().arg("subnetMask"); n.gateway=host_.server().arg("gateway"); n.dns1=host_.server().arg("dns1"); n.dns2=host_.server().arg("dns2");
    const bool updatePassword=host_.server().arg("changeWifiPassword")=="1"; if(updatePassword)n.wifiPassword=host_.server().arg("wifiPassword");
    observer_.networkSettingsSaved(settings_.setNetworkConfiguration(n, updatePassword));
}
}
