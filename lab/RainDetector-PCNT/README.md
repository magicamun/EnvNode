# RainDetector PCNT Lab-Firmware

Eigenständiger PoC für den digitalen 3,3-V-Ausgang eines ICM7555.
Eigenständige Anwendung mit gemeinsamem Auswertungskern aus `firmware/lib`,
experimenteller lokaler Benetzungsauswertung, ohne MQTT und ohne Kapazitätsberechnung. Eine manuelle,
wahlweise zeitlich begrenzte oder dauerhafte Heater-Steuerung dient dem Heiz-/Trocknungsversuch.

## Projekt und Toolchain

Die produktive PlatformIO-Konfiguration verwendet `esp32dev`, Arduino und eine
ungepinnte `espressif32`-Plattform. Dieses Lab-Projekt fixiert die bei der
Implementierung installierte Plattform auf **7.0.1**. Der geprüfte Build nutzt
Arduino-ESP32 **2.0.17** (Paket `3.20017.241212+sha.dcc1105b`) und dessen
ESP-IDF **4.4.7**. Es benötigt keine zusätzlichen Bibliotheken.

Verwendet wird die klassische API aus `driver/pcnt.h`, PCNT Unit 0 / Channel 0:
`pcnt_unit_config`, `pcnt_counter_clear`, `pcnt_counter_resume`,
`pcnt_counter_pause` und `pcnt_get_counter_value`.
Steigende Flanken inkrementieren; fallende Flanken werden ignoriert.
Filter und CPU-PCNT-Interrupts sind deaktiviert. Es gibt keine ISR,
kein `pulseIn()` und keine GPIO-Abfrageschleife.

Referenz: [Espressif PCNT API für ESP-IDF 4.4](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/pcnt.html).

## Anschluss und Konfiguration

- ICM7555 `RAIN_FREQ` (digital, 3,3 V) an **GPIO 27** des klassischen ESP32.
- Gemeinsame Masse zwischen RainDetector und ESP32 herstellen.
- Keine 5-V-Signale einspeisen. GPIO 27 muss auf dem verwendeten Aufbau frei sein.
- Controller `HEATER_EN` (J3 Pin 3) an **GPIO16**. Der Ausgang ist aktiv HIGH;
  die Heizlast wird vom MOSFET des Controllers geschaltet, nicht direkt vom ESP32.
- Weather GPIO16 → J3 und GPIO27 → J4: jeweils Pin 1 GND, Pin 2 +5 V,
  Pin 3 Signal, Pin 4 +3,3 V. Spannungsfrei verbinden und reale Kontaktlage prüfen.
- GPIO16 muss frei sein (klassischer ESP32-WROOM-Aufbau, kein dort angebundener PSRAM).
  Versorgung für den Head-Heizstrom zusätzlich zur übrigen Last auslegen.
- Heizung startet mit 0 %. PWM: **5 kHz, 8 Bit, LEDC-Kanal 0**. Frequenz und
  Pins stehen in `LabConfig.h`; die bewährte Frequenz wird vorerst beibehalten.
- GPIO, Messfenster und Baudrate stehen zentral in `include/LabConfig.h`.
  GPIO 27 ist eine Lab-Vorgabe, keine Zuordnung aus einem EnvNode-Boardprofil.
- Getestetes Build-Ziel ist `esp32dev`, nicht ESP32-C3/S3 oder andere Varianten.

## Messung und Grenzen

Der Zähler wird bei pausierter Messung gelöscht, dann gestartet und nach
nominell **250.000 µs** angehalten. Währenddessen gibt `delay(1)` CPU-Zeit frei;
PCNT zählt unabhängig weiter. Die Dauer wird über `esp_timer_get_time()` direkt
vor Resume und direkt nach Pause erfasst. Die Rechnung lautet:

`frequency_hz = pulse_count * 1000000.0 / (end_us - start_us)`

