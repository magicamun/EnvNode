# Ein Mainboard prüfen

**Stand 0.9.1:** Die Anschlussprüfung ist mit der echten Messhardware verbunden.
Die vollständige Boardabnahme einschließlich der Diodenpfade wird jetzt erstmals
praktisch erprobt. Vor einer Bewertung zunächst
[Controller, Adapter und Pigtails prüfen](selbsttests.md).

## Prüfling vorbereiten

- Boardtyp, Hardware- und PCB-Revision eindeutig feststellen.
- **ESP32 aus dem Prüfling entfernen.** USB, externe Versorgung, Sensoren,
  Aktoren und eingesteckte Module entfernen; der Prüfling bleibt spannungslos.
- Provisioning-Kabel vollständig abziehen. Es ist kein Messpigtail und verbindet
  gegebenenfalls zusätzliche Masse-/Versorgungspfade.
- Ursprünglichen Input-Adapter 0.1 mit dem Controller verbinden und korrekt
  orientiert auf den 38-poligen ESP32-Steckplatz des Prüflings setzen.
- Bei Profilen mit 40 Eingängen das zweipolige Referenzkabel zusätzlich an den
  Mainboard-Messanschluss stecken: TP1/1 `MB_GND`, TP1/2 `MB_3V3_SYS`.
  Diese Referenzen sind Prüflingskontakte; keine zusätzliche Brücke zur echten
  Controllermasse herstellen. Die Dioden auf dem Mainboard liegen im Prüfpfad.

Vor dem Aufbau die Versorgung ausschalten. Die Testhardware anschließend versorgen,
den Prüfling weiterhin ohne eigene Versorgung lassen.

## Boardprofil und Betriebsart wählen

**Anschlussprüfung** (`/test`) öffnen. Auf echter Testhardware ist beim Start
**Reale Messung · ursprünglicher Input-Adapter 0.1** ausgewählt. Den sichtbaren
Modushinweis kontrollieren. Die Betriebsart wird nicht als Geräteeinstellung
persistiert; nach Neustart gilt wieder reale Messung.

Board und genaue Revision aus der Liste auswählen und **Board laden / neuer Testlauf**
drücken. Der Katalog zeigt Remote- und eingebaute Profile an. Das geladene Profil,
dessen Quelle und PCB-Revision stehen unter **Quelle der Prüfdaten**. Die Zeichnung
zeigt das zu prüfende Board von oben, nicht die Unterseite des Adapters.

Eingebaute Profile sind Weather 0.2 / PCB 0.2.1, Weather 0.3 / PCB 0.3.0,
Mini 0.8 / PCB 0.8.1 und Mini 0.9 / PCB 0.9.0. Weitere Remote-Einträge müssen
zum unterstützten Profilformat und zur 38-/40-Eingangs-Hardware passen.

Die **Simulation** ist ein ausdrücklich anderer Modus. Dort werden die Sollverbindungen
als Messwerte nachgebildet; optionale Unterbrechung/Zusatzverbindung dienen dem
Test der Oberfläche. Es werden keine GPIOs angesteuert. **Ein Moduswechsel verwirft
alle bisherigen Ergebnisse**, auch wenn dasselbe Board geladen bleibt.

## Einen Anschluss prüfen

1. Anschluss in der Liste auswählen. Die Anwendung nennt den passenden GPIO-,
   I²C- oder Modul-Pigtail und markiert die zu prüfende Buchse im Boardbild.
2. Nur den geforderten Pigtail an diese Buchse stecken. Die übrigen Messpigtails
   bleiben frei; mehrere gleichzeitig verbundene Pigtails können zusätzliche Wege erzeugen.
3. Den spannungslosen Aufbau, den entfernten ESP32 und die richtige Verbindung
   mit der Checkbox bestätigen.
4. **Verbunden – Prüfung starten** drücken. Die Firmware misst jeden Kontakt gegen
   alle 38 beziehungsweise 40 Eingänge. Bei vierpoligen Anschlüssen sind das
   152/160 Einzelprüfungen, beim 14-poligen Modulanschluss 532/560.
5. Auf den Abschluss warten. Der Messpfad wird danach deaktiviert. Ergebnis ansehen.
6. Erst dann umstecken und den nächsten Anschluss auswählen. **Nächster Anschluss**
   sucht einen noch ungeprüften Eintrag; freie Auswahl und Wiederholung sind möglich.

