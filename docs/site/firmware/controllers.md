# Lokal automatisch steuern

Controller verbinden eine lokale Regel oder Zeitfunktion mit einem konfigurierten
Aktor. Der Zielaktor muss bereits funktionieren. Controller sind keine allgemeine
Wetterauswertung und ersetzen keine externen Langzeit-Automationen.

| Controller | Aufgabe |
| --- | --- |
| Blink | On/Off-Aktor mit Ein- und Aus-Dauer periodisch schalten |
| Threshold / Hysteresis | On/Off-Aktor abhängig von einem numerischen Messwert steuern |
| RainDetector | Frequenzsensor und Heizaktor für Benetzungserkennung, Trocknung und optionales Taupunktheizen verbinden |
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

## Gemeinsame Felder und Laufzeit

| Feld / Anzeige | Bedeutung |
| --- | --- |
| Aktiviert | Controller beim Start und nach dem Anwenden automatisch initialisieren. |
| Name | Zweck der Regel, unabhängig von der Bezeichnung des Zielaktors. |
| Implementierung | Algorithmus: Blink, Threshold, Selector oder RainDetector. |
| Quelle | Messung, Entscheidung oder Value, auf die sich die Regel bezieht. |
| Zielaktor | Ausgang mit passender Fähigkeit. Ein aktivierter steuernder Controller beansprucht dieses Ziel exklusiv. |
| Läuft / Running | Tatsächlicher Laufzeitzustand; nicht mit der gespeicherten Aktivierung gleichsetzen. |
| Entscheidung und Grund | Was die Regel anfordert und weshalb; „unbekannt“ ist keine Aus-Entscheidung. |
| Ziel verfügbar / Operationsergebnis | Ob die Anforderung überhaupt am Ausgang umgesetzt werden konnte. |

Die Zielauswahl berücksichtigt bereits beanspruchte Ausgänge. Ein benannter
Aktorplatz und derselbe automatisch erkannte Modulkanal sind kein Paar unabhängiger
Ausgänge; die Oberfläche führt diese Zuordnung zusammen.

Speichern und **Controlleränderungen anwenden** übernimmt die Konfiguration und
initialisiert aktivierte Controller neu. Bisherige Laufzeitentscheidungen werden
dabei nicht als Fortsetzung einer laufenden Regel garantiert. Prüfe danach Quelle,
Ziel, Entscheidung und Ausgang erneut.

## Blink einstellen

| Feld | Bedeutung |
| --- | --- |
| Zielaktor | On/Off-Ausgang, der periodisch schalten soll. |
| Ein-Dauer in ms | Dauer der Ein-Phase, mindestens 1 ms. |
| Aus-Dauer in ms | Dauer der Aus-Phase, mindestens 1 ms. |

Der Zyklus beginnt mit der Ein-Phase, sobald das Ziel verfügbar ist. Beide Zeiten
werden als Millisekunden eingegeben; 2000 ms sind zwei Sekunden. **Stop** beendet
den Zyklus und fordert Aus an. Für einen reinen Versuch anschließend auch die
gespeicherte Aktivierung ausschalten, damit der Zyklus nach Neustart nicht erneut beginnt.