Das reale Fenster ist durch Task-Scheduling etwas länger als 250 ms und nicht
hardwaregenau getaktet. Die Zeitmessung umfasst zusätzlich den kleinen Aufwand
der Resume-/Pause-Aufrufe; sie ist eine Näherung der tatsächlichen Zählzeit.
Eine konkrete Dauer am Zielgerät wurde noch nicht gemessen. Drei Nachkommastellen
der Ausgabe sind keine Genauigkeitszusage: die Zählauflösung liegt bei etwa 4 Hz
pro Puls; hinzu kommen Gate-Timing- und ESP32-Taktfehler.

Serial-Ausgabe und Reset passieren zwischen den Fenstern. Es gibt daher eine
kurze Messpause; Zeitstempelabstände sind nicht gleich der Zählfensterdauer.
Die Firmware misst Fensterfrequenzen, keinen lückenlosen Gesamtpulszähler.

Der 16-Bit-PCNT setzt beim oberen Limit von 32767 Pulsen auf null zurück.
Um dann keine falsche niedrige Frequenz auszugeben, wird ausschließlich das
High-Limit-Ereignis aktiviert. Dessen gelatchtes Raw-Interrupt-Bit wird nach
Pause geprüft und vor jedem Fenster gelöscht (`PCNT.int_raw` / `PCNT.int_clr`,
klassische ESP32-Register). Dafür wird kein CPU-Interrupt aktiviert.
Ein Überlauf ergibt `nan` statt einer Frequenz; `pulse_count` ist dann nur der
Restzählerstand. Bei 250 ms liegt die Grenze ungefähr bei 131 kHz, bei längeren
Fenstern entsprechend niedriger. Für einige 10 kHz reicht der Zähler aus.

## Serial-Format

115200 Baud, 8N1. Einmalig nach Initialisierung:

```csv
timestamp_ms,pulse_count,frequency_hz,heater_percent
```

Danach eine Zeile je Fenster mit dezimalem Punkt und drei Nachkommastellen.
`timestamp_ms` bezeichnet das Fensterende in Millisekunden seit Boot.
Beispielwerte (keine reale Hardwaremessung):

```csv
1250,10000,40000.000,0
1505,0,0.000,25
1760,123,nan,25
```

Ohne Flanken ist das Ergebnis null; ein fehlendes Signal lässt sich daraus
nicht von einem stillstehenden Oszillator unterscheiden. Bei einem API-Fehler
wird die Heizung ausgeschaltet; es erscheint
`# error,<operation>,<ESP-Fehlername>` und die Messung stoppt bis Reset.
ESP32-ROM-Bootmeldungen können vor dem CSV-Header erscheinen. Für Auswertung
erst ab dem Header lesen, alle mit `#` beginnenden Kommentar-/Statuszeilen
überspringen und `nan` behandeln. `heater_percent` ist der angeforderte PWM-Prozentwert
während des gesamten Messfensters, keine gemessene Heizleistung oder Temperatur.

## Manuelle Heizung über Serial

Befehle in Großbuchstaben senden, jeweils mit Zeilenende LF oder CR/CRLF:

| Befehl | Wirkung |
| --- | --- |
| `HEAT 25 30` | 25 % PWM für 30 Sekunden |
| `HEAT 50 ON` | 50 % PWM dauerhaft bis zum Ausschalten |
| `HEAT OFF` | Heizung aus, zuletzt gewählten Pegel merken |
| `HEAT ON` | Zuletzt gewählten Pegel dauerhaft einschalten |
| `HEAT 10 60` | 10 % PWM für 60 Sekunden |
| `OFF` | Heizung vorzeitig aus |
| `STATUS` | Heizpegel und verbleibende Laufzeit ausgeben |
| `HELP` | Befehlsübersicht |

