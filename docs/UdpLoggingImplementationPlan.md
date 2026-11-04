# Klassischer ESP32: Umstellung von SD auf UDP-Logging

EnvNode-owned: siehe ../LICENSE.

Stand der Prüfung: 2026-11-04. Dies ist ein Umsetzungsplan, keine abgeschlossene
Implementierung oder Hardwarevalidierung.

## Geprüfter Ausgangspunkt

- Worktree: `/Volumes/Daten/Development/EnvNode/esp32-rain-integration`.
- Branch: `codex/esp32-rain-integration`.
- HEAD: `f3ca1eee6bde530bb363a2dd70062cf5f29e02f4`.
- Vorhandene lokale Änderung: gelöschte `firmware/firmware.bin`. Erhalten;
  weder wiederherstellen noch in einen fachfremden Commit aufnehmen.
- Arduino-ESP32 bleibt auf dem bestehenden PlatformIO-Profil
  `espressif32 @ 7.0.1`; kein Frameworkwechsel.
- `firmware/platformio.local.ini` und lokale `.pio/build/esp32dev/firmware.*`
  fehlen. PlatformIO ist unter `~/.platformio/penv/bin/pio` vorhanden, aber
  nicht im aktuellen PATH. In dieser Bestandsaufnahme wurde kein Build/Test ausgeführt.
- Die Partitionen reservieren je OTA-App `0x1C0000` (1.835.008 Byte).
- ADR 0007 nennt für den vorherigen Web-Schritt 1.656.157 Programmbytes und
  101.764 statische RAM-Bytes **vor dem letzten Build-Identity-Refresh**.
  Das ist keine neu gemessene HEAD-Baseline. Frühere Heapwerte in
  `SdCardSpiBringup.md` gehören anderen Testständen an.

## Bestehende Integrationspunkte

`EnvNodeFirmware.cpp` verteilt Systemlogs an Serial und `enqueueSdSystemLog`.
`StructuredLogger` vergibt bereits Systemlogsequenzen und pflegt den RAM-Weblog.
`IDiagnosticRecorder` trennt fachliche Ereignisse vom Speicherziel.
`BufferedDiagnosticRecorder` enthält acht Records, nicht wartende Aufnahme und
Verluststatistiken; sein Sequenzzähler steigt bisher nur bei erfolgreicher Aufnahme.
`SdSystemLogService.cpp` koppelt Masken, Verfügbarkeit, Bootzähler, zwei
Aufzeichnungspfade und einen 12-KiB-Workerstack an SD. Diese Aufgaben müssen
getrennt werden; SD-Dateibatches sind keine geeignete UDP-Warteschlange.

`RainDetectorSensor::onRainFrequency` erzeugt je Gate `rain_window` mit
`revision`, `start_us`, `end_us`, `pulses`, optional `frequency_hz`, `fault`.
`Esp32RainFrequencyInput.cpp` setzt nominal 250.000 µs. Der Controller zeichnet
Kontext, Entscheidungen und Ausgänge auf. Ereignisnamen, Typen, Einheiten und
Bedeutung dieser Felder bleiben erhalten.

Diagnosepräferenzen der Sensoren/Aktoren/Controller liegen separat in NVS und
Backup. Systemdiagnose und Systemlog liegen noch im Bitfeld `sdStorage`;
Backup-Schema 5 enthält `sd.systemDiagnostics`. Die Checkbox-Weblogik hängt
aktuell an `extendedDiagnosticsAvailable()`. Ein Netzausfall darf künftig weder
Präferenzen löschen noch die Bearbeitung der Auswahl verhindern.

SD betrifft zusätzlich Discovery/Modulauswahl, Hardwarebelegung,
Konfigurationsvalidierung, Webrouten und Navigation, Dateitransfers,
Formatierung, Rotation, Native-Tests sowie `prepare_sd_framework.py`,
`sd_framework_patch.py` und dessen Tests. Das gemeinsame ESP32-Profil bindet den
Patch derzeit auch für das Testboard ein. SX1262 wurde in den geprüften
Anwendungssourcen und platformio.ini nicht gefunden; die spätere Bereinigung
prüft zusätzlich Registry, Profile und Produktdokumentation.

Der kooperative HTML-/CSS-Versand aus HEAD bleibt erhalten. SDK-Requestparser,
MQTT und andere Netzwerkpfade benötigen weiterhin unabhängige Latenzprüfung.

