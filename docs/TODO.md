# TODO

- [x] Controller-Zielauswahl: konfigurierten Aktor-Slot und automatisch erkanntes Modulrelais bei identischer Hardware als einen Eintrag mit Benutzername und Modulzuordnung anzeigen. Auch die exklusive Zielbelegung muss beide Referenzarten als dasselbe Ziel erkennen. Bestehende gespeicherte Referenzen kompatibel halten.

- [ ] In einem separaten Thread eine Benutzerdokumentation der Web-Oberfläche erstellen: Seiten und Felder, Speichern/Anwenden/Neustart, Values und Neustartverhalten, Hysterese/Selector sowie Display-Quellen und Übersetzungen. RainControl als durchgängiges Konfigurationsbeispiel verwenden.

- [ ] Values-Seite: MQTT-Befehlstopic und Beispiel-Payload pro Value anzeigen; Befehl und Statusrückmeldung klar unterscheiden.

- [ ] Font-/Clipping-Hardwaretest: zwei 128×64-Displays mit Fixed 6x10 (21×6) und Fixed 8x13 (16×4), lange Zeilen, Umlaute am rechten Rand, Fontwechsel und Neustart prüfen. Software, Speicherung und Vorschau sind umgesetzt und automatisiert geprüft; weitere Pixelgeometrien bleiben ein späterer Schritt.

## Firmware-Releases nach Abschluss der Hardware-Dokumentationsrunde

- Versionsstand je Release-/Versionscommit dokumentieren, einschließlich Quellcommit und Zielhardware.
- Zugehörige geprüfte Binärdateien mit Prüfsummen und Flash-/Updatehinweisen bereitstellen.
- Unterstützte Boardrevisionen und bekannte Einschränkungen pro Release festhalten.
- Für SD-Card 0.1 danach die geprüfte Firmware-Mindestversion in Hardwareanleitung und Kompatibilität ergänzen.

Vorgemerkt am 28.10.2026; Umsetzung als separater Schritt nach der Hardware-Dokumentation.
