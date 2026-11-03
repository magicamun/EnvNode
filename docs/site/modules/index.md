# Erweiterungsmodule

Die vollständigen Aufbauanleitungen stehen direkt auf den jeweiligen Modulseiten:

| Modul | Aufgabe | Dokumentierter Stand |
| --- | --- | --- |
| [AnalogHydroPressure](analog-hydro-pressure/index.md) | Analoger Drucksensoranschluss und lokale 5-V-auf-24-V-Versorgung | Revision 0.6 |
| [SD-Card](sd-card/index.md) | Träger für Adafruit-254-MicroSD-Breakout | Revision 0.1; öffentliche SD-Unterstützung ab Firmware 0.8.7 |
| [DuoRelay](duo-relay/index.md) | Zwei über AUX_GPIO gesteuerte 5-V-Wechslerrelais | Revision 0.3 |

Die Produktseiten enthalten Verdrahtung, Bestückung, Inbetriebnahme und PDF-Anleitungen.
Wähle den Modulslot und die Firmware-Konfiguration passend zur tatsächlichen Hardware.
Die Firmware muss den zum Descriptor passenden Treiber unterstützen. Beim SD-Card-Modul werden Slot und SD-Zugriff dann automatisch übernommen; die gewünschten Aufzeichnungen werden separat eingeschaltet. Ohne gültigen Descriptor stehen keine daraus abgeleiteten modulabhängigen Einstellungen zur Verfügung.
