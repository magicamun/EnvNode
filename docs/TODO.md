# TODO

- [x] SD-Card Rev. 0.1: Bausatz ENK-00022 und InvenTree-BOM mit sechs Positionen (je 1× PCB ENP-00137, U1 ENP-00136, U2 ENP-00102, C1 ENP-00006, R1 ENP-00018 und J1 ENP-00059) angelegt und validiert. Mitgelieferte U1-Stiftleiste wird nicht zusätzlich gezählt.

- [x] Verkaufs-BOMs ENK-00022 (SD-Card) und ENK-00002 (DuoRelay) um je 1× ENP-00164 (M2,5-Abstandshalter 10 mm), ENP-00165 (M2,5-Schraube 8 mm) und ENP-00166 (M2,5-Mutter) ergänzt und erneut validiert.

- [ ] SD-Card: zwei M2-Schrauben, zwei M2-Abstandshalter 2–3 mm und die benötigten M2-Muttern für das Breakout auswählen, als Artikel anlegen und anschließend in ENK-00022 aufnehmen. Die Adafruit-Herstellerverknüpfung bleibt separat offen.

- [x] SD-Card: Modulprofil/Provisioning, automatische Slot-Erkennung, Geräteinventar, Systemlog und allgemeine Diagnoseschnittstelle integriert; physischer Schreib-/Lesetest, gültige Diagnose-JSONL, Neustartpersistenz und Download bestätigt. Web-Sendelatenz bleibt separat offen.

- [x] Controller-Zielauswahl: konfigurierten Aktor-Slot und automatisch erkanntes Modulrelais bei identischer Hardware als einen Eintrag mit Benutzername und Modulzuordnung anzeigen. Auch die exklusive Zielbelegung muss beide Referenzarten als dasselbe Ziel erkennen. Bestehende gespeicherte Referenzen kompatibel halten.

- [ ] In einem separaten Thread eine Benutzerdokumentation der Web-Oberfläche erstellen: Seiten und Felder, Speichern/Anwenden/Neustart, Values und Neustartverhalten, Hysterese/Selector sowie Display-Quellen und Übersetzungen. RainControl als durchgängiges Konfigurationsbeispiel verwenden.

- [ ] Values-Seite: MQTT-Befehlstopic und Beispiel-Payload pro Value anzeigen; Befehl und Statusrückmeldung klar unterscheiden.

- [ ] Font-/Clipping-Hardwaretest: zwei 128×64-Displays mit Fixed 6x10 (21×6) und Fixed 8x13 (16×4), lange Zeilen, Umlaute am rechten Rand, Fontwechsel und Neustart prüfen. Software, Speicherung und Vorschau sind umgesetzt und automatisiert geprüft; weitere Pixelgeometrien bleiben ein späterer Schritt.

## Firmware-Releases nach Abschluss der Hardware-Dokumentationsrunde

- Versionsstand je Release-/Versionscommit dokumentieren, einschließlich Quellcommit und Zielhardware.
- Zugehörige geprüfte Binärdateien mit Prüfsummen und Flash-/Updatehinweisen bereitstellen.
- Unterstützte Boardrevisionen und bekannte Einschränkungen pro Release festhalten.
- Für SD-Card 0.1 gilt Profilminimum 0.8.6 mit zusätzlicher Prüfung des SD-Betriebstreibers. Öffentlich integrierter Stand ist 0.8.7; ältere 0.8.6-Builds können noch ohne SD-Treiber sein.

Vorgemerkt am 28.10.2026; Umsetzung als separater Schritt nach der Hardware-Dokumentation.

## Dokumentation nach erfolgreicher Erprobung der Testhardware

- [ ] Input-Adapter: Quellenstand-Hinweis aus der Benutzeranleitung entfernen, sobald Hardware und abgestimmte Test-Firmware gemeinsam erfolgreich erprobt sind.
- [ ] Test-Firmware: Bei älteren Mainboards ohne TP1 das unverbundene zweipolige JST-PH-TestPin-Kabel berücksichtigen und das Testergebnis als „unvollständig“ ausweisen; Verhalten am Aufbau validieren.

- [ ] Testhardware-Dokumentation (Test-Controller, Input-Adapter, Modul-Adapter) nach erfolgreicher Erprobung auf die tatsächliche Bedienung abstimmen und überholte Hinweise auf Firmware-Simulation und noch fehlende Firmware-Anbindung entfernen.

- [ ] Input-Adapter in KiCad: TP1-Stiftleiste auf DNP setzen; die Pigtail-Adern werden direkt an die Pads gelötet. In der Kaufteilliste nur die 38 J4-Kontaktpositionen als ENP-00145 zählen.
