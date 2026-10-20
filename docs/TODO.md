# TODO

- [x] SD-Card Rev. 0.1: Bausatz ENK-00022 und InvenTree-BOM mit sechs Positionen (je 1× PCB ENP-00137, U1 ENP-00136, U2 ENP-00102, C1 ENP-00006, R1 ENP-00018 und J1 ENP-00059) angelegt und validiert. Mitgelieferte U1-Stiftleiste wird nicht zusätzlich gezählt.

- [ ] SD-Card: Bestellte Distanzhülsen erst nach Prüfung durch den Nutzer in Bestand und BOM aufnehmen; bis dahin ausdrücklich ausgeschlossen. Adafruit-Herstellerverknüpfung, physischer Bring-up und SD-Firmwareintegration bleiben offen.

- [x] Controller-Zielauswahl: konfigurierten Aktor-Slot und automatisch erkanntes Modulrelais bei identischer Hardware als einen Eintrag mit Benutzername und Modulzuordnung anzeigen. Auch die exklusive Zielbelegung muss beide Referenzarten als dasselbe Ziel erkennen. Bestehende gespeicherte Referenzen kompatibel halten.

- [ ] In einem separaten Thread eine Benutzerdokumentation der Web-Oberfläche erstellen: Seiten und Felder, Speichern/Anwenden/Neustart, Values und Neustartverhalten, Hysterese/Selector sowie Display-Quellen und Übersetzungen. RainControl als durchgängiges Konfigurationsbeispiel verwenden.

- [ ] Values-Seite: MQTT-Befehlstopic und Beispiel-Payload pro Value anzeigen; Befehl und Statusrückmeldung klar unterscheiden.

- [ ] Font-/Clipping-Hardwaretest: zwei 128×64-Displays mit Fixed 6x10 (21×6) und Fixed 8x13 (16×4), lange Zeilen, Umlaute am rechten Rand, Fontwechsel und Neustart prüfen. Software, Speicherung und Vorschau sind umgesetzt und automatisiert geprüft; weitere Pixelgeometrien bleiben ein späterer Schritt.
