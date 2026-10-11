// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include "WebLocalization.h"
#include <cstddef>
#include "JsonWriter.h"
namespace EnvNode {
namespace {
struct Translation { const char* english; const char* german; };
const Translation translations[] = {
#define ENVNODE_WEB_TEXT(id, english, german) {english, german},
#include "WebTextCatalog.inc"
#undef ENVNODE_WEB_TEXT
};
static_assert(sizeof(translations) / sizeof(translations[0]) ==
    static_cast<size_t>(TextId::Count), "Every TextId requires a catalog entry");
}
const char* webLanguageTag(Locale locale) {
    return locale == Locale::GermanGermany ? "de-DE" : "en";
}
const char* webText(Locale locale, TextId id) {
    const size_t index = static_cast<size_t>(id);
    if (index >= static_cast<size_t>(TextId::Count)) return "";
    const Translation& entry = translations[index];
    return locale == Locale::GermanGermany && entry.german != nullptr && entry.german[0]
        ? entry.german : entry.english;
}
String buildWebUiScript(Locale locale) {
    struct ScriptText { const char* key; TextId id; };
    static const ScriptText texts[] = {
        {"AssignmentsReviewed", TextId::AssignmentsReviewed},
        {"SelectSourceForEveryPlaceholder", TextId::SelectSourceForEveryPlaceholder},
        {"ControlCharactersAreNotAllowed", TextId::ControlCharactersAreNotAllowed},
        {"EraseALLConfigurationAndRestart", TextId::EraseALLConfigurationAndRestart},
        {"FormatChangedReviewSourcesAndTranslationsUnusedSources", TextId::FormatChangedReviewSourcesAndTranslationsUnusedSources},
        {"InvalidPlaceholderUseFUSOr", TextId::InvalidPlaceholderUseFUSOr},
        {"LEVEL", TextId::LEVEL},
        {"Line", TextId::Line},
        {"MESSAGE", TextId::MESSAGE},
        {"Maximum128UTF8BytesPerLine", TextId::Maximum128UTF8BytesPerLine},
        {"MaximumFourSourcesPerLine", TextId::MaximumFourSourcesPerLine},
        {"MaximumSixLinesIncludingBlankLines", TextId::MaximumSixLinesIncludingBlankLines},
        {"NoLogEntriesAvailable", TextId::NoLogEntriesAvailable},
        {"NoSource", TextId::NoSource},
        {"On2S", TextId::On2S},
        {"On2SRefreshFailed", TextId::On2SRefreshFailed},
        {"Paused", TextId::Paused},
        {"PleaseReview", TextId::PleaseReview},
        {"PleaseReviewTheMarkedAssignments", TextId::PleaseReviewTheMarkedAssignments},
        {"PreviewIsOutOfDatePleaseRefresh", TextId::PreviewIsOutOfDatePleaseRefresh},
        {"RestartTheDeviceNow", TextId::RestartTheDeviceNow},
        {"SEQ", TextId::SEQ},
        {"SelectAMeasurement", TextId::SelectAMeasurement},
        {"SelectASource", TextId::SelectASource},
        {"SelectAnOption", TextId::SelectAnOption},
        {"Source", TextId::Source},
        {"TIME", TextId::TIME},
        {"Unavailable2", TextId::Unavailable2},
        {"UsePreviewRefreshToUpdateThePreview", TextId::UsePreviewRefreshToUpdateThePreview},
        {"WriteAndVerifyTheDuorelayDescriptorInThe", TextId::WriteAndVerifyTheDuorelayDescriptorInThe},
        {"WriteAndVerifyThisBoardIdentityTheActive", TextId::WriteAndVerifyThisBoardIdentityTheActive},
    };
    String html;
    html.reserve(4096);
    html = "<script>globalThis.EnvNodeUi=Object.freeze({";
    bool first = true;
    for (const auto& text : texts) {
        if (!first) html += ',';
        first = false;
        appendJsonString(html, text.key);
        html += ':';
        appendJsonString(html, webText(locale, text.id));
    }
    html += "});</script>";
    return html;
}
} // namespace EnvNode