## Zielvertrag

1. Gemeinsamer Anwendungskern, austauschbarer Recorder/Transport, explizite
   Hardwarefähigkeiten. Klassischer ESP32 unterstützt RainDetector, kein SD/SX1262.
   S3 mit neuem Framework bleibt ein späteres Profil, kein zweiter Firmwarefork.
2. Konfiguration in NVS/Backup/Web: Netzwerklogging aktiv, Empfänger-IP,
   UDP-Port, Systemlogauswahl und bestehende Diagnoseauswahl. Zunächst numerische
   IP verwenden, damit kein synchrones DNS in Mess-/Controllerpfade gelangt.
   Migration übernimmt bestehende Auswahlbits, aktiviert ohne konfiguriertes Ziel
   keinen Versand. Alte Backups bleiben lesbar, Migration ist idempotent.
3. Ein Ereignis pro Datagramm; RFC-5424-Hülle mit unterschiedlichen APP-NAMEs
   `envnode-system` und `envnode-diagnostic`. Diagnosen behalten ihren JSON-Inhalt;
   Zuordnungsfelder werden versioniert ergänzt. Empfängerkonfiguration vor
   Implementierung gegen offizielle rsyslog-Dokumentation und lokalen Test prüfen.
4. Jedes Datagramm ist eigenständig zuordenbar: stabile Gerätekennung,
   Boot-Kennung, Firmware-/Buildkennung, Streamkennung, Sequenz, Aufnahme-Uptime,
   optionale UTC-Zeit mit expliziter Ungültigkeit. Boot-ID unabhängig von SD;
   robustes Verhalten bei NVS-Fehlern, keine kollidierende konstante Ersatz-ID.
   Sensorrevision bleibt das bestehende fachliche Feld. Konfigurationsgeneration
   und periodischer Kontext erlauben Slot-/Konfigurationszuordnung nach UDP-Verlust.
5. Sequenzen je Stream vor möglichem Queueverlust vergeben, deaktivierte Quellen
   explizit ausnehmen. Überlauf/Neustart und Uptime-Wrap definieren und testen.
   Systemlogsequenzen bewahren; Transportsequenzen bei Bedarf zusätzlich benennen.
   Zähler und spätere Verlustmeldungen ergänzen Sequenzlücken. UDP-Erfolg bedeutet
   nur lokale Socketannahme; stille Verluste im Netz/Server sind nicht messbar.
6. Produzenten kopieren ausschließlich in begrenzte RAM-Queues, ohne Netzwerk-I/O,
   JSON-Encoding, dynamische Allokationen je Ereignis oder wartende Locks.
   Separate System-/Diagnosebudgets und faire Abarbeitung verhindern Verdrängung.
   Bei Überlauf neues Ereignis verwerfen und zählen. Keine Mittelung/Drosselung
   der RainDetector-Erzeugung, keine persistente Nachlieferung.
7. Transport verwendet den zum installierten Framework passenden BSD/lwIP-Socket
   im Nichtblockiermodus. Begrenzte Datagrammzahl pro Service-Schritt, kein Warten
   auf WLAN/ARP/Server und keine Wiederholungsschleife bei EAGAIN/ENOBUFS.
   Fehler-/Offlineereignisse werden gezählt und verworfen; Socket-Reinitialisierung
   mit Backoff ohne Logrekursion. Konfigurationswechsel klar von Ausfall trennen.
8. Maximale kodierte Größe aus allen erlaubten Feldlängen einschließlich Escaping,
   UTF-8 und Hülle ableiten. Ziel: höchstens 1.200 Byte UDP-Nutzlast ohne Fragmentierung
   bei üblicher MTU; falls der bestehende Vertrag das nicht erlaubt, vorab explizites
   Framing entscheiden. Niemals still abschneiden oder fachliche Felder streichen.
   Größenüberschreitungen sichtbar zählen. Queue/Stack erst nach Größenmessung wählen.
9. Lokal/Web sichtbar: Queuefüllstand/Maximum, erzeugt, aufgenommen, lokal gesendet,
   full/busy/offline/send-error/encode-error/stale, letzter Fehler und Zielzustand.
   Kein vermeintlicher Status „Server erreichbar“ ohne Empfangsbestätigung.

