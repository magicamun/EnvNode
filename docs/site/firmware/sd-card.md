# SD-Karte und Diagnoseaufzeichnung

**Historischer SD-Stand:** Im klassischen ESP32-Teststand wird die Aufzeichnung
auf [UDP-Netzwerklogging](network-logging.md) umgestellt. Der SD-Code bleibt bis
zur Validierung vorübergehend enthalten. Die folgende Anleitung beschreibt den
bisherigen SD-Betrieb.

Stand: **EnvNode 0.8.7, 3. November 2026**. Lizenz: siehe [Lizenz und Nutzung](../license.md).
Die SD-Karte dient der begrenzten lokalen Diagnoseaufzeichnung. Messungen und
Regelung laufen unabhängig davon; die Aufzeichnung ist keine Wetterhistorie.

## Modul erkennen und Karte vorbereiten

Voraussetzung ist ein geprüftes [SD-Card-Modul 0.1](../modules/sd-card/index.md)
mit passendem EEPROM-Descriptor und einer lesbaren FAT16-/FAT32-Karte.
Mit einer SanDisk Ultra 8 GB und FAT32 wurde der Aufbau erprobt. exFAT ist nicht
der hier geprüfte Betriebsweg; die Typanzeige SDHC/SDXC bestätigt allein noch
kein unterstütztes Dateisystem.