Es gibt absichtlich keinen „Alle prüfen“-Knopf: Das Umstecken ist ein manueller
Arbeitsschritt. Die Firmware kann bei elektrisch identischen Buchsen nicht sicher
erkennen, ob tatsächlich zur nächsten Buchse umgesteckt wurde.

## Was elektrisch gemessen wird

Die Fixture-Tabelle übersetzt logische Kontakte in Multiplexer und Kanal.
Der Messtreiber kennt dabei **keine erwarteten Verbindungen**: Er misst erst LOW,
dann HIGH und gibt ausschließlich „verbunden“, „getrennt“ oder „Messfehler“ zurück.
Die darüberliegende Prüfmatrix vergleicht dieses Ergebnis mit dem Board-Prüfprofil.

Pro Phase werden acht ADC-Werte mit mindestens 2 ms Abstand erfasst, nach zunächst
20 ms LOW- beziehungsweise 40 ms HIGH-Einschwingzeit. Vorläufige Grenzwerte:

| Beobachtung | Treiberergebnis |
| --- | --- |
| Alle LOW-Werte ≤ 200 und alle HIGH-Werte ≤ 200 | Getrennt |
| Alle LOW-Werte ≤ 200 und alle HIGH-Werte 800–3800 | Verbunden |
| LOW zu hoch, HIGH im Zwischenbereich, instabile/grenzübergreifende Werte oder Sättigung | Messfehler |

Jeder Pfad wird nach Auswertung abgeschaltet. Ein interner Messpfad darf höchstens
500 ms dauern; auch die Prüfsteuerung hat eine Fehlerzeitgrenze. **Abbrechen** und
WLAN-Verlust geben die Hardware frei. Die Oberfläche sendet beim Verlassen während
eines Laufs zusätzlich einen bestmöglichen Abbruch.

Die LOW/HIGH-Messung ist eine Kontaktprüfung, keine Widerstandsmessung und keine
Prüfung aktiver Boardfunktionen. Dioden, Pull-ups, Kapazitäten und weitere reale
Schaltungspfade beeinflussen die Messwerte. Die Grenzen sind noch nicht an allen
Mainboards validiert. Ein Messfehler ist deshalb zunächst zu untersuchen; er darf
nicht durch Simulation oder pauschales Ignorieren in „bestanden“ verwandelt werden.

## LEDs, Matrix und Gesamturteil

| Anzeige | Bedeutung |
| --- | --- |
| Grau / `·` | Noch nicht geprüft |
| Grün / `●` | Erwarteter Durchgang vorhanden |
| Grün / `○` | Erwartete Trennung vorhanden |
| Rot / `!` | Unerwarteter Durchgang |
| Rot / `×` | Erwarteter Durchgang fehlt |
| Rot / `E` | Messfehler / uneindeutige Messung |
| Orange / unvollständig | Mindestens ein Kontakt hat keinen positiven, vollständig abgedeckten Prüfpfad |

Blaue Markierungen zeigen den gerade geprüften Kontakt. Die Anschlussliste hält
die Ergebnisse der zuvor geprüften Buchsen; eine Wiederholung ersetzt nur das
Ergebnis dieses Anschlusses. Ein Gesamturteil ist erst nach allen geforderten
Anschlüssen sinnvoll. Übersprungene Modulanschlüsse bleiben ungeprüft und führen
nicht zu einem vollständigen „bestanden“.

Bei älteren Revisionen fehlt ein separater `3V3_SYS`-Referenzanschluss. Solche
Kontakte bleiben je nach Profil ausdrücklich unvollständig prüfbar. Die vier
Steckerkontakte einfach fehlerfrei durchzulaufen, beweist ohne einen positiven
Prüfpfad noch keinen Durchgang dieses Kontakts.

Anschlussprüfergebnisse bleiben derzeit im RAM und gehen beim Neustart oder Laden
einer neuen Sitzung verloren. Der JSON-Download des Hardware-Selbsttests ist ein
eigenes Protokoll; eine Speicherung der Mainboard-Prüfergebnisse im EEPROM des
Prüflings ist noch nicht implementiert.