Wert und Dauer sind ganzzahlig: 1–100 % und beim zeitbegrenzten Befehl
1–120 Sekunden. `HEAT <Wert> ON` und `HEAT ON` haben **keinen Timeout**;
mit `HEAT OFF` oder dem weiterhin gültigen Alias `OFF` beenden.
`HEAT ON` übernimmt den letzten gültig gewählten Pegel, auch nach Timeout oder OFF,
und startet immer Dauerbetrieb. Vor der ersten Pegelwahl seit Boot wird es abgewiesen.
Ein neuer gültiger Einschaltbefehl ersetzt den bisherigen Modus und dessen Timer.
Der letzte Pegel bleibt nur im RAM; nach Reset startet alles AUS und ohne gemerkten
Pegel. Keine kumulative Laufzeitbegrenzung und keine automatische Heizregelung.
Ungültige Befehle ändern den laufenden Test nicht. Zeilen über 63 Zeichen oder
mit ungültigen Steuerzeichen werden bis zum nächsten Zeilenende verworfen.

Status-/Ereigniszeilen haben das Format:

```text
# heater,timestamp_ms,heater_percent,remaining_ms,reason
# heater,1250,25,29999,command
# heater,31250,0,0,timeout
```

Bei Dauerbetrieb ist `remaining_ms` **−1** (unbegrenzt), bei AUS 0.
CSV-Spalten bleiben unverändert. Beispiel: `# heater,1250,50,-1,command`.
Die erste Zeile oben ist eine Formatbeschreibung, kein zusätzlich ausgegebener Header.
Gründe sind `boot_off`, `command`, `off`, `status` und `timeout`.
Befehle und Abschaltung werden zwischen Messfenstern bearbeitet. Die Abschaltung
kann im normalen Betrieb um ungefähr ein Messfenster (250 ms plus Bearbeitungszeit)
verspätet sein; dies ist keine harte Echtzeitgarantie. Im zeitbegrenzten Modus läuft der Timeout ohne weitere
Serial-Eingaben. Dauerbetrieb läuft auch nach Schließen des Monitors weiter;
vor dem Beenden deshalb ausdrücklich `HEAT OFF` senden und Pegel 0 prüfen. PCNT-Fehler schalten ebenfalls ab. Software-Hänger oder ein defekter
MOSFET werden dadurch nicht abgesichert. Es gibt keine Temperaturmessung oder
Temperaturregelung; Tests nur unter Beobachtung durchführen. Die Hardware muss
HEATER_EN auch während Reset/Boot über den vorhandenen Pull-down LOW halten.

## Heiz-/Trocknungsversuch

1. Trocken und unberührt bei `heater_percent=0` eine Referenz aufzeichnen.
2. Zunächst am **trockenen Head** kurz `HEAT 10 30` ausführen. Frequenz während
   Erwärmung und nach automatischer Abschaltung bis zur Stabilisierung beobachten.
   Dadurch wird der Temperatureinfluss vor Benetzungsversuchen sichtbar; ohne
   Temperaturmessung lässt sich kein Temperaturkoeffizient daraus bestimmen.
3. Erst danach eine vergleichbare Benetzung einmal passiv und einmal mit
   begrenzter Heizung trocknen lassen; Menge, Position und Bedingungen notieren.
4. Mit `OFF` abschließen und Status 0 prüfen. Kein Nass-/Trocken-Schwellwert
   und keine automatische Trockenreferenz-Nachführung sind implementiert.

## Build, Flash und Test

Vom Repository-Root aus (falls `pio` nicht im PATH liegt:
`~/.platformio/penv/bin/pio` verwenden):

```sh
pio run -d lab/RainDetector-PCNT
pio device list
pio run -d lab/RainDetector-PCNT -t upload --upload-port /dev/cu.usbserial-0001
pio device monitor -d lab/RainDetector-PCNT -e esp32dev
```

Den Beispielport durch den tatsächlichen ESP32-Port ersetzen. Flashen ersetzt
die aktuell auf diesem ESP32 laufende Firmware. Nach Öffnen des Monitors bei
Bedarf Reset drücken, um den CSV-Header zu erfassen. `send_on_enter` sendet
Befehle erst nach Enter. Die Eingabe wird ohne lokales Echo möglicherweise nicht
angezeigt; die Firmware bestätigt Befehle über `# heater,...`.