## Umsetzung und getrennte Commits

### 1. Reproduzierbare Basis und Vertrag

Commit `docs(logging): define UDP contract and resource baseline`:
Baseline an HEAD mit eigener temporärer Build-/Artefaktablage erstellen,
Toolchainpakete und Flags dokumentieren. Fehlende lokale Zusatzkonfiguration
für den Build isoliert behandeln, keine Nutzerdateien überschreiben.
ELF, Map, Größenreport, Buildkennung, Binärdatei und Hash aufbewahren.
Bestehende relevante Native-/Webtests ausführen. Keine vorhandene gelöschte
Release-Binärdatei dabei wiederherstellen. SD-Historie bleibt über HEAD und
vorherige Commits erreichbar; keine Historie umschreiben.

### 2. Format, Queue und Konfiguration

Commit `feat(logging): add bounded syslog encoding and configuration`:
Portabler Codec/Queues mit Fake-Uhr und Fake-Transport; Metadaten und
Sequenzsemantik implementieren. Konfigurationsmigration, Backup-Roundtrip,
Zielvalidierung und Webdarstellung ergänzen. Bestehende Checkboxwerte auch bei
unverfügbarem Transport erhalten. Noch keine Entfernung des SD-Codes.

### 3. UDP-Integration bei voller Diagnoserate

Commit `feat(logging): integrate nonblocking UDP recording`:
Logging-Fanout und Recorder auf UDP umstellen; SD-Code für Vergleich zunächst
noch vorhanden, parallele doppelte Aufzeichnung nur bewusst im Test aktivieren.
Ressourceninstrumentierung und Verluststatus ergänzen. Fake-Transport mit
voller Gatefolge und zusätzlichen Controller-/Systemereignissen testen;
anschließend lokaler UDP-Testempfänger. Kein fremdes Ziel ungeklärt kontaktieren.

**Freigabekriterium vor SD-Entfernung:** Codec-/Ausfall-/Lasttests erfolgreich,
250-ms-Gates unverändert, begrenzte Speicherbelegung und kein wartender
Socketpfad. Volle reale Hardwarevalidierung ist zusätzlich nötig, bevor der
Hardwaretest als bestanden gilt. Soll diese bereits vor Entfernung erfolgen,
wartet dieser Meilenstein auf geklärtes Testgerät und Testempfänger; lokale
Simulation allein beweist weder ESP32-Latenz noch Laufzeit-Heap.

### 4. SD aus klassischem Build entfernen

Commit `refactor(esp32): remove SD runtime and framework patch`:
SD-Aufzeichnung, Dateiverwaltung, Webrouten/-zustände/-texte, Format-/Rotationstests,
Treiber, Arbiter, Aktivierung, Pinreservierungen und Framework-Patch entfernen.
SD-spezifische Tests entfernen, generische Recorder-/Migrationstests erhalten
und auf UDP umstellen. Legacy-Buildalias bereinigen. Historische Hardwaredaten,
Schaltpläne und SD-Bringup-Nachweise bleiben als solche erhalten; kein aktiver
Support im klassischen Profil. Descriptoren dürfen erkannt und als unsupported
angezeigt werden, aber keine Laufzeitaktivierung auslösen.
Link-Map und Include-/Symbolprüfung müssen SD/FAT-Abhängigkeiten ausschließen.
Testboard und gemeinsame Libraries separat bauen/testen, damit Patchentfernung
und Hardwarefähigkeiten keine unbeabsichtigten Nebenwirkungen verursachen.

### 5. Empfänger, Dokumentation und Testartefakte

Commit `docs(logging): add receiver setup and validation results`:
Versionierte Beispielkonfiguration für rsyslog `imudp` auf dediziertem,
konfigurierbarem Port; eigenes Ruleset, getrennte Dateien für beide APP-NAMEs,
JSON-MSG unverändert speichern, Ruleset mit `stop` abschließen. Unbekannte
Nachrichten gesondert behandeln. Logrotate-Beispiel mit täglich/Größenlimit,
begrenzter Aufbewahrung, Kompression, passenden Besitzrechten und HUP zum
Wiederöffnen; kein copytruncate-bedingter Verlust. Syntaxprüfung mit
`rsyslogd -N1 -f ...` in isolierter Umgebung und lokaler Paket-/Rotationstest.
Keine Installation oder Änderung auf einem realen Server.

