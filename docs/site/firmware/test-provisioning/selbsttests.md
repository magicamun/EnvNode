# Testhardware und Pigtails prüfen

Ein Selbsttest prüft die **Prüfvorrichtung**, noch kein Mainboard. Vorher alle
Prüflinge, Module und das externe Provisioning-Kabel abziehen. Die Messversorgung
muss stabil anliegen. Kontakte am freien Referenzkabel sind hier Messpunkte;
`MB_GND` darf nicht mit der echten Controllermasse verwechselt werden.

## Manuelle Hardwarediagnose

Unter **Hardwarediagnose** (`/diagnostics`) lassen sich Output U1/U2 und Input
U4/U5/U6, jeweils mit Hardwarekanal I0–I15, auswählen. Dazu kommt LOW/HIGH für den
Signalausgang GPIO17. Erst nach Bestätigung und **Schalten / 60 Sekunden freigeben**
werden reale GPIOs angesteuert; eine bloße Dropdown-Änderung schaltet nichts um.

Bei Input **Alle aus** wird GPIO34 direkt über COM_IN geprüft. Ohne Brücke zieht
R9 den Eingang gegen GND, sodass ein ADC-Wert nahe null erwartet wird.
Die Anzeige ist ein 12-Bit-Rohwert, keine kalibrierte Spannungsmessung.

Beispiel eines bereits praktisch erprobten Wegs:

1. Stromlos Controller **J3-Pin 2 mit J8-Pin 9** verbinden.
2. **U2**, Output-Kanal **6**, Input **Alle aus** wählen.
3. LOW schalten: ADC nahe 0. HIGH schalten: ADC deutlich höher.
4. Bei HIGH auf Kanal 7 wechseln und erneut schalten: ADC zurück nahe 0.

R6 (1 kΩ), Multiplexerwiderstand und R9 (2,2 kΩ) begrenzen das Signal am ADC;
bei HIGH wird daher kein Vollausschlag erwartet. Am bisherigen Aufbau wurden auf
einem direkten Schleifenpfad ungefähr 2273 ADC-Schritte beobachtet. Dieser Wert
ist ein Vergleichspunkt dieses Aufbaus, kein universeller Sollwert.

**Alle Multiplexer abschalten**, WLAN-Verlust oder 60 Sekunden nach dem letzten
Schaltbefehl deaktivieren die Messpfade und geben GPIO17 frei. Statusabfragen
verlängern diese Freigabe nicht. Ein Neustart startet ohne aktive Messpfade.

## Geführter Hardware-Selbsttest

Unter **Hardware-Selbsttest** (`/selftest`) entfällt das manuelle Eingeben der
Multiplexeradressen. Die Anschlussliste dient auch zur freien Auswahl und
Wiederholung eines Kontakts. Während eines Laufs ist die Auswahl gesperrt.

| Gruppe | Verbindung für den jeweiligen Lauf |
| --- | --- |
| 22 Outputs: J1/1–4, J2/1–4, J3/1–14 | Input-Adapter abziehen; ausgewählten Controllerkontakt mit Controller J8/9 (COM_IN) brücken |
| 38 Sockelkontakte am Input-Adapter | Input-Adapter anschließen; Controller J3/2 mit ausgewähltem Adapter J4-Pin verbinden |
| 2 Referenzeingänge | Controller J3/2 mit Adapter TP1/2 (`MB_3V3_SYS`) beziehungsweise TP1/1 (`MB_GND`) verbinden; Referenzkabel an keinem Mainboard angeschlossen |

1. Kontakt auswählen und die angezeigte Verbindung lesen.
2. Versorgung ausschalten, nur diese Brücke stecken und wieder einschalten.
3. Verbindung bestätigen und **Diesen Kontakt prüfen** starten.
4. LOW/HIGH und Trennung werden automatisch gemessen; danach sind die Pfade aus.
5. Ergebnis ansehen, nächsten Kontakt frei auswählen oder **Nächster Kontakt** nutzen.