### Versuch automatisch aufzeichnen

Der oben angegebene Monitor lädt die Projektkonfiguration mit `send_on_enter`,
`direct` und `log2file`. Auch der PlatformIO-Projektmonitor verwendet diese Filter.
Jede Monitorsitzung schreibt eine neue Datei unter
`lab/RainDetector-PCNT/logs/device-monitor-YYMMDD-HHMMSS.log`.
Beim Start zeigt PlatformIO den absoluten Dateipfad an. Keine zusätzlichen
`--filter`-Optionen angeben, die die konfigurierte Filterliste ersetzen könnten.
Ein bereits laufender Monitor muss geschlossen und neu gestartet werden;
kein erneutes Flashen nötig. Nur einen Monitor am seriellen Port öffnen.

Aufgezeichnet wird die empfangene Ausgabe: CSV-Messungen einschließlich Heizpegel,
`# heater`-Bestätigungen mit Zeitstempel und Timeout sowie eventuelle Fehler- und
Bootmeldungen. Die eingegebenen Befehle selbst werden nicht mitgeschrieben;
angenommene Heizbefehle sind durch die Bestätigungen dokumentiert.
`timestamp_ms` ist die Zeit seit ESP32-Start, der Dateiname kennzeichnet die
Monitorsitzung. Ein Reset beginnt die Gerätezeit erneut bei null.
Die Logdatei ist wegen der Kommentarzeilen kein reines CSV; für Auswertungen
Messzeilen und Ereignisse getrennt einlesen. Ausgaben vor Monitorstart fehlen.

Für den Benetzungsversuch zuerst etwa 60 Sekunden trocken aufzeichnen. Zeitpunkt
des Tropfens und sichtbaren Zustand separat mit Gerätezeit notieren, anschließend
30 Sekunden ohne Heizung messen und `HEAT 50 30` senden. Nach Timeout bis zur
Stabilisierung weiterloggen. Zum Beenden Ctrl+C drücken und die Logdatei für die
Auswertung behalten; lokale Logs werden nicht versioniert.

Zunächst Signal und gemeinsame Masse verbinden, dann die Frequenz mit
Oszilloskop/Frequenzzähler vergleichen. Optional einen bekannten 3,3-V-Takt
einspeisen: 40 kHz sollten bei ungefähr 250 ms ungefähr 10000 Pulse ergeben,
unabhängig vom Tastverhältnis (innerhalb der PCNT-Eingangsgrenzen).
Mit auf GND gelegtem Eingang müssen null Pulse erscheinen. Optional mit
150 kHz testen: die Frequenz muss `nan` werden. Den ICM7555-Ausgang vor
Einspeisung eines externen Testtakts trennen.

Parser- und Timeout-Prüfung auf dem Host (vom Repository-Root):

```sh
c++ -std=c++11 -Wall -Wextra -Werror -I lab/RainDetector-PCNT/include \
  lab/RainDetector-PCNT/tests/test_heater_control.cpp -o /tmp/envnode-rain-heater-test
/tmp/envnode-rain-heater-test
```

Hardware-Abnahme nach dem Flashen: Boot/Reset startet mit ausgeschalteter Heizung;
`HEAT ON` wird vor der ersten Pegelwahl abgewiesen;
`HEAT 10 3` schaltet nach ungefähr drei Sekunden ohne weiteren Befehl ab;
`HEAT 10 ON` läuft über drei Sekunden hinaus, bis `HEAT OFF` gesendet wird;
anschließend schaltet `HEAT ON` wieder mit 10 % ohne Timeout ein;
`OFF` beendet einen längeren Versuch vorzeitig; ungültige Befehle aktivieren nichts.
CSV muss zu den beobachteten Heizphasen passen. Frequenzmessung, Überlauf (`nan`)
und Nullsignal weiterhin prüfen. Die neue kombinierte Funktion wurde noch nicht
auf Hardware getestet oder von diesem Auftrag aus geflasht.

## Offline-Auswertung der Messlogs

