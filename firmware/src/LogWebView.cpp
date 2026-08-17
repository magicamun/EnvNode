#include "LogWebView.h"

#include "HtmlEscaping.h"
#include "LogEntryFormatting.h"

namespace EnvNode {
namespace {

const char* levelBadgeClass(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "badge log-level debug";
        case LogLevel::Info: return "badge log-level info";
        case LogLevel::Warn: return "badge warn log-level";
        case LogLevel::Error: return "badge bad log-level";
        default: return "badge log-level";
    }
}

} // namespace

String buildRecentLogHtml(const IRecentLogReader& logReader) {
    const size_t entryCount = logReader.count();
    String html;
    html.reserve(720 + entryCount * 390);
    html = "<section class='card'><div class='log-summary'><div><strong>Recent in-memory log entries: ";
    html += String(static_cast<unsigned int>(entryCount)).c_str();
    html += " / ";
    html += String(static_cast<unsigned int>(logReader.capacity())).c_str();
    html += "</strong><p class='help'>Recent in-memory diagnostic history. Oldest entries are overwritten when the buffer is full. Logs do not persist across restart.</p></div><a class='button' href='/logs'>Refresh</a></div>";

    if (entryCount == 0) {
        html += "<p class='muted log-empty'>No log entries available.</p></section>";
        return html;
    }

    html += "<div class='table-scroll log-table-wrap'><table class='log-table'><thead><tr><th class='log-seq'>SEQ</th><th class='log-time'>TIME</th><th class='log-level-cell'>LEVEL</th><th class='log-message'>MESSAGE</th></tr></thead><tbody>";
    for (size_t offset = 0; offset < entryCount; ++offset) {
        LogEntry entry;
        if (!logReader.copyEntry(entryCount - offset - 1, entry)) continue;
        char timestamp[24];
        const bool timestampValid = formatLogEntryTimestamp(entry, timestamp, sizeof(timestamp));
        html += "<tr><td class='log-seq'>#";
        html += String(static_cast<unsigned int>(entry.sequence)).c_str();
        html += "</td><td class='log-time'>";
        html += timestampValid ? timestamp : "—";
        html += "</td><td class='log-level-cell'><span class='";
        html += levelBadgeClass(entry.level);
        html += "'>";
        html += logLevelDisplayName(entry.level);
        html += "</span></td><td class='log-message'>";
        html += escapeHtml(String(entry.message)).c_str();
        html += "</td></tr>";
    }
    html += "</tbody></table></div></section>";
    return html;
}

} // namespace EnvNode