Anleitungen für Konfiguration, Migration, Verlustinterpretation, Hardwareprofile
und Rückkehr zu einem historischen SD-Commit aktualisieren. Firmware für
Hardwaretest mit eindeutiger Buildkennung, SHA-256 und benötigten Flash-/OTA-Dateien
in separatem Releaseverzeichnis bereitstellen; Downloadreferenzen prüfen.
Keine automatisch generierten Änderungen an der lokal gelöschten firmware.bin.

## Ressourcenvergleich und Abnahme

Drei Messstände mit identischer Toolchain/Partitionierung/Sensorkonfiguration:
A = HEAD mit SD; B = integriertes UDP vor SD-Entfernung; C = UDP ohne SD.
Für jeden Stand Programmbytes/OTA-Reserve, statisches DRAM (.data/.bss nach
Toolchainreport), Queue-/Codecgrößen, Taskstacks und einmalige Heapallokationen
angeben; B−A und C−A getrennt berichten. Keine Heapersparnis aus statischen
RAM-Zahlen ableiten.

Auf Hardware gleiche Szenarien: Boot, WLAN/MQTT verbunden, alle gewählten
Diagnosen aktiv, Web/Display/Controller parallel, Zielausfall, WLAN-Reconnect,
Webclient mit TCP-Backpressure, OTA-Vorbereitung. Messen: interner 8-Bit-Heap,
Minimum und größter freier Block, Stackreserve, Queuemaximum/Verluste,
maximale Loop-/Servicezeit und Gate-/Controllerlatenz. Instrumentierung darf
nicht selbst pro Gate synchron seriell loggen. Vorher-/Nachherkurven und
Versuchsbedingungen speichern. Ohne Zugriff bleiben Laufzeitwerte ausdrücklich
offen; Analog/SCT013 erst danach einzeln und kombiniert budgetieren.

Testmatrix:

- Format: Escaping/UTF-8, Grenzlängen, alle Feldtypen, ungültige UTC, Metadaten,
  keine Credentials; unveränderte Rain-Feldinhalte gegen bestehende Fixtures.
- Sequenzen: zwei Streams, verlorene Aufnahme, Ausfall, Bootwechsel, Wrap,
  Auswahlwechsel und Kontextwiederholung; Verlustbericht selbst darf verloren gehen.
- Puffer: exakt voll, nächstes Ereignis verworfen, Busy, Allokationsfehler,
  begrenzte Speicherbelegung und faire Drain-Reihenfolge.
- Netz: offline, Socketfehler, EAGAIN/ENOBUFS, unerreichbares Ziel, Wiederverbinden,
  Konfigurationswechsel; kein Rückstau ohne Grenze und keine Rekursion.
- Funktion: identische Gateeingaben mit/ohne Diagnosen ergeben identische
  Messwerte, Controllerentscheidungen und Aktorausgänge, auch bei voller Queue.
- Dauerlast: mindestens eine Stunde 4 Gates/s je aktivem RainDetector plus
  Controllerburst, Systemlogs und wiederholte Webzugriffe. Im kontrollierten
  verlustfreien lokalen Versuch jedes Gate am Empfänger wiederfinden; bei
  provozierten Ausfällen Lücken/Zähler nachvollziehbar und Ressourcen begrenzt.
- Regression: Konfiguration/Backups, Diagnosenauswahl, Rainpolicy,
  Controller-/FirmwareRuntime, StructuredLogging, Webhost/Backpressure,
  Display und Testboard; keine neue Annahme, dass HEAD alle Netzwerkprobleme behebt.

## Grenzen dieses Arbeitsschritts

Nur Bestandsaufnahme und Planung. Keine Firmwareänderung, kein Flashen, kein
Serverzugriff. Vor Hardwarevalidierung Gerät/Boardrevision, Zugriffsmethode,
vorhandene Konfiguration, Testaufbau und Empfänger-IP/Port klären.
Vor einer realen rsyslog-Änderung Zielsystem, Betriebssystem/rsyslog-Version,
Zugriff und gewünschte Pfade/Aufbewahrung klären. Das hindert lokale Umsetzung,
Builds, Fake-Transporttests und vorbereitete Empfängerkonfiguration nicht.