`tools/analyze_logs.py` erzeugt ohne zusätzliche Python-Pakete einen lokalen,
interaktiven Bericht. Keine Verbindung zum ESP32, keine Firmwareänderung und
keine automatische Heizungs- oder Regenentscheidung. Beispiel vom Repository-Root:

```sh
python3 lab/RainDetector-PCNT/tools/analyze_logs.py \
  lab/RainDetector-PCNT/logs/device-monitor-261019-134016.log \
  lab/RainDetector-PCNT/logs/device-monitor-261019-151915.log \
  lab/RainDetector-PCNT/logs/device-monitor-261019-163136.log \
  lab/RainDetector-PCNT/logs/device-monitor-261019-173457.log \
  lab/RainDetector-PCNT/logs/device-monitor-261019-170314.log \
  --output lab/RainDetector-PCNT/logs/analysis/offline
open lab/RainDetector-PCNT/logs/analysis/offline/report.html
```

Der Bericht funktioniert ohne Netzwerk oder lokalen Server. Versuch auswählen,
Zeitbereich in Minuten eingrenzen oder eine Ereigniszeile anklicken. Die
Frequenzkurve behält beim Verkleinern Minima und Maxima; Heizphasen,
Beobachtungsmarken und Abfall-Kandidaten bleiben sichtbar. Ein Zeiger über dem
Diagramm zeigt Zeit, Frequenz und Heizpegel. Ergebnisse stehen zusätzlich in
`results.json`, `events.csv` und `summary.md`. Quelldateien werden nicht verändert;
SHA-256-Hashes der gelesenen Log-Snapshots werden protokolliert; ein noch
laufendes Log wird pro Auswertung einmal eingelesen. Bei erneutem Aufruf im gleichen
Ausgabeordner werden die erzeugten Berichte überschrieben.

Erkennungsansatz: Sekundenmediane vergleichen einen aktuellen Wert mit einer
lokalen Referenz aus vier vorausgehenden Sekunden, mit einer Sekunde Abstand.
Ein Abfall um mindestens 250 Hz **und** mindestens 1 % ist ein Kandidat.
15 Sekunden Sperrzeit verhindern Mehrfachmeldungen desselben Übergangs.
Heizbefehle und Heizwechsel sperren die Auswertung für 10 Sekunden;
Teil-PWM, Nullsignal, ungültige Messfenster und Datenlücken werden ausgeschlossen.
Diese Grenzen sind explorative Startparameter, keine validierte Sensor-Spezifikation.
Die Erkennung bleibt bei unveränderter 100-%-Heizung aktiv und sucht relative
Änderungen auch auf bereits nasser Fläche.

Zehnsekunden-Trends beschreiben Steigungen ab 3 Hz/s als „Erholung“, Beträge
bis 1,5 Hz/s als „stabil / langsame Drift“, andere Verläufe als „unklar“.
**Stabil bedeutet nicht trocken, Erholung bedeutet nicht Regenende.** Es gibt
keine nachgeführte Trockenreferenz und keine absolute Trockenfrequenz.

`tools/observations.json` enthält die ungefähren Versuchsanmerkungen mit Herkunft
und Einschränkungen. Bei mehreren Boot-Sitzungen wird jeweils separat ausgewertet;
die optionale `session`-Angabe einer Beobachtung zählt ab null. Alte Werte vor einem
vollständigen ersten Boot-Marker und doppelte Zeitstempel werden verworfen.
Die Darstellung und Erkennung sind offline und rückblickend; die Sekundenmediane
müssen vor einer späteren Live-Übernahme hinsichtlich Latenz und Fenstergrenzen
separat geprüft werden.

Die ersten Parameter wurden an Entwicklungssitzungen geprüft und anschließend
unverändert auf `170314` angewendet. Dieser Versuch ist ein **retrospektiver Holdout**:
Er war bereits im Gespräch bekannt und ersetzt keinen unabhängigen Blindtest.
Zeitmarken werden innerhalb ±10 Sekunden genau einem Kandidaten zugeordnet.
Zusätzliche Kandidaten gelten als „nicht zugeordnet“, nicht automatisch als
Fehlalarm, da die manuellen Notizen unvollständig sind. Auch zusätzliche reale
Benetzungen, Handhabung oder elektrische Störungen können dahinterstehen.

