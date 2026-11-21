# Einheiten wählen

Unter **Einheiten** wählst du die Darstellung unterstützter Messgrößen. Die
physische Erfassung und die Bedeutung der Messung bleiben erhalten: Ein Wechsel
von Pa nach hPa kalibriert den Sensor nicht neu.

## Auswahl der Darstellung

| Einstellung | Angebotene Einheiten |
| --- | --- |
| Frequenz | Hz oder kHz |
| Temperatur | °C oder °F |
| Luftdruck | Pa, hPa, kPa oder inHg |
| Solarzellentemperatur | °C oder °F |
| RainDetector-Level | Verhältniswert oder Prozent |

Eine angebotene Einheit ist kein Nachweis, dass ein entsprechender Sensor im
eigenen Gerät vorhanden ist. Die aktuelle RainDetector-Frequenzmessung und ihre
Benetzungserkennung sind insbesondere kein austauschbarer RainDetector-Level.
Nicht separat angebotene Größen behalten ihre festgelegte Einheit, etwa relative
Luftfeuchte in Prozent, Regenwippen-Zuwachs in mm und SCT013-Strom in Ampere.

## Wo sich die Auswahl auswirkt

| Bereich | Wirkung |
| --- | --- |
| Messwertseite | Zahlenwert und Einheit werden entsprechend dargestellt. |
| MQTT | Zukünftige Messwertnachrichten verwenden die gewählte Darstellungseinheit. |
| Home Assistant Discovery | Die Metadaten der betreffenden Messgröße berücksichtigen die Einheit; bei bestehender Integration anschließend kontrollieren. |
| Display und Vorschau | Sensorzahlen werden in der gewählten Anzeigeeinheit ausgegeben. Fester Einheitentext im Format muss von dir angepasst werden. |
| Controller | Schwellwerte bleiben in der im Controllerformular genannten kanonischen Einheit. Sie werden nicht automatisch umgerechnet. |

Beispiel zur Größenordnung: 100000 Pa entsprechen 1000 hPa. Ein
Druck-Schwellwert im Controller bleibt deshalb eine Eingabe in Pascal, auch
wenn Messwertseite und Display hPa zeigen. Frequenzgrenzen des RainDetectors
werden weiterhin in Hz eingegeben.

## Speichern und prüfen

1. Gewünschte Einheiten wählen und **Einheiten speichern**.
2. Eine neue Messung abwarten; vorhandene gespeicherte Historie wird nicht rückwirkend umgerechnet.
3. Messwertseite und neue MQTT-Nachrichten auf Zahlenwert und Einheit prüfen.
4. Bei Displays die festen Einheitentexte kontrollieren und bei Bedarf deren Seite speichern.
5. In externen Verbrauchern prüfen, ob Zahlenwert und Metadaten weiterhin zusammenpassen.

Die Einstellungen werden ohne Geräteneustart übernommen und bleiben nach Neustart
erhalten. Das Dezimaltrennzeichen bestimmt dagegen die
[Spracheinstellung](language-time.md), nicht die gewählte Einheit.
