# SCT013 in Betrieb nehmen

Ziel: einen gültigen Effektivstrom in Ampere erfassen. Vorausgesetzt werden
passende EnvNode-Firmware ab **0.8.14**, eine erreichbare Weboberfläche und ein
[elektrisch geprüfter Head Rev. 0.2](../sct013-head/assembly.md) beziehungsweise
ein entsprechend korrigierter Rev.-0.1-Aufbau. Nennstrom und Spannungsausgang
des konkreten Stromwandlers müssen bekannt sein.

## Sensor konfigurieren

Unter **Sensoren** einen freien Slot als **SCT013 AC RMS** konfigurieren,
einen verständlichen Namen vergeben und aktivieren. Am Mini ist ADC1 GPIO34 und ADC2 GPIO35; beide verwenden die ADC-Einheit 1 des ESP32. Nur ein im Boardprofil verfügbarer, freier Analogport kann belegt werden. Ausgabe: Effektivstrom in Ampere (`electric_current`), auch über MQTT und Home Assistant. Der Treiber berechnet keine Leistung oder Energie.

Für den getesteten Head Rev. 0.2 bzw. reparierten Prototyp gelten folgende **Lab-Vergleichswerte**:

| Sensor | Nennstrom / Ausgang | Kalibrierfaktor | Tiefpass | Aus / Ein |
| --- | --- | --- | --- | --- |
| getesteter 1-V-Sensor | 5 A / 1000 mV | 1,48 | 200 Hz | 170 / 200 mA (DMA am Test-Mini) |
| 0,333-V-Sensor | 5 A / 333 mV | 1,00 (nicht absolut kalibriert) | 200 Hz | 180 / 220 mA |

Die Voreinstellung für neue Slots ist 5 A / 1000 mV, Faktor 1, Tiefpass 200 Hz, Schwellen 0/0 (aus). Einstellungen pro Sensor speichern. Ein Faktor korrigiert den Maßstab, die Schwellen betreffen den bereits kalibrierten Strom. Nach Profil- oder Faktorwechsel die Nullschwellen erneut prüfen. Ein aktives Gate hält seinen Zustand zwischen den Schwellen; ein stromloser oder fehlender Sensor ist nicht unterscheidbar.

Nach dem Speichern ausstehende Sensoränderungen anwenden und unter **Messwerte**
Quelle, Einheit, Gültigkeit und wiederholte Aktualisierung prüfen.
Nach Neustart die Einstellungen und Messung erneut kontrollieren und
[Konfiguration sichern](../../firmware/maintenance.md).
Ein ungültiger Stromwert bedeutet weder 0 A noch eine nachgewiesen stromlose Leitung.

## Funktion mit Referenz prüfen

Zunächst Lampe aus/an/aus mit aktivem WLAN und MQTT, danach mit dem Referenzgerät vergleichen. Filter und Kalibrierung wurden zunächst im Lab und danach im DMA-Testbuild auf dem Mini mit parallelem RainDetector geprüft. Beim 1-V-Head bleibt der einzelne Kalibrierfaktor 1,48; es wird keine stromabhängige Korrekturkurve angewendet. Die elektrische Buchsenprüfung steht in der [Aufbauanleitung](../sct013-head/assembly.md).

**Portauswahl:** Auf bekannten Mini- und Weather-Profilen zeigt das Formular
die Anschlussbezeichnung der Platine: ADC1/ADC2 am Mini, ADC1 bis ADC4 an der Weatherstation.
Die elektrische Belegung und bestehende gespeicherte Slots bleiben gleich.

## Erfassung und Diagnose ab Firmware 0.8.14

 Der Standardbuild `esp32dev` nutzt auf dem klassischen
ESP32 die erprobte ADC-DMA-Erfassung über I2S0. Es kann genau ein SCT013-Slot
gleichzeitig aktiv sein. RainDetector verwendet den separaten Pulszähler und
kann parallel laufen. Je zehn bei 20 kHz erfasste Rohcodes werden für die
2-kHz-Auswertung gemittelt. Die erweiterte Diagnose kennzeichnet diesen Pfad
mit `sct013_dma` und `acquisition=adc_dma`.

Bei DMA-Verwerfungen erscheint statt eines Stromfensters `sct013_dma_error`,
bei anhaltendem Fehler höchstens einmal je Sekunde. Die vier Zähler für
Lesefehler, falsche Kanalkennung, zu späte Abholung und falsche Datenlänge
laufen seit Start der Erfassung. `last_read_code` ist der letzte ESP-Fehlercode
(0: bisher keiner), `last_bad_channel` die unerwartete ADC-Kanalnummer
(-1: bisher keine). Ein Timeout bei leerem Puffer ist beim nichtblockierenden
Lesen normal und zählt nicht als Fehler.

Bei DMA-Fehlern wird die Erfassung vollständig neu initialisiert. Teilfenster und
gepufferte Daten werden verworfen; erst ein vollständiges neues Messfenster kann
wieder gültig sein. Wiederholte Neustartversuche erfolgen höchstens alle fünf
Sekunden ohne Warteblockade der Hauptschleife. `sct013_dma_recovery` meldet
`attempts`, `successes`, `last_code` (0 = Neustart erfolgreich) und den Messzustand.
Ein erfolgreicher Neustart allein bedeutet noch keinen gültigen Stromwert.

### OTA im aktuellen Quellstand

Die folgende Erweiterung ist ab Quellcommit `e572c2d` enthalten. Ein älteres,
unter derselben Versionsnummer veröffentlichtes Image enthält sie nicht
automatisch; Build-Commit mit der [Downloadseite](../../firmware/downloads.md) vergleichen.

Während eines OTA-Firmwareuploads wird SCT013-DMA vor dem ersten Flash-Zugriff
angehalten. Messungen sind währenddessen ungültig (Diagnose `sct013_ota_pause`),
nicht 0 A. Bei Erfolg bleibt die Erfassung bis zum Geräteneustart gestoppt.
Nach Uploadabbruch oder Fehler startet sie neu und wartet auf ein vollständiges
Messfenster. Kann DMA nicht sicher angehalten werden, wird der Upload abgelehnt.

## Messfenster beurteilen

Der Treiber erfasst unabhängig vom Ausgabeintervall Fenster mit 2000 Samples bei nominell 2 kHz. Für den Vergleich zwischen ungefiltertem und gefiltertem Effektivwert entfallen die ersten 100 ms. Ein Tiefpass begrenzt auch echte Oberwellen. Bei **erweiterter Diagnose** zeigt das Ereignis `sct013_window` den Mittelpunkt, ungefilterten und gefilterten Strom vor dem Gate, Timingfehler und Gate-Zustand. Fenster mit unplausiblem Mittelpunkt, Bereichs- oder Timingfehlern sowie veraltete Messungen liefern keinen gültigen Stromwert. Eine ungültige Messung wird nicht als 0 A ausgegeben.
