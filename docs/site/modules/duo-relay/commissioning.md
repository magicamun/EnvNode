# DuoRelay in Betrieb nehmen

Hier richtest du ein **aufgebautes und elektrisch geprüftes DuoRelay** mit der
EnvNode-Firmware ein. Ziel sind zwei eindeutig zugeordnete Relaisaktoren, die du
manuell schalten kannst und deren Verhalten nach Neustart geprüft ist.

Wenn das Modul noch bestückt oder elektrisch geprüft werden muss, beginne mit
[Aufbau und elektrischer Prüfung](assembly.md). Wie du die funktionierenden Kanäle
anschließend verwendest, steht unter [Relais schalten](../../firmware/example-relay.md).

## 1. Voraussetzungen prüfen

- Mainboard und DuoRelay sind nach ihren Aufbauanleitungen geprüft.
- Die EnvNode-Weboberfläche ist erreichbar und das aktive Boardprofil passt zum
  tatsächlichen Mainboard. Die separate Test-/Provisioning-Firmware ist nicht
  die Anwendung zum Betreiben der Relais.
- Der verwendete physische Modulslot ist bekannt.
- Für die erste Einrichtung sind externe Lasten getrennt und lokale Controller,
  die diese Ausgänge verändern könnten, deaktiviert.

Module nur bei ausgeschalteter Versorgung einsetzen oder umstecken.
Die [allgemeine erste Inbetriebnahme](../../firmware/configuration.md) erklärt
Netzwerk und Gerätezugriff; die [Aktorbedienung](../../firmware/actuators.md)
erklärt die gemeinsamen Felder, Speichern und Anwenden der Änderungen.

## 2. Modulidentität prüfen

Öffne **Hardware** und kontrolliere, ob DuoRelay im tatsächlich verwendeten Slot
mit einem gültigen, kompatiblen Descriptor erkannt wird. Der Descriptor beschreibt
die Modulgeräte und deren Verbindung zu den Ressourcen des Mainboards.
Eine Antwort des EEPROMs beim I²C-Scan allein bestätigt diesen Inhalt nicht.

Ist der Datensatz leer oder unpassend, verwende den allgemeinen Ablauf
[Modulidentität schreiben](../../firmware/provisioning.md#modul-schreiben) mit der
Vorlage **DuoRelay 0.3** und dem richtigen Slot. Dort stehen Schreibbestätigung,
Rückleseprüfung und die Kontrolle nach dem Neustart. Ein bereits gültiger Datensatz
muss nicht erneut geschrieben werden.

**Ergebnis:** Die erkannte Modulidentität passt zu Modul und Slot.

## 3. Einrichtungsweg für die Relaiskanäle wählen

### Mit kompatiblem Modul-Descriptor

Die Firmware kann aus einem kompatiblen Descriptor mit aufgelösten, verfügbaren
Ausgängen die Relaisaktoren automatisch bereitstellen. Öffne **Aktoren (Actuators)**
und suche die beiden zum gewählten Modulslot gehörenden Kanäle, beispielsweise
`relay.1` und `relay.2`. Die Oberfläche bezeichnet diese Zuordnung als **Module On/Off**.

Prüfe für jeden Kanal Slot, Gerätezuordnung und Verfügbarkeit. Vergib eindeutige
Namen wie „Relais 1“ und „Relais 2“ und kontrolliere die Aktivierung. Speichere
geänderte Angaben und wende ausstehende Aktoränderungen an.
Die Ausgangsressource kommt bei diesem Weg aus der Modulzuordnung; ein freier GPIO
wird dafür nicht zusätzlich ausgewählt.

Fehlt ein Kanal, prüfe zuerst die angezeigte Modulzuordnung, Kompatibilität und
Ressourcenverfügbarkeit. Ein fehlender Kanal ist kein Anlass, ungeprüft einen
zweiten Aktor auf demselben Ausgang anzulegen.

### Mit ausdrücklich manueller GPIO-Zuordnung

Wenn du das Modul ohne nutzbare Descriptor-Zuordnung betreibst, richte je Kanal
einen **GPIO On/Off**-Aktor ein. Wähle den zum tatsächlichen Mainboard und Slot
gehörenden Ausgang: Kanal 1 verwendet **AUX_GPIO1**, Kanal 2 **AUX_GPIO2**.
Die zugehörigen GPIO-Nummern entnimmst du der Ressourcenbelegung des aktiven
Boardprofils beziehungsweise der Anleitung deines Mainboards.

DuoRelay wird **aktiv HIGH** angesteuert. Verwende aussagekräftige Namen,
aktiviere die benötigten Aktoren und speichere und übernimm die Konfiguration.
Eine Zuordnung aus einer anderen Mainboardrevision ist kein Ersatz für den Abgleich.

**Verwende je Relaisausgang genau einen Einrichtungsweg.** Ein automatisch
bereitgestellter Modulaktor und ein zusätzlicher freier GPIO-Aktor dürfen nicht
denselben Ausgang beanspruchen.

## 4. Kanalzuordnung manuell bestätigen

Prüfe ohne angeschlossene reale Last und ohne laufenden Controller:

| Aktion in der Weboberfläche | Erwartetes Ergebnis |
| --- | --- |
| Beide Relaisaktoren ausschalten. | Beide Relais sind abgefallen; beide Kanal-LEDs sind aus. |
| Nur „Relais 1“ einschalten. | Nur Kanal 1 zieht an; seine LED leuchtet. |
| „Relais 1“ wieder ausschalten. | Kanal 1 fällt ab. |
| Nur „Relais 2“ einschalten und wieder ausschalten. | Nur Kanal 2 folgt den Befehlen. |

Der Aktorzustand und die LED bestätigen keine mechanische Kontaktstellung.
Die Kontaktprüfung erfolgt mit dem Verfahren aus der
[elektrischen Erstprüfung](assembly.md#6-elektrische-erstprufung-und-funktionstest).
Dort steht auch die maßgebliche Klemmenbelegung; sie wird hier nicht wiederholt.

**Ergebnis:** Beide Namen sind dem richtigen physischen Kanal zugeordnet und
beeinflussen sich bei manueller Bedienung nicht.

## 5. Neustart und Konfiguration prüfen

Lass externe Lasten für diesen Test getrennt. Starte das Gerät neu und prüfe
beide Kanäle erneut. Die Relais dürfen ohne beabsichtigte Ansteuerung nicht anziehen.
Berücksichtige dabei gespeicherte Aktivierungen und eventuell bereits eingerichtete
Controller. Ein zuvor ausgeführtes Controller-STOP deaktiviert diesen nicht dauerhaft.

Wenn Zuordnung und Verhalten stimmen, erstelle eine
[Konfigurationssicherung](../../firmware/maintenance.md#konfiguration-sichern-und-wiederherstellen).

**Die Inbetriebnahme ist abgeschlossen, wenn** beide Kanäle richtig zugeordnet sind,
manuelles Ein/Aus funktioniert und das Verhalten nach Neustart dem vorgesehenen
Betrieb entspricht.

[Weiter: Relais bedienen und mit Blink betreiben](../../firmware/example-relay.md){ .md-button }
