// EnvNode: see repository LICENSE.
#include "WebResult.h"
#include "WebLocalization.h"
#include "HtmlEscaping.h"
#include <cstring>
namespace EnvNode {
void sendWebResult(IWebPagePresenter& pages, const char* title, const char* route, const char* message, bool success) {
    String content;
    content.reserve(240 + strlen(title) + strlen(message) + strlen(route));
    content = "<div class='notice ";
    content += success ? "success" : "error";
    content += "'><strong>";
    content += escapeHtml(title);
    content += "</strong><p>";
    content += escapeHtml(message);
    content += "</p></div><a class='button' href='";
    content += route;
    content += (String("'>") + webText(pages.webLocale(), TextId::Back) + "</a>");
    pages.sendPage(title, route, content, success ? 200 : 400);
}
}
