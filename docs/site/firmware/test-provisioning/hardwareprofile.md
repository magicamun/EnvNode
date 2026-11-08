# Controller und Adapter auswählen

Seit Test-Firmware **0.12.0** sind Controller und Input-Adapter getrennte
Hardwareprofile. Sie beschreiben die Prüfvorrichtung; das Mainboard-Prüfprofil
beschreibt weiterhin nur den Prüfling und seine erwarteten Verbindungen.

## Unterstützte Kombinationen

| Auswahl | Besonderheit |
| --- | --- |
| Test-Controller 0.1 (Legacy) | Geführter Input-Selbsttest speist über Controller J3/2 |
| Test-Controller 0.2 (TP2) | Input-Selbsttest speist direkt über TP2/2 TEST_DRIVE |
| Input-Adapter 0.1 (Legacy) | Produzierte ursprüngliche Platine, Teststecker J4 |
| ESP32 DevKitC Adapter 0.3 | Neue Kanalzuordnung, Teststecker J2 mit 2×19 Kontakten |

Beide Controller können mit beiden Adaptern kombiniert werden. In allen Fällen
werden 38 Socketkontakte und zwei Referenzkontakte geprüft. Der passive
Moduladapter 0.2 verändert die elektrische 1:1-Zuordnung nicht.
Ein ESP32-S3-Adapter mit 44 Kontakten ist noch nicht auswählbar; dafür werden später
Adapterprofil, Eingangsanzahl, Prüfprofile und Darstellung erweitert.

## Auswahl übernehmen

1. Messung beziehungsweise Selbsttest beenden und alle Messpfade abschalten.
2. Prüfling und Provisioning-Kabel trennen; nur die gewünschte Testhardware aufbauen.
3. Unter **Anschlussprüfung → Testhardware** den tatsächlich aufgebauten Controller
   und den tatsächlich angeschlossenen Adapter wählen.
4. Die getrennte Prüflingsverbindung und Hardwareauswahl bestätigen und
   **Hardware übernehmen** drücken.
5. Anschließend Selbsttest beziehungsweise einen neuen Mainboard-Prüflauf beginnen.

Die Kombination wird in den Tester-Einstellungen gespeichert und nach Neustart
wiederhergestellt. Ohne gespeicherte Auswahl gilt die bisherige Legacy-Kombination.
Es gibt keine automatische Hardwareerkennung. Vor jeder Messung die aktive
Kombination kontrollieren; die Firmware kann falsch angeschlossene Hardware nicht erkennen.

Ein Wechsel verwirft alle Mainboard-Ergebnisse einschließlich Rohwerthistorie und
alle aktuellen Selbsttestergebnisse. Während aktiver Messung oder Diagnose ist er
gesperrt. Scheitert das Speichern, bleiben die vorherige Auswahl und Sitzung erhalten.
Der Wechsel selbst aktiviert keine GPIOs. Selbsttest-Browserdaten sind nach
Hardwarekombination getrennt; geöffnete Selbsttestseiten laden bei einem Wechsel neu.
Die Mainboard-JSON-Datei enthält Fixture-ID und beide ausgewählten Profile.

## Selbsttest am Controller 0.2

TP2 hat **Pin 1 GND, Pin 2 TEST_DRIVE, Pin 3 COM_IN**. TEST_DRIVE liegt hinter
R6 (1 kΩ), vor beiden Output-Multiplexern. Es ist ein geschaltetes Testsignal,
keine permanente 3,3-V-Versorgung.

- **Outputs:** Den jeweils angezeigten J1-/J2-/J3-Kontakt mit **TP2/3 COM_IN**
  verbinden. Input-Adapter abziehen. Der Ablauf prüft LOW/HIGH, abgeschalteten
  Output-Pfad und Trennung zu den anderen Output-Kanälen.
- **Inputs:** Adapter anschließen. **TP2/2 TEST_DRIVE** mit genau dem angezeigten
  Socket- oder Referenzkontakt verbinden. Beide Output-Multiplexer bleiben aus;
  die Firmware schaltet nur die passenden Input-Kanäle. J3/2 wird nicht benötigt.
- **TP2/1 GND** ist die echte Controllermasse beziehungsweise LOW-Referenz. Diese
  Masse nicht mit MB_GND oder anderen Prüflingskontakten brücken.

Nur die angezeigte Brücke stecken, keinen Prüfling anschließen und vor dem Umstecken
Versorgung ausschalten. Die geführte Seite zeigt die Kontakte passend zur Auswahl.
Die Auswahl 15/50/100 ms und die bestehenden Grenzwerte bleiben verfügbar; für die
praktische Inbetriebnahme hat sich **100 ms** als sinnvoll erwiesen.

## Stand der Abnahme

Die Legacy-Kombination und die Mainboard-Messlogik wurden praktisch erprobt.
Neue Controller-/Adapterprofile sind aus den gespeicherten PCBs geprüft, durch
Softwaretests abgesichert und müssen nach Fertigung am realen Aufbau abgenommen werden.
Mainboard-Provisioning bleibt ein eigener Ablauf über SDA16/SCL5; seine praktische
Erprobung und Vervollständigung ist der nächste Arbeitsschritt. Modul-Provisioning
ist weiterhin deaktiviert.
