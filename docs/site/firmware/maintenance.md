# Updates und Diagnose

## Web-OTA

1. Ein passendes `firmware.bin` für das ESP32-Ziel bauen.
2. In der Weboberfläche **Firmware** öffnen und das Anwendungsimage hochladen.
3. Upload-Ergebnis prüfen. Ein erfolgreicher Upload stellt das Image bereit,
   startet das Gerät aber noch nicht neu.
4. **Restart Now** wählen oder später gezielt neu starten.
5. Danach Build-Identität, Sensoren, Aktoren und Kommunikation prüfen.

Web-OTA nimmt Anwendungsimages entgegen. Wenn die laufende Firmware keine
funktionierende Aktualisierung ermöglicht, bleibt USB-Flash der Wiederherstellungsweg.

## Diagnose

| Bereich | Kontrolle |
| --- | --- |
| Status | Netzwerk, Laufzeit und Gerätezustand |
| Diagnostics | Boardprofil, Identity, I²C-Scan und Hardware-Ressourcen |
| Measurements | Vorhandene Messgrößen und Sensorzustand |
| Actuators / Controllers | Schaltzustände und lokale Steuerentscheidungen |
| Logs | Ereignisse und Fehlerursachen |
| Firmware | Version, Build-Identität, Git-Stand und bereitgestelltes Update |

Ein I²C-Scan prüft Erreichbarkeit, nicht die richtige Sensorimplementierung.
Fehlende Werte zunächst lokal untersuchen, bevor Broker oder Home Assistant
als Ursache angenommen werden.

## Factory Reset

**Factory reset** löscht alle gespeicherten Konfigurationen und startet in den
Einrichtungsmodus. Vorher Netzwerk-, MQTT- und Hardwareeinstellungen dokumentieren.
Ein normaler Neustart ist der passende erste Schritt, wenn die Konfiguration
beibehalten werden soll.