[Kurzes Relaisbeispiel](example-relay.md#beispiel-einen-kanal-mit-blink-betreiben).

## Threshold verstehen

Threshold bildet aus einer numerischen Sensormessung eine Ein-/Aus-Entscheidung.

| Feld | Bedeutung |
| --- | --- |
| Quellsensor | Aktivierte Messquelle; nach Auswahl auch die richtige Messgröße wählen. |
| Messgröße | Numerische Zustandsgröße, zum Beispiel Temperatur oder Druck. Ein Kippereignis ist keine solche Zustandsgröße. |
| Nur Entscheidung / Decision only | Entscheidung bereitstellen, ohne selbst einen Aktor zu steuern; Voraussetzung für die Verwendung als Selector-Quelle. |
| Zielaktor | Bei direkter Steuerung der zu schaltende On/Off-Ausgang. |
| Schaltrichtung | Einschalten bei Erreichen oder Überschreiten beziehungsweise Unterschreiten der Einschaltschwelle. |
| Ein-/Ausschaltschwelle | Grenzen der Hysterese in der **im Formular genannten kanonischen Einheit**. Die Darstellungswahl unter Einheiten rechnet diese Eingaben nicht um. |
| Maximales Messwertalter in ms | Ältester noch verwendbarer Messwert. Größer als das normale Messintervall einschließlich erwartbarer Verzögerungen wählen. |

Beispielsweise arbeitet eine Druckschwelle mit Pascal, auch wenn die Messwertseite
hPa anzeigt. Bei einer Quellenänderung Einheit und beide Grenzen erneut prüfen.

Messquelle und Messgröße müssen zur Regel passen. Für **Einschalten oberhalb**
liegt die Einschaltschwelle über der Ausschaltschwelle, etwa 30 °C ein und 28 °C aus.
Dazwischen wird der vorherige Zustand gehalten. Diese Hysterese verhindert
ständiges Umschalten nahe einer einzigen Grenze.

Die Frischegrenze bestimmt, wie alt ein Eingabewert sein darf. Sie muss zum
Messintervall passen. Fehlende, ungültige oder alte Messungen werden nicht
als normale Nullwerte beziehungsweise automatische Aus-Anweisung interpretiert.
Entscheidung und Diagnose prüfen und das Verhalten bei Sensorausfall ausdrücklich testen.

### Hysterese und Startverhalten

Bei **Einschalten unterhalb** muss die Einschaltschwelle kleiner als die
Ausschaltschwelle sein, etwa bei 5 °C ein und bei 7 °C aus. Das Erreichen einer
Grenze zählt bereits als Schaltbedingung. Zwischen den Grenzen wird die bisherige
Entscheidung gehalten. Nach Start gibt es zunächst keine solche Entscheidung:
Liegt der erste Wert im Zwischenband, wartet Threshold auf eine erreichte Grenze.

Fehlende oder zu alte Eingaben erzeugen keine neue Schaltentscheidung. Ein bereits
geschalteter Ausgang wird dadurch **nicht automatisch ausgeschaltet**. Bei direkter
Steuerung können noch ausstehende Umsetzungsversuche einer vorherigen Entscheidung
fortgesetzt werden. Für die Fehlersuche Messwertalter, Entscheidungsgrund und
Aktorstatus zusammen betrachten. **Stop** fordert bei direkter Steuerung Aus an;
bei „Nur Entscheidung“ wird ausschließlich die Entscheidung beendet.

**Prüfung:** Beide Grenzen gezielt erreichen, den Zustand im Zwischenband prüfen
und anschließend fehlende beziehungsweise veraltete Eingaben und Neustart testen.
Ein plausibler einzelner Zahlenwert reicht als Funktionsnachweis nicht aus.

## Selector verstehen

Selector verbindet eine Betriebsart mit einer automatischen Entscheidung und
einem physischen Ausgang. Benötigt werden ein [Enum-Value](values.md), ein
aktivierter Threshold mit **Nur Entscheidung** und ein eigener On/Off-Zielaktor.
Der Threshold steuert hier keinen Ausgang; ausschließlich der Selector schreibt
das Ziel. Dadurch konkurrieren Automatik und Handmodus nicht als zwei Controller
um denselben Aktor.

| Feld | Bedeutung |
| --- | --- |
| Betriebsart-Value | Die Auswahl, die zwischen Automatik, Ein und Aus umschaltet. |
| Automatische Entscheidung | Aktivierter Threshold im Modus „Nur Entscheidung“. |
| Automatik bei | Optionsschlüssel des Values, der die Threshold-Entscheidung übernehmen soll. |
| Ausgang Ein bei | Davon verschiedener Optionsschlüssel für dauerhaft angefordertes Ein. |
| Ausgang Aus bei | Dritter Optionsschlüssel für angefordertes Aus. |
| Zielaktor | Ausgang, der tatsächlich geschaltet wird. |

Zuerst Value und Threshold einrichten, dann Selector und Optionszuordnung speichern
und Controlleränderungen anwenden. Die Zuordnung verwendet Optionsschlüssel,
nicht bloß ähnlich klingende Anzeigetexte.

Im Automatikmodus wird nur eine aktuelle, verwendbare Threshold-Entscheidung
übernommen. Bei unbekannter Entscheidung, unpassendem Modus oder fehlender Quelle
schreibt Selector keinen neuen Zustand; der Ausgang bleibt dadurch unverändert.
Eine weitere, nicht zugeordnete Option ist deshalb **kein** zusätzlicher Aus-Modus.
Die festen Ein-/Aus-Modi benötigen keine gültige automatische Messentscheidung.

**Stop** beendet den Selector, schaltet den Ausgang aber nicht selbst aus.
Für bewusstes Ausschalten bei laufendem Selector den zugeordneten Aus-Modus setzen
und den Aktorstatus prüfen. Nach Neustart wirken die gespeicherte
Controlleraktivierung und die Neustartregel des Betriebsart-Values zusammen.

**Prüfung:** Alle drei Modi wählen, die tatsächliche Ausgangswirkung kontrollieren
und anschließend den Automatikmodus bei unbekannter Quelle sowie das Verhalten
nach Neustart prüfen.


## Wirkung von Stop vergleichen

| Controller | Wirkung von Stop bei erreichbarem Ziel |
| --- | --- |
| Blink | Zyklus beenden und Ausgang ausschalten |
| Threshold, direkte Steuerung | Entscheidung beenden und Ausgang ausschalten |
| Threshold, nur Entscheidung | Entscheidung beenden; kein eigener Ausgang |
| Selector | Steuerung beenden; Ausgang unverändert lassen |
| RainDetector | Automatik entwaffnen und Heizung ausschalten |

Die Rückmeldung und den tatsächlichen Aktorzustand kontrollieren. Ein nicht
verfügbarer Ausgang kann einen Ausschaltauftrag nicht umsetzen. Stop ist bei
keiner dieser Varianten eine dauerhaft gespeicherte Deaktivierung.

## RainDetector einrichten

RainDetector verbindet einen passenden Frequenzsensor mit einem Heizaktor.
Die [baugruppenspezifische Inbetriebnahme](../commissioning/rain-detector.md)
ist die maßgebliche Anleitung für Quellen, Schwellen, Startprüfung und Abschaltung.
Sie erklärt auch die optionale Klimaquelle für vorbeugendes Heizen.
Die [Anwendungsseite](../applications/rain-detector.md) erläutert die Zustände.
