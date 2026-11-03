# Updates und Diagnose

## Konfiguration sichern und wiederherstellen

Unter **Konfiguration (Configuration)** eine Sicherung herunterladen.
Passwörter werden nur nach ausdrücklicher Auswahl mit exportiert. Eine Sicherung
mit Passwörtern vertraulich aufbewahren und nicht für Fehlerberichte veröffentlichen.

Für die Wiederherstellung die JSON-Datei hochladen und zuerst die Vorschau prüfen.
Erst nach Kontrolle des Zielgeräts und der geplanten Änderungen die Wiederherstellung
bestätigen. Vorher eine Sicherung des aktuellen Zustands erstellen.
Nach erfolgreichem Speichern und Prüfen der Wiederherstellung das Gerät neu
starten; die laufende Konfiguration wird nicht allein durch die Vorschau ersetzt.
Die Konfigurationssicherung ersetzt keine separate Prüfung der EEPROM-Identitäten.

Standort und lokale Temperaturauswahl für den Meereshöhendruck gehören ab 0.6.0
zur Sicherung und erscheinen in der Wiederherstellungsvorschau. Ältere Formate
bleiben lesbar; fehlende Einstellungen werden als nicht konfiguriert übernommen.
[Details zur Kompatibilität](geo-position-pressure.md#speicherung-und-sicherungen)

## Web-OTA

1. Ein passendes Anwendungsimage unter [Downloads](downloads.md) laden oder selbst bauen.
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