Prüfung mit synthetischen Abfällen, Drift, Heizwechseln, PWM, Datenlücken,
Nullsignal, ungültigen Werten, Neustarts und Annotationen:

```sh
python3 -m unittest discover -s lab/RainDetector-PCNT/tests -p 'test_analyze_logs.py' -v
```

Der anschließende Versuch `180137` dient als prospektiver Prüfdatensatz mit
unveränderten Parametern. Nur der letzte Sprühstoß bei etwa 440 s wurde einzeln
bestätigt; die übrigen Sprühzeiten sind nicht vollständig manuell markiert.
Eine Anzeige 0/1 bezieht sich daher ausschließlich auf diese einzelne Marke,
nicht auf die ganze Sprühfolge. Der letzte kurze Einbruch wird von der ersten
Erkennung nicht gemeldet, obwohl zahlreiche frühere Einbrüche Kandidaten erzeugen.
Parameteränderungen erst nach Dokumentation dieses Ergebnisses neu bewerten.

## Lokaler Auswertungskern: experimenteller Live-Test

Die Lab-Firmware verwendet jetzt die hardwareunabhängige Bibliothek
`firmware/lib/RainDetectorEvaluation`. Sie arbeitet inkrementell und ohne Heap-
Allokation im Kern. Im geprüften ESP32-Build benötigt die Instanz **112 Byte**;
zusätzlich fallen Stack, Ausgabe und wenige Diagnose-Zustandswerte im Lab an.
Ein langer Ringpuffer ist für diese erste Fassung nicht erforderlich.
Die normale EnvNode-Firmware 0.6.3 integriert den Sensor noch nicht.

Die vier bestehenden CSV-Spalten und alle HEAT-Befehle bleiben erhalten. Zusätzlich
erscheinen höchstens ungefähr einmal pro Sekunde sowie bei Ereignissen/Zustandswechseln:

```text
# evaluation,timestamp_ms,fast_hz,slow_hz,trend_hz_s,valid,estimate,new_wetting
```

Dies ist eine Formatbeschreibung, kein ausgegebener Header. `new_wetting=1`
kennzeichnet einen Ereigniskandidaten. `estimate` hat die Werte `unknown`, `active`
oder `end_estimated`. **`end_estimated` bedeutet weder nachgewiesenes Regenende
noch trockene Oberfläche.** Start ohne bekannten vorherigen Zustand bleibt
`unknown`. `valid=0` kennzeichnet ausgeschlossene/noch nicht auswertbare Werte.
Beim Boot wird zusätzlich `# evaluation_core_bytes,112` ausgegeben (Zahl ist
abhängig vom Build). Die Diagnosezeilen werden durch `log2file` mit aufgezeichnet.

Startparameter, im C++-Konfigurationsobjekt veränderbar, noch keine NVS-/Web-UI:

| Parameter | Erste Fassung |
| --- | --- |
| Schneller Filter | Zeitkonstante 0,25 s |
| Langsamer Diagnosefilter | Zeitkonstante 10 s |
| Geglättete Änderungsrate | Zeitkonstante 5 s |
| Neues Benetzungsereignis | Abfall des schnellen Filters gegenüber vorherigem Wert um mindestens 150 Hz und 0,5 % |
| Ereignis-Sperrzeit | 2 s |
| Beginn einer Erholung | Änderungsrate mindestens 3 Hz/s |
| Fortsetzung der Erholung | Stabile Plateaus erlaubt; Rückgang stärker als 1,5 Hz/s verwirft die Bestätigung |
| Erforderlicher Nettoanstieg | Mindestens 150 Hz (aktuell dieselbe absolute Mindeständerung wie bei der Abfallerkennung) |
| Regenende-Bestätigung | Mindestens 120 s Erholung ohne erneuten deutlichen Rückgang und 120 s ohne neues Ereignis |
| Anlauf-/Heizwechsel-Sperre | 5 s |
| Maximale Messlücke | 1,5 s |

