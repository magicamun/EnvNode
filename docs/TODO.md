# TODO

- [ ] SD-Card Rev. 0.1: Bausatz/Baugruppe und vollständige BOM in InvenTree erstellen. U1 (ENP-00136, Adafruit 254) und PCB (ENP-00137) sind angelegt; vorhandene Standardteile sind zugeordnet. Direkte Montage über 1×8-Stiftleiste ENP-00019; Lieferumfang prüfen und mitgelieferte Leiste nicht doppelt zählen. Befestigungslängen/-artikel und Adafruit-Herstellerverknüpfung noch offen. Bestückungs- und Bring-up-Anleitung liegt im FullSize/SD-Card-Projekt; physischer Bring-up und SD-Firmwareintegration stehen noch aus.

- [x] Controller-Zielauswahl: konfigurierten Aktor-Slot und automatisch erkanntes Modulrelais bei identischer Hardware als einen Eintrag mit Benutzername und Modulzuordnung anzeigen. Auch die exklusive Zielbelegung muss beide Referenzarten als dasselbe Ziel erkennen. Bestehende gespeicherte Referenzen kompatibel halten.

- [ ] In einem separaten Thread eine Benutzerdokumentation der Web-Oberfläche erstellen: Seiten und Felder, Speichern/Anwenden/Neustart, Values und Neustartverhalten, Hysterese/Selector sowie Display-Quellen und Übersetzungen. RainControl als durchgängiges Konfigurationsbeispiel verwenden.

- [ ] Values-Seite: MQTT-Befehlstopic und Beispiel-Payload pro Value anzeigen; Befehl und Statusrückmeldung klar unterscheiden.

- [ ] Font-/Clipping-Hardwaretest: zwei 128×64-Displays mit Fixed 6x10 (21×6) und Fixed 8x13 (16×4), lange Zeilen, Umlaute am rechten Rand, Fontwechsel und Neustart prüfen. Software, Speicherung und Vorschau sind umgesetzt und automatisiert geprüft; weitere Pixelgeometrien bleiben ein späterer Schritt.
