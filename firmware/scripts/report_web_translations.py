#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
# See the repository root LICENSE.
"""Inventory static web UI text still needing catalog entries or wiring.

This is a source inventory, not a C++/JavaScript parser. Text split by runtime
values is listed as fragments. Review runtime-only messages separately.
"""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parents[2]
LITERAL = re.compile(r'"(?:\\.|[^"\\])*"')
CATALOG = ROOT / 'firmware/include/WebTextCatalog.inc'
entries = {json.loads(m.group(1)) for m in re.finditer(
    r'^ENVNODE_WEB_TEXT\(\w+, ("(?:\\.|[^"\\])*"),', CATALOG.read_text(), re.M)}
TECHNICAL = {'Status', 'MQTT', 'WiFi', 'SSID', 'RSSI', 'MAC', 'IP', 'OTA',
             'Firmware', 'Flash', 'Build', 'MCU', 'CPU', 'GPIO', 'I2C', 'NTP',
             'KB', 'MHz', 'dBm', 'ID', 'Format', 'Home Assistant Discovery',
             'DHCP', 'EEPROM', 'ENHD descriptor 0.1', 'EnvNode DuoRelay 0.3',
             'EnvNode · v', 'SDA GPIO'}
files = sorted((ROOT / 'firmware/src').glob('*WebView.cpp'))
files += sorted((ROOT / 'firmware/src').glob('WebService*.cpp'))
files += [ROOT / 'firmware/src/ControllerWebSupport.cpp']
rows = {}

def add(path, offset, text):
    text = text.strip()
    if not re.search(r'[A-Za-z]{2}', text) or text in TECHNICAL:
        return
    if re.search(r'function\b|document\.|textContent|setInterval|=>|[{};=]|\b(?:sensorFields|actuatorFields|thresholdMeasurements)\(', text):
        return
    if text.startswith('badge ') or text in {'pio run', '.pio/build/&lt;environment&gt;/firmware.bin'}:
        return
    if '<' in text or '>' in text or re.fullmatch(r'[a-z_]+/[\w/]+', text):
        return
    line = path.read_text().count('\n', 0, offset) + 1
    key = (str(path.relative_to(ROOT)), text)
    rows.setdefault(key, line)

for path in files:
    source = path.read_text()
    # Only regular literals: raw JavaScript/CSS blocks are handled separately.
    source_for_literals = re.sub(r'R"(\w*)\([\s\S]*?\)\1"',
                                 lambda m: ' ' * len(m.group()), source)
    for match in LITERAL.finditer(source_for_literals):
        try:
            literal = json.loads(match.group())
        except ValueError:
            continue
        if '<' in literal or '>' in literal:
            for text_match in re.finditer(r'(?<=>)([^<>]+)(?=<|$)', literal):
                add(path, match.start(), text_match.group(1))
    # Static titles and result messages; ignore CSS classes and technical args.
    for call in re.finditer(r'\b(sendPage|sendResult|badge)\(\s*("(?:\\.|[^"\\])*"|[^;\n]*?)(?=;|\n)', source_for_literals):
        literals = list(LITERAL.finditer(call.group()))
        if not literals:
            continue
        candidates = [literals[0]]
        if call.group(1) == 'sendResult' and len(literals) >= 3:
            candidates.append(literals[2])
        for literal_match in candidates:
            text = json.loads(literal_match.group())
            # WebService centrally translates page/result titles and messages.
            if (call.group(1) in {'sendPage', 'sendResult'} or 'uiText(' in call.group()) and text in entries:
                continue
            if not text.startswith('/') and not re.fullmatch(r'[a-z_]+', text):
                add(path, call.start() + literal_match.start(), text)
    for match in re.finditer(r'\b(?:error|firmwareUploadRequestError_)\s*=\s*("(?:\\.|[^"\\])*")', source_for_literals):
        add(path, match.start(), json.loads(match.group(1)))
    # User-facing return strings (validation and property-format errors).
    for match in re.finditer(r'\breturn\s+("(?:\\.|[^"\\])*"|String\("(?:\\.|[^"\\])*"\))', source_for_literals):
        if path.name == 'WebService.cpp' and 'pendingActionMessage() const {' in source[:match.start()].rsplit('\n}', 1)[-1]:
            continue
        quoted = LITERAL.search(match.group())
        if quoted:
            text = json.loads(quoted.group())
            if ' ' in text and not text.startswith('<'):
                add(path, match.start(), text)

path = ROOT / 'firmware/include/PageEditorScript.h'
source = path.read_text()
patterns = [r"(?:Error|new Option|setCustomValidity)\('([^'\n]+)'",
            r"textContent\s*=\s*'([^'\n]+)'",
            r"(?:\?|:)\s*'([A-Z][^'\n]{9,})'"]
for pattern in patterns:
    for match in re.finditer(pattern, source):
        add(path, match.start(), match.group(1))

# uiText literals have already been translated. Remove them from candidates.
for key in list(rows):
    file, text = key
    source = (ROOT / file).read_text()
    line = rows[key]
    source_line = source.splitlines()[line - 1]
    if 'uiText(' + json.dumps(text, ensure_ascii=False) + ')' in source_line:
        del rows[key]

