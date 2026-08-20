#include "LogWebView.h"

#include "HtmlEscaping.h"
#include "JsonWriter.h"
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

void appendLogRows(String& html, const IRecentLogReader& reader, size_t count) {
    for (size_t offset = 0; offset < count; ++offset) {
        LogEntry entry;
        if (!reader.copyEntry(count - offset - 1, entry)) continue;
        char timestamp[24];
        const bool validTime = formatLogEntryTimestamp(entry, timestamp, sizeof(timestamp));
        html += "<tr><td class='log-seq'>#";
        html += String(static_cast<unsigned int>(entry.sequence)).c_str();
        html += "</td><td class='log-time'>";
        html += validTime ? timestamp : "—";
        html += "</td><td class='log-level-cell'><span class='";
        html += levelBadgeClass(entry.level);
        html += "'>";
        html += logLevelDisplayName(entry.level);
        html += "</span></td><td class='log-message'>";
        html += escapeHtml(String(entry.message)).c_str();
        html += "</td></tr>";
    }
}

} // namespace

String buildRecentLogHtml(const IRecentLogReader& reader) {
    const size_t count = reader.count();
    LogEntry newestEntry;
    const bool hasNewest = count > 0 && reader.copyEntry(count - 1, newestEntry);
    String html;
    html.reserve(2900 + count * 390);
    html = "<section class='card' id='log-view'><div class='log-summary'><div><strong>Recent in-memory log entries: <span id='log-count'>";
    html += String(static_cast<unsigned int>(count)).c_str();
    html += " / ";
    html += String(static_cast<unsigned int>(reader.capacity())).c_str();
    html += "</span></strong><p class='help'>Recent in-memory diagnostic history. Oldest entries are overwritten when the buffer is full. Logs do not persist across restart.</p></div><div class='log-refresh-controls'><label class='choice'><input id='log-auto-refresh' type='checkbox' checked>Auto refresh</label><span class='help' id='log-refresh-status'>On · 2 s</span><a class='button' href='/logs'>Refresh</a></div></div><div id='log-content' data-newest-sequence='";
    html += String(static_cast<unsigned int>(hasNewest ? newestEntry.sequence : 0)).c_str();
    html += "'>";
    if (count == 0) {
        html += "<p class='muted log-empty'>No log entries available.</p>";
    } else {
        html += "<div class='table-scroll log-table-wrap'><table class='log-table'><thead><tr><th class='log-seq'>SEQ</th><th class='log-time'>TIME</th><th class='log-level-cell'>LEVEL</th><th class='log-message'>MESSAGE</th></tr></thead><tbody>";
        appendLogRows(html, reader, count);
        html += "</tbody></table></div>";
    }
    html += "</div><script>(()=>{const intervalMs=2000,box=document.getElementById('log-auto-refresh'),status=document.getElementById('log-refresh-status'),content=document.getElementById('log-content'),count=document.getElementById('log-count');let newest=Number(content.dataset.newestSequence)||0,currentCount=Number(count.textContent.split('/')[0])||0;const levelClass={DEBUG:'badge log-level debug',INFO:'badge log-level info',WARN:'badge warn log-level',ERROR:'badge bad log-level'};function cell(row,className,text){const value=document.createElement('td');value.className=className;value.textContent=text;row.appendChild(value);return value}function render(data){count.textContent=data.count+' / '+data.capacity;content.dataset.newestSequence=data.entries.length?data.entries[0].sequence:0;if(!data.entries.length){const empty=document.createElement('p');empty.className='muted log-empty';empty.textContent='No log entries available.';content.replaceChildren(empty);return}const wrap=document.createElement('div');wrap.className='table-scroll log-table-wrap';const table=document.createElement('table');table.className='log-table';const head=document.createElement('thead'),headRow=document.createElement('tr');for(const title of ['SEQ','TIME','LEVEL','MESSAGE']){const th=document.createElement('th');th.textContent=title;headRow.appendChild(th)}head.appendChild(headRow);const body=document.createElement('tbody');for(const entry of data.entries){const row=document.createElement('tr');cell(row,'log-seq','#'+entry.sequence);cell(row,'log-time',entry.time);const levelCell=cell(row,'log-level-cell',''),badge=document.createElement('span');badge.className=levelClass[entry.level]||'badge log-level';badge.textContent=entry.level;levelCell.appendChild(badge);cell(row,'log-message',entry.message);body.appendChild(row)}table.append(head,body);wrap.appendChild(table);content.replaceChildren(wrap)}async function refresh(){if(!box.checked)return;try{const response=await fetch('/logs/data',{cache:'no-store'});if(!response.ok)throw new Error();const data=await response.json(),next=data.entries.length?data.entries[0].sequence:0;if(next!==newest||data.count!==currentCount){render(data);newest=next;currentCount=data.count}status.textContent='On · 2 s'}catch(error){status.textContent='On · 2 s · refresh failed'}}box.addEventListener('change',()=>{status.textContent=box.checked?'On · 2 s':'Paused';if(box.checked)refresh()});setInterval(refresh,intervalMs)})();</script></section>";
    return html;
}

String buildRecentLogJson(const IRecentLogReader& reader) {
    const size_t count = reader.count();
    String json;
    json.reserve(64 + count * 350);
    json = "{\"count\":";
    json += String(static_cast<unsigned int>(count)).c_str();
    json += ",\"capacity\":";
    json += String(static_cast<unsigned int>(reader.capacity())).c_str();
    json += ",\"entries\":[";
    bool first = true;
    for (size_t offset = 0; offset < count; ++offset) {
        LogEntry entry;
        if (!reader.copyEntry(count - offset - 1, entry)) continue;
        char timestamp[24];
        const bool validTime = formatLogEntryTimestamp(entry, timestamp, sizeof(timestamp));
        if (!first) json += ',';
        first = false;
        json += "{\"sequence\":";
        json += String(static_cast<unsigned int>(entry.sequence)).c_str();
        json += ",\"time\":";
        appendJsonString(json, validTime ? timestamp : "—");
        json += ",\"level\":";
        appendJsonString(json, logLevelDisplayName(entry.level));
        json += ",\"message\":";
        appendJsonString(json, entry.message);
        json += '}';
    }
    json += "]}";
    return json;
}

} // namespace EnvNode
