# MQTT und Integration

Unter **MQTT** Brokername beziehungsweise IP, Port und gegebenenfalls Zugangsdaten
setzen. Die Netzwerkverbindung und anschließend den MQTT-Verbindungsstatus prüfen.

## Topic-Struktur

Die Topics beginnen mit dem Topic-tauglichen Gerätenamen:

```text
envnode/<device>/...
```

| Aufgabe | Topic |
| --- | --- |
| Sensormessung | `envnode/<device>/sensor/<SensorId>/<measurement-type>` |
| On/Off-Befehl | `envnode/<device>/actuator/<ActuatorId>/cmd/on_off` |
| On/Off-Zustand | `envnode/<device>/actuator/<ActuatorId>/status/on_off` |
| Controller-Befehl | `envnode/<device>/controller/<ControllerId>/cmd` |
| Controller-Status | `envnode/<device>/controller/<ControllerId>/status` |

On/Off-Befehle verwenden `ON` oder `OFF`; Controller-Befehle `START` oder `STOP`.
Ein Controller-STOP ändert nicht dessen dauerhaft konfigurierte Aktivierung.
Sensor-Messungen werden nicht retained veröffentlicht; Aktor- und Controllerstatus
sind retained. Steuerbefehle nicht retained senden, damit kein alter Befehl beim
Wiederverbinden erneut angewendet wird.

## Erste Kontrolle

1. MQTT-Broker im Gerät konfigurieren und Verbindung prüfen.
2. In einem MQTT-Client `envnode/<device>/#` abonnieren.
3. Messwerte mit der lokalen Seite **Measurements** vergleichen.
4. Aktorbefehle zunächst ohne gefährliche oder empfindliche Last testen.
5. Status nach Neustart und Wiederverbindung kontrollieren.

Die Firmware unterstützt Sensor-Home-Assistant-Discovery.
Discovery für generische Aktoren und Controller ist nicht gleichermaßen als
fertiger Funktionsumfang zugesagt. Speicherung und historische Auswertung
übernimmt das externe System.