missing = [(file, text, line) for (file, text), line in rows.items() if text not in entries]
unwired = [(file, text, line) for (file, text), line in rows.items() if text in entries]
technical_rows = []
for path in files:
    source = path.read_text()
    for match in LITERAL.finditer(source):
        try:
            literal = json.loads(match.group())
        except ValueError:
            continue
        for text_match in re.finditer(r'(?<=>)([^<>]+)(?=<|$)', literal):
            text = text_match.group().strip()
            if text in TECHNICAL:
                technical_rows.append((str(path.relative_to(ROOT)),
                    source.count('\n', 0, match.start()) + 1, text))

report = """<!-- SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0 -->
<!-- See the repository root LICENSE. -->
# Oberflächentexte: Übersetzung und Ausnahmen

Die Zuordnung ist in [WebLocalization.md](WebLocalization.md) festgelegt.
Alle erfassten Bedien-, Hilfe-, Bestätigungs- und Fehlertexte verwenden stabile
Text-IDs im zentralen `firmware/include/WebTextCatalog.inc`. Die gespeicherte
Locale wählt Deutsch oder den englischen Fallback. Browsertexte verwenden eine
kleine, vom Server gelieferte Auswahl desselben Katalogs.

Dieser Bericht prüft statische HTML-Texte, direkt verwendete Seiten-/Meldungstexte
und Texte des Display-Editors. Aufgeteilte Texte werden als Fragmente erfasst.
Indirekte Messgrößen-, Qualitäts-, Implementierungs- und Formatfehlertexte sind
in `WebPresentationText` typisiert angebunden. Die Klassifizierung der übrigen
technischen Daten steht unten; sie sind kein Übersetzungsrückstand.

Regenerieren: `python3 firmware/scripts/report_web_translations.py`.

"""
for title, data in [('Noch zu übersetzen', missing),
                    ('Katalogeintrag vorhanden, noch anzubinden', unwired)]:
    report += f'## {title} ({len(data)} Fundstellen)\n\n'
    if not data:
        report += 'Keine Fundstellen in den geprüften Webansichten.\n\n'
    current = None
    for file, text, line in sorted(data):
        if file != current:
            report += f'\n### `{file}`\n\n'
            current = file
        report += f'- Zeile {line}: `{text.replace(chr(96), chr(39)).replace(chr(10), " ")}`\n'
    report += '\n'
report += """## Nicht zu übersetzen

| Kategorie | Stellen / Beispiele | Grund |
|---|---|---|
| HTTP- und Formularschnittstelle | WebService: `/time/save`, `server_.arg("locale")`, `Cache-Control`; ValueWebView: `name='code'` | Stabile Schnittstellen, keine Beschriftungen |
| DOM und Browserlogik | PageEditorScript: `page-formats`, `data-property-source`, `selectorChoice`, Formular- und JSON-Schlüssel | Identifikatoren für die Programmlogik |
| Eingaben und Nutzerdaten | `cfg.device.name`, `slot.name`, `d.name`, `option.label`, `contentPage.formats`, boolesche und Enum-Displaytexte | Vom Benutzer festgelegte Inhalte |
| Diagnoseprotokolle | `logger_.*`, LogWebView: `entry.message`, `entry.level`, `buildRecentLogJson()` | Rohdiagnose und Log-Level-Codes bleiben geräteweit konsistent |
| Technische Statuscodes | `boardIdentityStatusName`, `hardwareDescriptorDecodeStatusName`, `hardwareDescriptorStoreStatusName`, `hardwareDescriptorCompatibilityStatusName`, `moduleDeviceInventoryStatusName`, Herkunftscodes | Rohcodes wie `InvalidCRC` oder `ResourceUnavailable`, auch für Diagnose und Support |
| Hardware und Identitäten | Platinen-/Sensormodell, Treiber-ID, Modulslot A/B, UUID, EEPROM-Bank, `org.envnode.driver.*` | Identifikation statt sprachlicher Beschriftung |
| Werte und Einheiten | Messwerte, GPIO-/I2C-Adressen, `hPa`, `kPa`, `inHg`, `ms`, `SDA`, `SCL` | Physische Werte und standardisierte Symbole |
| Locale und Zeitzonen | `de-DE`, `en-GB`, `en-US`, `Deutsch (de-DE)`, `English (en-GB)`, `Europe/Berlin`, POSIX-Zeitzonenwerte | Stabile Locale-/Zeitzonenwerte und Sprachen in Eigenbezeichnung |
| Formatierung und Firmware | `%f`, `%u`, `%s`, `%%`, Firmware-/Build-Kennungen, `.pio/build/.../firmware.bin` | Technische Syntax und Artefaktkennungen |
| Beispiel-Nutzerdaten | Wasserquelle, Automatik, Zisterne, Hauswasser in WebServiceValues | Beispiel einer gespeicherten Wertdefinition, keine UI-Beschriftung |

### Direkt sichtbare technische Texte

Diese festen Texte bleiben bewusst unverändert. Sprachliche Beschriftungen
neben ihnen werden übersetzt.

"""
for file, line, text in sorted(set(technical_rows)):
    report += f'- `{file}:{line}`: `{text}`\n'
(ROOT / 'docs/WebTranslationsRemaining.md').write_text(report)
print(f'{len(missing)} missing catalog entries; {len(unwired)} existing translations not yet wired.')