Filterkoeffizienten berücksichtigen die tatsächlichen Zeitabstände. Die verwendete
EMA-Näherung lautet `alpha = dt / (tau + dt)`. Der schnelle Filter bewahrt kurze
Abfälle besser als der bisherige Sekundenmedian. Der langsame Frequenzfilter
ist zunächst ein Diagnosewert; die Erholungsentscheidung verwendet die geglättete
Änderungsrate des schnellen Filters. Ein stationäres niedriges Signal kann ohne
Erholung nicht allein durch Zeitablauf ein Regenende auslösen. Nach Beginn einer
Erholung darf der Verlauf abflachen; andernfalls könnte ein schneller trocknender
Head nach Erreichen eines Plateaus nie ein Regenende bestätigen. Gegenüber dem
Beginn der Erholung muss ein Nettoanstieg mindestens in Höhe der absoluten
Mindeständerung vorliegen. Ein stärkerer negativer Trend verwirft den Zeitgeber.

Bei Nullsignal, `nan`, Teil-PWM, zurückspringender/doppelter Zeit oder zu großer
Messlücke wird die bekannte Bewertung verworfen und die Auswertung beginnt neu.
Nach einem Heizwechsel werden die Filter neu initialisiert; eine zuvor bekannte
Regen-Schätzung bleibt erhalten, ist während der Sperre aber nicht gültig.
Ein Temperatur- oder Versorgungssprung bei unveränderter Heizung kann weiterhin
wie Benetzung aussehen. Einzelne Störimpulse werden nicht grundsätzlich von
Wassertropfen unterschieden. Die Empfindlichkeit und zusätzliche Kandidaten
müssen deshalb weiter geprüft werden.

Der Kern regelt die Heizung **nicht automatisch**. Zum Live-Vergleich am trockenen
Head starten, normal loggen und dieselben kontrollierten Sprühversuche mit
manuell unveränderter 100-%-Heizung durchführen. Benetzungszeiten, optische
Trocknung und OFF weiter separat notieren. Nach dem Flashen startet die Heizung
weiterhin AUS. Auswertungsparameter aus bekannten Logs sind noch keine unabhängige
Hardwarevalidierung.

Host-Prüfung des Kerns (vom Repository-Root):

```sh
c++ -std=c++11 -Wall -Wextra -Werror \
  -I firmware/lib/RainDetectorEvaluation/src \
  lab/RainDetector-PCNT/tests/test_rain_evaluation.cpp -o /tmp/envnode-test-rain-core
/tmp/envnode-test-rain-core
```

Replay derselben C++-Implementierung gegen ein Log (benötigt Python 3 und `c++`):

```sh
python3 lab/RainDetector-PCNT/tools/replay_evaluation.py \
  lab/RainDetector-PCNT/logs/device-monitor-261019-180137.log \
  --output lab/RainDetector-PCNT/logs/analysis/local-core
```

Es entsteht je Boot-Sitzung eine zusätzliche CSV mit den Kern-Ergebnissen sowie
`core-results.json` und `core-summary.md`. Das bisherige Offline-Verfahren wird
nicht überschrieben. Quellen- und Kern-Hashes machen den Replay-Stand nachvollziehbar.
Der Replay enthält die neue Filterversion gegen bereits bekannte Logs, also
keinen weiteren unabhängigen Blindtest. Im Sprühfolgen-Log `180137` liefert diese
erste Fassung 20 Kandidaten einschließlich des letzten kurzen Impulses. Eine
Regenende-Schätzung folgt bei etwa 567 s seit Boot (rund 126 s nach letztem Impuls).
Zusätzliche Kandidaten in älteren Logs bleiben ungeklärt und werden nicht als
bestätigte Benetzungen gewertet.