Nach [Provisioning und Neustart](provisioning.md#sd-card-01-provisionieren-und-aktivieren)
erscheint unter **Hardware** das Infrastruktur-Gerät `sd.1` mit `sd-spi v1`,
`storage.sd` und `Ready`. **Ready** bestätigt die Descriptor-/Treiberzuordnung,
nicht die Lesbarkeit einer eingelegten Karte.

Unter **SD-Karte** werden Slot und SD-Zugriff automatisch vom Modul übernommen.
Für ein separat verdrahtetes Breakout ohne Modul-EEPROM können **SD-Zugriff
aktivieren** und Slot A/B manuell gewählt werden. Bei Modulkonflikten ist der
Zugriff gesperrt; nicht durch eine fremde Modulidentität umgehen.

Die Seite zeigt Kartenerkennung, Typ, Kapazität und Verzeichnisse. Angaben zum
freien Speicher können Näherungswerte sein oder fehlen. **Aktualisieren** lädt
die aktuelle Verzeichnisansicht erneut; ein Verzeichnisname öffnet dieses
Verzeichnis. Bei vielen Einträgen die angebotene Seitennavigation verwenden.

## Welche Aufzeichnung einschalten?

| Einstellung | Wirkung |
| --- | --- |
| **Systemlog auf SD aufzeichnen** auf der SD-Seite | Speichert normale Protokollereignisse unter `/logs/system`. |
| **Erweiterte Systemdiagnose** auf der SD-Seite | Gibt System-/Moduldiagnosen frei; der Systemproduzent schreibt unter anderem regelmäßig Laufzeit-/Speicherzustand nach `/logs/diagnostics`. |
| **Erweiterte Diagnose** beim Sensor, Aktor oder Controller | Gibt Diagnoseereignisse dieser konfigurierten Quelle frei, ebenfalls unter `/logs/diagnostics`. |

Die drei Freigaben sind unabhängig: Für einen Controller muss die erweiterte
Systemdiagnose **nicht** zusätzlich aktiviert sein. SD-Zugriff und Karte müssen
verfügbar und die betreffende Quelle aktiviert sein. Anschließend speichern.

Eine Freigabe erzeugt nur Ereignisse, die die jeweilige Implementierung bereits
liefert. Sie ist keine Zusage, dass jeder Sensor oder Controller schon eine
Detaildiagnose enthält. RainDetector-/SCT013-Instrumentierung ist ein eigener
Ausbauschritt; dafür muss der SD-Treiber nicht geändert werden.

Ohne verfügbare Karte bleiben Diagnose-Checkboxen sichtbar, sind aber nicht
editierbar. Bereits gespeicherte Freigaben bleiben erhalten. Sie werden nicht
durch Entfernen der Karte gelöscht. Einstellungen überleben einen normalen
Neustart und Firmwareupdate; sie sind auch in Konfigurationssicherung Schema 5
enthalten. Ältere Sicherungen bleiben lesbar, fehlende neue Diagnosefreigaben
sind dabei ausgeschaltet.

## Dateien, Rotation und Speicherbudget

Beide Aufzeichnungen verwenden JSONL: jede Zeile ist ein eigenständiger
JSON-Datensatz. Am Dateianfang stehen Sitzungs-/Firmwareinformationen;
Protokolle enthalten unter anderem Laufzeit und, sobald verfügbar, UTC-Zeit.
Ohne Zeitsynchronisation darf kein gültiger Kalenderzeitpunkt vorausgesetzt werden.

Beispiel: `boot-0000000020-part-0001.jsonl`. Die erste Nummer identifiziert die
persistierte Start-/Aufzeichnungssitzung, die zweite den Dateiteil. Beide Nummern
sind Zuordnungsmerkmale, kein Kalenderdatum.

- Je Verzeichnis werden Teile von bis zu **1 MiB** und ein Budget von **32 MiB** verwaltet.
- Systemlog und erweiterte Diagnose haben getrennte Budgets, zusammen bis zu **64 MiB** verwaltete Dateien.
- Bei Erreichen des Budgets werden die ältesten passenden verwalteten Dateien gelöscht. Fremde Dateinamen werden nicht zur Bereinigung herangezogen; andere Dateien können die Karte trotzdem füllen.
- Beim Download eines gerade beschriebenen Teils wird dieser abgeschlossen. Weitere Aufzeichnung beginnt in einem neuen Teil. Ein Download kann deshalb einen neuen Dateiteil auslösen, bevor die Größengrenze erreicht ist.

Die Aufzeichnung arbeitet mit begrenzten RAM-Puffern im Hintergrund. Sie wartet
nicht im Messpfad auf die Karte. Bei Überlast können Diagnosen verloren gehen;
Verluste werden soweit möglich gezählt und protokolliert. Noch nicht gesicherte
Daten können bei Stromausfall verloren gehen. Speicherfehler führen zu einer
Wiederholung mit Wartezeit, nicht zu unbegrenztem RAM-Verbrauch. Die Statuszeilen
für Systemlog und erweiterte Diagnose zeigen den Aufzeichnungszustand.

## Download und Löschen

**Download** an der gewünschten Datei lädt sie ohne Ausbau der Karte herunter.
Downloads laufen einzeln und haben ein Zeitlimit von zehn Minuten. Die Karte
währenddessen eingesteckt lassen. JSONL-Dateien können anschließend auf dem
Computer ausgewertet werden.

**Löschen** öffnet zunächst eine Bestätigungsseite. Den dort genannten Pfad
prüfen und erst dann bestätigen. Ein einfacher Aufruf der Bestätigungsseite
löscht noch nichts. Die Aktion betrifft die ausgewählte Datei, keine rekursive
Verzeichnisbereinigung. Das Löschen eines aktiven Logteils beendet nicht die
Aufzeichnung; spätere Einträge werden in einen neuen Teil geschrieben.

## Formatieren

**Karte formatieren** löscht den Karteninhalt einschließlich eigener Dateien.
Vorher benötigte Dateien herunterladen. Die Firmware unterstützt hierfür aktuell
lesbare Karten von **4 bis 32 GB**. Andere oder nicht lesbare Karten zuerst am
Computer passend vorbereiten.

1. Zielkarte, Slot und angezeigte Kapazität prüfen.
2. Die auf der Seite verlangte Bestätigung genau ausführen.
3. Karte bis zum Ergebnis nicht entfernen und Versorgung eingeschaltet lassen.
4. Ergebnis prüfen: FAT32 und leeres Stammverzeichnis werden kontrolliert.

Während der Formatierung können Webzugriffe warten; Messung und Steuerung laufen
weiter. Aktive Aufzeichnung kann anschließend wieder neue Dateien anlegen,
daher ist ein dauerhaft leeres Verzeichnis kein erwarteter Betriebszustand.

## Funktionstests

Der **4-KiB-Schreib-/Lesetest** wird ausdrücklich gestartet. Er legt eine eigene
Testdatei an, schreibt und vergleicht 4096 Bytes und entfernt seine Testdatei.
Er läuft nicht automatisch bei jedem Boot.

**Rotation und Speicherbudget testen** verwendet ein separates Testverzeichnis
mit verkleinerten Testgrenzen. Produktionslogs bleiben erhalten. Das Ergebnis
meldet Rotation, Bereinigung, Datenerhalt, Inhalt und Aufräumen getrennt. Nach
abgebrochenen oder fehlgeschlagenen Tests können Testdateien zurückbleiben.

Bei aktivierter erweiterter Systemdiagnose kann **Diagnose-Testereignis
aufzeichnen** ein `recorder_test`-Ereignis über dieselbe Diagnoseschnittstelle
anfordern. Erfolg beim Einreihen in den Puffer ist noch kein Haltbarkeitsnachweis;
für die vollständige Kontrolle später die Diagnose-Datei herunterladen und prüfen.

## Kartenwechsel und Fehler

Das **Modul selbst nur stromlos umstecken**. Für einen geplanten Kartenwechsel
Downloads und Tests beenden, aktive Aufzeichnungen ausschalten und deren Ende
abwarten; danach möglichst stromlos wechseln. Die Hotplug-Wiederaufnahme ist
eine Fehlerbehandlung, keine Garantie gegen Datenverlust beim Ziehen während
eines Schreibvorgangs.

Bei fehlender Karte werden neue Diagnoseereignisse nicht angenommen. Nach
Wiedereinsetzen kann die Aufzeichnung mit den gespeicherten Freigaben wieder
anlaufen. Die begrenzten Puffer ersetzen keine beliebig lange Ausfallsicherung.
Bei Problemen Dateiliste aktualisieren und Status sowie Systemprotokoll prüfen.

| Befund | Nächster Schritt |
| --- | --- |
| `unknown` / `UnsupportedDeviceKind` | Passende Firmware mit SD-Treiber installieren; für den beschriebenen Stand mindestens 0.8.7. |
| Modulkonflikt | Descriptor und Slotbelegung prüfen; nur ein unterstütztes SD-Modul verwenden. |
| Karte fehlt | Karte, CD-Kontakt, Versorgung und Steckverbindungen prüfen. |
| Karte nicht lesbar / Schreibfehler | FAT-Dateisystem, freien Speicher, Versorgung und Kontakte prüfen; vorhandene Daten vor Neuformatierung sichern. |
| Webantwort verzögert | SD-Status und Protokoll prüfen. Eine verzögerte Antwort allein belegt keinen Kartenfehler. |

Die erfolgreiche Erprobung am Mini mit SD-Card 0.1 ersetzt keine elektrische
Leistungsaufnahmeprüfung oder Langzeitqualifikation für einen konkreten Außeneinsatz.
