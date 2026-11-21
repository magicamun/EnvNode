# Netzwerklogging

Die Firmware kann Systemmeldungen und erweiterte Diagnosen an einen externen,
konfigurierten und laufenden **rsyslogd** senden. Dieser Dienst nimmt die
Meldungen auf einem anderen Rechner entgegen und speichert sie dort. Damit
lassen sich beispielsweise Sensorfehler oder der Ablauf einer Heizungssteuerung
über längere Zeit nachvollziehen.

Netzwerklogging ist optional. Die lokale Seite **Protokolle** bleibt davon
unabhängig verfügbar. Die Einrichtung des externen rsyslogd wird hier vorausgesetzt.

## Voraussetzungen

- Ein eingerichteter und laufender rsyslogd muss Nachrichten über UDP annehmen.
- Die IPv4-Adresse dieses Rechners und der dort konfigurierte UDP-Port müssen bekannt sein.
- EnvNode muss den Empfänger über das Netzwerk erreichen können; dessen Firewall muss den Empfang zulassen.

Ein MQTT-Broker ist kein Ersatz für diesen Empfänger. Die Firmware verwendet
für Netzwerklogging eine numerische IPv4-Adresse; ein Hostname ist hier nicht vorgesehen.

## Einstellungen auf EnvNode

Öffne **Netzwerklogging** in der Weboberfläche.

| Einstellung | Bedeutung |
| --- | --- |
| Netzwerklogging aktiv | Versand an den eingetragenen Empfänger einschalten. |
| Empfänger IPv4 | Adresse des Rechners, auf dem rsyslogd läuft. Die Adresse gehört zum eigenen Netz. |
| UDP-Port | Empfangsport des Dienstes, von 1 bis 65535. Häufig wird 514 verwendet; maßgeblich ist die tatsächliche Serverkonfiguration. |
| Systemlogs senden | Allgemeine Meldungen über Betrieb, Ereignisse und Fehler übertragen. |
| Erweiterte Systemdiagnose | Zusätzliche Diagnosen des Geräts übertragen, etwa zur Untersuchung von Ressourcen und Laufzeit. |

Adresse, Port und gewünschte Freigaben eintragen und **Speichern** wählen.
Die Einstellungen werden im laufenden Betrieb übernommen und bleiben nach
Neustart erhalten. Für eine erste Kontrolle genügt es, die benötigten
Meldungsarten gezielt einzuschalten.

## Einzelne Komponenten untersuchen

Zusätzliche Sensor-, Aktor- und Controllerdiagnosen werden in deren jeweiligen
Einstellungen über **Erweiterte Diagnose** ausgewählt und gespeichert. Die
Freigabe der erweiterten **System**diagnose schaltet diese Komponenten nicht
pauschal mit ein.

Für eine Heizungsprüfung beispielsweise Sensor, Heizaktor und zuständigen
Controller auswählen: Die Sensordiagnose zeigt die Eingabe, die Controllerdiagnose
die Entscheidung und die Aktordiagnose den angewendeten Software-Ausgang.
Letztere ist keine elektrische Rückmessung der Heizung.

Soweit die laufende Firmware **Testbeginn / Ende markieren** anbietet, können
zusammengehörige Versuche in der externen Aufzeichnung gekennzeichnet werden.
Damit lassen sich Ereignisse leichter einem Testabschnitt zuordnen.

## Empfang kontrollieren

1. Auf EnvNode Empfänger und Freigaben speichern.
2. Für die angebotene Funktion **Diagnoseereignis erzeugen** die erweiterte
   Systemdiagnose aktivieren und ein Testereignis erzeugen.
3. Auf dem Empfänger prüfen, ob die Meldung tatsächlich eingetroffen ist.
4. Anschließend einen bekannten Ablauf der ausgewählten Komponente beobachten
   und mit der Aufzeichnung vergleichen.

UDP bestätigt den Empfang nicht. Ein steigender lokaler Sendezähler bedeutet,
dass EnvNode die Nachricht an seinen Netzwerkstack übergeben hat. Er bestätigt
keine Speicherung auf dem Empfänger. Bei fehlenden Meldungen deshalb Adresse,
Port, Erreichbarkeit, Firewall und den laufenden Empfangsdienst prüfen.

Netzwerk- oder Serverausfälle und volle Puffer können Aufzeichnungslücken
verursachen. Fehlende Meldungen werden später nicht aus einem dauerhaften
Gerätespeicher nachgeliefert. Aufbewahrung und Löschen der Aufzeichnungen werden
auf dem externen Rechner festgelegt.

[Wartung und Diagnose](maintenance.md#diagnose) ·
[Sensoren](sensors.md) · [Aktoren](actuators.md) · [Controller](controllers.md)

Lizenz: siehe [Lizenz und Nutzung](../license.md).
