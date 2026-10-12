# Lokal automatisch steuern

Controller verbinden eine lokale Regel oder Zeitfunktion mit einem konfigurierten
Aktor. Der Zielaktor muss bereits funktionieren. Controller sind keine allgemeine
Wetterauswertung und ersetzen keine externen Langzeit-Automationen.

| Controller | Aufgabe |
| --- | --- |
| Blink | On/Off-Aktor mit Ein- und Aus-Dauer periodisch schalten |
| Threshold / Hysteresis | On/Off-Aktor abhängig von einem numerischen Messwert steuern |
| Selector | Zwischen automatischer Entscheidung und festem Ein/Aus anhand eines Enum-Values wählen |

## Grundablauf

1. Unter **Controller (Controllers)** freien Platz und Implementierung wählen.
2. Einen passenden vorhandenen On/Off-Zielaktor zuordnen.
3. Implementierungsspezifische Parameter einstellen.
4. Dauerhafte Aktivierung prüfen, speichern und gegebenenfalls Änderungen anwenden.
5. Laufzeitfunktion Start/Stop verwenden und Entscheidung, Grund sowie Aktorzustand prüfen.

Ein Ziel darf nicht von mehreren aktivierten Controllern beansprucht werden.
STOP ändert nicht automatisch die gespeicherte Aktivierung; vor einem Neustart
deshalb die dauerhaft gewünschte Konfiguration prüfen.

## Threshold verstehen

Messquelle und Messgröße müssen zur Regel passen. Für **Einschalten oberhalb**
liegt die Einschaltschwelle über der Ausschaltschwelle, etwa 30 °C ein und 28 °C aus.
Dazwischen wird der vorherige Zustand gehalten. Diese Hysterese verhindert
ständiges Umschalten nahe einer einzigen Grenze.

Die Frischegrenze bestimmt, wie alt ein Eingabewert sein darf. Sie muss zum
Messintervall passen. Fehlende, ungültige oder alte Messungen werden nicht
als normale Nullwerte beziehungsweise automatische Aus-Anweisung interpretiert.
Entscheidung und Diagnose prüfen und das Verhalten bei Sensorausfall ausdrücklich testen.

## Selector verstehen

Die Betriebsart kommt aus einem Enum-Value. Im Automatikmodus wird die konfigurierte
automatische Entscheidung verwendet; feste Modi wählen Ein oder Aus.
Ein unbekannter Eingang hält den Ausgang gemäß dem aktuellen Selector-Verhalten.
Die passenden Enum-Codes in der Oberfläche auswählen, nicht beliebige Texte als
vermeintlich gleichwertige Betriebsarten eintragen.

[Praktischer Einstieg mit Blink](example-relay.md)
