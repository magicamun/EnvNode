// EnvNode: see repository LICENSE.
#include "WebPageFrame.h"
#include "HtmlEscaping.h"
namespace EnvNode {
String buildWebPageHeader(const WebPageFrameData& page) {
    String html;
    if (!html.reserve(2048)) return String();
    html = String("<!doctype html><html lang='") + escapeHtml(page.languageTag) + "'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>";
    html += escapeHtml(page.title);
    html += " · "; html += escapeHtml(page.productName);
    html += "</title><link rel='stylesheet' href='/style.css?build=";
    html += escapeHtml(page.buildTimestamp);
    html += "'>";
    html += page.uiScript();
    html += "</head><body><div class='shell'><aside class='side'><div class='brand'>";
    html += escapeHtml(page.deviceName);
    html += "</div><div class='version'>";
    html += escapeHtml(page.productName); html += " · v";
    html += escapeHtml(page.semanticVersion);
    html += "</div>";
    html += page.navigation();
    html += "</aside><main class='main";
    if (page.wideContent) html += " main-wide";
    html += "'><div class='top'><div><h1>";
    html += escapeHtml(page.title);
    html += "</h1>";
    if (page.setupAccessPoint) html += String("<p class='muted'>") + escapeHtml(page.setupMessage) + "</p>";
    html += "</div></div>";
    html += page.notices();
    return html;
}
const char* webPageFooter() { return "</main></div></body></html>"; }
}
