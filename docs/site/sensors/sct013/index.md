# SCT013 und SCT013Head

!!! note "Entwicklungsstand / WIP"
    Der SCT013Head ist eine passive Aufbereitung für einen Stromwandler mit
    Spannungsausgang. Ein SCT013-Treiber ist in der gemeinsamen Firmware noch
    nicht registriert. Die [Aufbauanleitung für Rev. 0.1](../sct013-head/assembly.md) beschreibt Bestückung und Erstprüfung; die Kalibrierung am realen Aufbau steht noch aus.

## Aufgabe und unterstützter Sensortyp

Der Head nimmt das Wechselspannungssignal eines SCT013 mit integrierter Bürde
und Spannungsausgang auf und verschiebt es in den positiven Messbereich des ESP32-ADC.
Die genaue SCT013-Ausführung, Nennstrom und Ausgangsspannung müssen zum Aufbau passen.
SCT013-Varianten mit reinem Stromausgang sind durch diesen Entwurf nicht abgedeckt.

Die kompakte Platine wird von einer verschraubten Lumberg-1503-09-Buchse getragen.
Das Pigtail zum Mainboard benötigt eine eigene Zugentlastung.

## Anschluss zum Mainboard

| Pin | Funktion |
| --- | --- |
| 1 | GND |
| 2 | Nicht verwendet; die angebotenen 5 V werden nicht genutzt |
| 3 | Aufbereitetes ADC-Signal |
| 4 | +3V3_SYS |

Das passt zur vierpoligen ADC-Portbelegung von Mini und Weather.
Verwende einen analogen Eingang und prüfe die tatsächliche Verdrahtung.

## Signalaufbereitung

Die aktuelle Entwurfsbeschreibung nennt R1 = R4 = 10 kΩ als Teiler relativ zur
Vorspannung VBIAS. R2/R3 bilden die Vorspannung aus der 3,3-V-Versorgung;
C2 stabilisiert sie. C1 = 10 nF liegt parallel zu R4.
Tip der Sensorbuchse führt über R1 zum ADC, Sleeve liegt an VBIAS.

Die Auslegung muss zum konkreten Spannungsausgang passen. Die Unterlagen enthalten
verschiedene Sensor-Ausgangspunkte; daraus ergibt sich keine universelle Stromkalibrierung.
Eine garantierte Überstromfestigkeit oder Schutzbeschaltung ist nicht nachgewiesen.

## Aufbau und Erstprüfung

Die [Bestückungs- und Bring-up-Anleitung Rev. 0.1](../sct013-head/assembly.md) enthält die aktuelle Artikelzuordnung aus dem Artikelpool, die Pigtail-Belegung und Prüfungen ohne Klinkenbuchse und Sensor.

R1-R4 werden einheitlich mit 10 kΩ (ENP-00018, vier Stück) bestückt. Der ältere Vermerk zu R1 = 1 kΩ ist für diesen Aufbau überholt. Die reale elektrische Erprobung, Kalibrierung sowie die Passprobe von Buchse, Gehäusewand und Zugentlastung bleiben offen.
