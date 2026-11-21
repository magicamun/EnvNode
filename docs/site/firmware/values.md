# Values: eigene Auswahlwerte

Ein **Value** hält eine benannte Auswahl, zum Beispiel eine Betriebsart. Er ist
keine Sensormessung. Die aktuelle Oberfläche unterstützt **Enum-Values**: einen
Wert aus einer fest definierten Optionsliste. Ein frei eingebbarer numerischer
Sollwert wird dadurch nicht bereitgestellt.

Ein Value schaltet selbst keinen Ausgang. Erst ein Verbraucher wie ein
[Selector](controllers.md#selector-verstehen) ordnet seiner Auswahl eine Wirkung
zu. Displays können den Wert unabhängig davon anzeigen.

## Definition und aktuellen Wert unterscheiden

Unter **Values** zeigen die Karten den aktuellen Wert und seine Neustartregel.
**Value hinzufügen (Add Value)** erstellt eine Definition. **Definition bearbeiten
(Edit definition)** ändert die erlaubten Optionen und die Neustartregel.
**Wert setzen (Set value)** wählt dagegen eine bereits definierte Option für den
laufenden Betrieb.

| Feld der Definition | Bedeutung |
| --- | --- |
| Name | Verständliche Bezeichnung des Values. |
| Optionsschlüssel / Option key | Stabiler Code für Zuordnungen und MQTT, etwa `auto`. Ein bis 32 Zeichen: Kleinbuchstaben `a`–`z`, Ziffern und Unterstrich. Innerhalb des Values eindeutig. |
| Anzeigetext / Display label | Lesbare Bedeutung, etwa „Automatik“. Dieser Text ist nicht der MQTT-Befehl. |
| Standardschlüssel / Default option key | Schlüssel einer vorhandenen Option; bestimmt den Standardwert. |
| Nach Neustart / After restart | Standard verwenden oder zuletzt gespeicherten Wert wiederherstellen. |

Es sind bis zu acht Values mit jeweils bis zu 16 Optionen möglich. Nicht verwendete
Optionszeilen vollständig leer lassen; jede verwendete Option benötigt Schlüssel
und Anzeigetext. Name und Anzeigetext dürfen jeweils bis zu 64 UTF-8-Bytes enthalten.

## Eine Definition speichern

1. Name und Optionen vollständig eingeben.
2. Einen der Optionsschlüssel als Standard eintragen.
3. Die gewünschte Neustartregel auswählen.
4. **Definition speichern** und die Rückmeldung prüfen.
5. Auf der Values-Karte den jetzt aktuellen Wert kontrollieren.

Die Definition wird sofort wirksam; ein separates Anwenden oder ein Neustart ist
nicht nötig. Dabei wird dieser Value neu initialisiert: mit dem Standardwert
beziehungsweise einem noch passenden gespeicherten Wert bei „Letzten Wert
wiederherstellen“. Das Bearbeiten einer Definition kann deshalb auch die aktuelle
Auswahl ändern. Ein bisher nur flüchtig gesetzter Wert muss nicht erhalten bleiben.

Ein aktivierter Selector schützt seine benötigten Optionen: Das Löschen des
verwendeten Values oder eine unpassende Änderung der Optionsliste wird abgewiesen.
Zuerst den Verbraucher deaktivieren beziehungsweise umkonfigurieren und dessen
Änderungen anwenden; anschließend Definition und Zuordnung gemeinsam prüfen.
Anzeigetexte können geändert werden, ohne die Bedeutung stabiler Schlüssel zu ändern.

## Einen Wert setzen

Auf der Karte eine Option wählen und **Wert setzen** betätigen. Die Auswahl wird
sofort wirksam; ein laufender Selector kann daraufhin seinen Ausgang verändern.
Die Karte zeigt anschließend die angenommene Auswahl. Bei einem Speicherfehler
bleibt der vorherige Zustand erhalten; die Fehlermeldung beachten.

## Verhalten nach Neustart

| Regel | Beim Setzen | Nach Neustart |
| --- | --- | --- |
| Standard verwenden | Auswahl gilt für den laufenden Betrieb | Der konfigurierte Standard wird gewählt |
| Letzten Wert wiederherstellen | Angenommene Auswahl wird dauerhaft gespeichert | Der gespeicherte gültige Schlüssel wird wiederhergestellt; ohne gespeicherten Wert gilt der Standard |

Die Definition selbst wird bei beiden Regeln dauerhaft gespeichert. Die Regel
betrifft die **aktuelle Auswahl**, nicht den Erhalt der Optionsliste.

**Prüfung:** Eine vom Standard abweichende Option setzen, neu starten und die
angezeigte Auswahl mit der gewählten Neustartregel vergleichen. Danach prüfen,
ob angeschlossene Verbraucher ebenfalls das erwartete Verhalten zeigen.

## Über MQTT bedienen

Auf der Karte **MQTT** aufklappen. Dort stehen das Befehlstopic, der konfigurierte
Standardschlüssel als Beispiel-Payload und das separate Statustopic.
Die Anzeige gehört zum aktuellen Quellstand; ältere Firmware-Images können diese
Bedienhilfe noch nicht enthalten.

| Richtung | Topic | Inhalt |
| --- | --- | --- |
| An EnvNode senden | `envnode/<device>/value/<ValueId>/cmd/state` | Optionsschlüssel als Klartext, etwa `auto` |
| Rückmeldung abonnieren | `envnode/<device>/value/<ValueId>/status/state` | Aktuell angenommener Optionsschlüssel |

Keine JSON-Klammern, Anführungszeichen, Anzeigetexte oder zusätzlichen Leerzeichen
mitsenden. Befehle **ohne Retain** senden. Der Status wird retained veröffentlicht
und nach MQTT-Wiederverbindung erneut bereitgestellt. Unbekannte Schlüssel werden
abgewiesen. Dieselbe Neustartregel gilt unabhängig davon, ob der Wert im Browser
oder über MQTT gesetzt wurde.

Ein gesendeter Befehl ist noch keine Rückmeldung: Statustopic und Values-Karte
kontrollieren. Der Value-Status bestätigt die Auswahl; ob ein Aktor sie umgesetzt
hat, steht im Controller- und Aktorstatus.

[Lokale Controller](controllers.md) · [Displayquellen](values-displays.md) ·
[MQTT einrichten](mqtt.md)

Lizenz: LicenseRef-EnvNode-Use-and-Redistribution-1.0; siehe [Lizenz](../license.md).