Geprüft werden der gewählte Pfad bei LOW und HIGH, dessen Abschaltung sowie die
Trennung zu den anderen zugeordneten Kanälen der Gruppe. Unbenutzte physische
Multiplexerkanäle sind nicht Teil dieses Plans. Ein Kontaktlauf dauert je nach Einschwingzeit typischerweise
ein bis fünf Sekunden. Nach 10 Sekunden ohne Abschluss wird abgebrochen.

Die vorläufigen Grenzen sind **LOW/Trennung ≤ 200**, **HIGH 1000–3800**.
Die Einschwingzeit nach jeder Schaltung ist seit 0.9.2 mit **15, 50 oder 100 ms**
wählbar; bisheriger Standard bleibt 15 ms. Danach werden unverändert acht
Messungen aufgenommen. Für einen Zeitvergleich dieselbe Brücke bei 15 und
100 ms testen; das Protokoll hält die gewählte Wartezeit je Ergebnis fest.
Die Tabelle zeigt LOW-/HIGH-Mittelwerte und den höchsten Trennungswert.
Seit 0.9.1 stehen daneben der zugehörige Kontakt samt Multiplexer/Kanal,
Mittelwert, Minimum und die Anzahl der acht Werte über 200. Oberhalb der Tabelle
erscheinen für den ausgewählten Kontakt alle acht Einzelwerte des Fensters mit
dem höchsten Spitzenwert sowie die Grenzüberschreitungen über sämtliche
Trennungsfenster. Bei gleichem Spitzenwert wird das Fenster mit höherem Mittelwert
angezeigt. „Alle Input-/Output-Multiplexer aus“ bezeichnet die Abschaltprüfung,
keinen zusätzlichen Steckerpin.

Die angezeigten Einstellungen für die manuelle Diagnose lassen sich zum Nachmessen
verwenden: Brücke unverändert lassen, genannten Pfad aktiv schalten und COM_IN
gegen GND messen. Ein Multimeter kann kurze Spitzen übersehen; das Gegenprüfen
hilft zunächst zwischen dauerhaftem Pegel und Umschalt-/Erfassungseffekt zu unterscheiden.
Die Messreihenfolge, acht Messwerte und Fehlergrenze 200 bleiben unverändert.
Nur die ausdrücklich gewählte Wartezeit vor dem Messfenster ändert sich.
Ältere Browserergebnisse bleiben erhalten, haben aber keine nachträglich erfundenen
Detaildaten: Für diese Kontakte muss der Test erneut ausgeführt werden.
**Fehler** bedeutet, dass der Messweg diese Bedingungen nicht erfüllt; Ursache
können auch eine falsche Brücke, fehlende Versorgung oder schlechte Kontaktierung sein.

## Pigtails einbeziehen

Für GPIO- und I²C-Pigtails die Brücke am **freien Steckerkontakt** ansetzen,
statt am zugehörigen J1-/J2-Lötpad. In der Liste weiterhin den entsprechenden
Steckerpin auswählen. Damit umfasst der Weg zusätzlich Lötstelle, Kabel und
Steckerkontakt. Den Moduladapter später genauso in den Weg von J3 einbeziehen.

Das separate I2C0-Provisioning-Kabel wird hierbei nicht geprüft.

## Ergebnisprotokoll und Abbruch

Firmwareergebnisse liegen im RAM. Die Seite sammelt sie zusätzlich im Browser für
die Tabsitzung, damit das stromlose Umstecken mit ESP32-Neustart den bisherigen
Bericht nicht verliert. Eine unterbrochene Wiederholungsmessung darf dabei nicht
als vorheriges bestandenes Ergebnis gelten. **Protokoll herunterladen** exportiert
JSON mit Kontakt, Brückenanweisung, Grenzwerten, Firmwareversion und Messwerten.
Vor dem Schließen des Tabs herunterladen. **Ergebnisse löschen** startet einen
neuen gesammelten Bericht.

Manuelle Diagnose, geführter Selbsttest, Anschlussprüfung und Provisioning teilen
sich die Hardware und sperren konkurrierende Eingriffe. **Abbrechen / alles
abschalten** beendet den Selbsttest auch vorzeitig. Das Verlassen der Seite sendet
zusätzlich einen bestmöglichen Stop-Befehl; ein Kommunikationsabbruch ist kein
bestandenes Prüfergebnis.
