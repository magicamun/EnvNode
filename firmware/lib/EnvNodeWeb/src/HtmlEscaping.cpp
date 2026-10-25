#include "HtmlEscaping.h"

namespace EnvNode {

String escapeHtml(const String& value) {
    String escaped;
    escaped.reserve(value.length() * 2 + 8);
    for (size_t index = 0; index < value.length(); ++index) {
        switch (value[index]) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += value[index]; break;
        }
    }
    return escaped;
}

} // namespace EnvNode
