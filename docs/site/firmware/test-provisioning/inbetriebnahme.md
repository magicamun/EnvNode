# Test-Firmware einrichten

## Bauen und aufspielen

Im Repository ist `firmware/` das PlatformIO-Projekt. Für den ESP32 des
Test-Controllers gilt:

```sh
pio run -e testboard
pio run -e testboard -t upload
```

In VS Code: **Project Tasks → testboard → General → Build / Upload**.
Die OTA-Datei liegt in diesem Projektstand unter
`.pio/build/testboard/firmware.bin`. Beim ersten Flashen beziehungsweise beim
Ändern der Partitionstabelle USB verwenden; OTA benötigt eine bereits passende
Firmware und Partitionierung.

`esp32dev` baut die normale EnvNode-Anwendung. `testboard_native` ist dagegen ein
Testziel für den Entwicklungsrechner und keine flashbare ESP32-Firmware:

```sh
pio test -e testboard_native
```

Eigene Logs, Konfiguration und Versionsangaben der Testanwendung verhindern eine
Vermischung mit einem vorherigen EnvNode-Einsatz des ESP32. Nach dem Flashen
Gerätename, WLAN und Sprache/Zeit in der Testoberfläche kontrollieren. Falls noch
kein WLAN eingerichtet ist, den angebotenen Setup-Zugang verwenden.

## Versorgung und Anschluss

Die Messhardware braucht `3V3_SYS` und gemeinsame Masse zwischen Controller und
Input-Adapter. **USB am DevKit allein versorgt die Multiplexer des Controllers
nicht regulär:** Der DevKit-3V3-Sockelkontakt ist dort nicht auf `3V3_SYS` geführt.
ORing-Power nach dem Bestückungsplan einsetzen und seine vorgesehene Versorgung
verwenden. Vor dem Aktivieren von GPIOs an J8/1 gegen J8/2 die Versorgung prüfen.
Eine nur durch Steuerleitungen rückgespeiste Spannung ist keine gültige Versorgung.

Der Input-Adapter wird über das 10-polige Flachbandkabel an Controller J8 verbunden.
Pin-1-Markierungen und Kabelorientierung beachten; Versorgung vor Änderungen an
Brücken und Kabeln ausschalten. Beim elektrischen Selbsttest bleibt der Prüfling
vollständig abgezogen.

| Controlleranschluss | Aufgabe |
| --- | --- |
| J1, 4-polig | GPIO-Testpigtail über U1 |
| J2, 4-polig | I²C-Testpigtail über U1; dieser Anschluss erzeugt Testpegel, keinen I²C-Verkehr |
| J3, 2×7 | Modul-Testanschluss über U2, bei Bedarf mit passivem Moduladapter |
| J8, 2×5 | Input-Adapter; Pin 1 `3V3_SYS`, Pin 2 GND, Pin 9 COM_IN |
| I2C0 mit J5/J6 | Separater Provisioning-Bus, SDA GPIO16 / SCL GPIO5 |

GPIO34/COM_IN verträgt keine 5 V. Auch beim Prüfen eines mit „+5 V“ bezeichneten
Mainboardkontakts speist die Testhardware nur ihr begrenztes 3,3-V-Testsignal ein.

## Erste Anmeldung

Auf der Startseite Version **Testboard 0.13.0** kontrollieren. Netzwerk, Sprache,
Zeitzone, Logs und Firmwareverwaltung stehen unabhängig von einem Prüfling bereit.
Die gemeinsamen HTTP-Verbesserungen aus EnvNode sind übernommen. Ein expliziter
Remote-Profildownload kann weiterhin kurz blockieren; während aktiver Messung
werden solche Downloads nicht neu gestartet.

Alte Browser-Tabs nach einem Wechsel von EnvNode zu Test-Firmware oder zurück
schließen: Deren periodische Statusabfragen können sonst nicht vorhandene Routen
anfragen und entsprechende Meldungen im seriellen Log erzeugen.

Die Hardwarekombination vor der ersten Messung unter
[Controller und Adapter](hardwareprofile.md) auswählen und bestätigen.
