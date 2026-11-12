# Relais schalten mit DuoRelay

Du möchtest einen Ausgang manuell ein- und ausschalten oder regelmäßig schalten
lassen. DuoRelay stellt dafür zwei Relaiskanäle bereit. Diese Seite beschreibt
die Anwendung mit **bereits eingerichteten und geprüften Relaisaktoren**.

[Relais zuerst einrichten und in Betrieb nehmen](../modules/duo-relay/commissioning.md){ .md-button }

## Welchen Betrieb brauchst du?

| Dein Ziel | Der passende Weg |
| --- | --- |
| Einen Kanal selbst ein- oder ausschalten. | Manuelle Bedienung unter **Aktoren**. |
| Einen Kanal regelmäßig ein- und ausschalten. | Ein lokaler **Blink**-Controller mit diesem Aktor als Ziel. |
| Abhängig von einem Messwert schalten. | Ein lokaler [Threshold-Controller](controllers.md#threshold-verstehen). |
| Von einem externen System aus schalten. | [MQTT-Befehle und zugehörigen Status](mqtt.md). |

Die Einrichtungsanleitung prüft die Zuordnung zum richtigen Relais.
Die [allgemeine Aktorbedienung](actuators.md#manuelle-bedienung-und-controller)
erklärt das Zusammenspiel von Handbedienung und laufenden Controllern.

## Manuell bedienen

Öffne **Aktoren (Actuators)** und schalte den eingerichteten Kanal über dessen
Ein-/Aus-Bedienung. Prüfe den angezeigten Zustand und die vorgesehene Wirkung.
Für eine eindeutige Handbedienung muss klar sein, ob ein lokaler Controller oder
ein externes System denselben Aktor ansteuert.

Eine leuchtende Kanal-LED zeigt die elektrische Ansteuerung an. Ob die angeschlossene
Anwendung tatsächlich wie vorgesehen arbeitet, prüfst du am eigenen Aufbau.
Verwende Last, Anschluss und Schutzbeschaltung innerhalb der in der
[Hardwareanleitung](../modules/duo-relay/assembly.md) beschriebenen Einsatzgrenzen.

## Beispiel: Einen Kanal mit Blink betreiben

Führe dieses Beispiel ohne reale Last aus. Der Relaisaktor wurde bereits nach der
[Inbetriebnahmeanleitung](../modules/duo-relay/commissioning.md) geprüft;
seine GPIO- oder Modulzuordnung wird hier nicht erneut eingerichtet.

1. Unter **Controller (Controllers)** einen freien Platz als **Blink** einrichten.
2. Den gewünschten, eindeutig benannten Relaisaktor als Ziel auswählen.
3. Zum Beispiel **2000 ms Ein-Dauer** und **2000 ms Aus-Dauer** einstellen.
4. Dauerhafte Aktivierung und Parameter speichern und ausstehende
   Controlleränderungen anwenden.
5. Den Controller starten und wiederholtes Ein/Aus beobachten.
6. Den Controller stoppen und den tatsächlich verbleibenden Aktorzustand prüfen.

**Erwartung:** Nur der ausgewählte Kanal schaltet periodisch. Ein weiterer
aktivierter Controller darf denselben Zielaktor nicht beanspruchen.
Die gemeinsamen Bedienregeln erklärt [Lokale Controller](controllers.md).

## Vom Test zum vorgesehenen Betrieb

Für einen reinen Versuch den Blink-Controller dauerhaft deaktivieren, speichern
und die Änderungen anwenden. Ein momentanes **STOP** allein verhindert keinen
späteren Start aufgrund gespeicherter Einstellungen. Prüfe das Verhalten nach
Neustart, bevor du den Aufbau im vorgesehenen Betrieb verwendest.

Für messwertabhängiges oder externes Schalten verwendest du denselben geprüften
Aktor weiter. Die jeweiligen Einstellungen stehen einmalig unter
[Threshold](controllers.md#threshold-verstehen) beziehungsweise [MQTT](mqtt.md).

[DuoRelay im Überblick](../modules/duo-relay/index.md)
