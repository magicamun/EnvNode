# Beispiel: DuoRelay manuell und mit Blink

Ziel: Ein Relais zunächst manuell prüfen und anschließend periodisch schalten.
Beispielaufbau: DuoRelay in **Mini Slot A**, ohne Netzspannung und ohne reale Last.
Andere Slots benötigen die dort angegebenen GPIOs.

## Einrichtungsvariante wählen

Bei vorhandener Descriptor-basierter Modulzuordnung kann der Kanal als
**Module On/Off** eingerichtet sein. Dann definiert der Descriptor die Ressource;
die GPIOs werden nicht frei ausgewählt. Aktivierung und Name prüfen,
speichern und Änderungen anwenden. [Screenshot](actuators.md#descriptor-basierter-modulaktor).

Das folgende Beispiel beschreibt die freie GPIO-Variante für die genannte
Board-/Slotbelegung. Sie darf nicht ungeprüft auf Mini Revision 0.3 übertragen
oder zusätzlich auf derselben bereits belegten Ressource eingerichtet werden.

## Kanal 1 konfigurieren

1. Versorgung ausschalten, Modul korrekt einsetzen und nach Hardwareanleitung prüfen.
2. Unter Aktoren einen freien Platz als GPIO-On/Off einrichten.
3. Namen „Relais 1“, GPIO33 und aktiven HIGH-Pegel wählen.
4. Gewünschten sicheren Start-/Aus-Zustand prüfen, speichern und anwenden.
5. Manuell einschalten: Kanal-1-LED leuchtet und das Relais zieht an.
6. Ausschalten: LED aus, Relais fällt ab.
7. Kontakte spannungsfrei beziehungsweise mit geeigneten Testmitteln nach Anleitung prüfen.

Kanal 2 verwendet in Mini Slot A GPIO13. Die vollständige Zuordnung steht auf der
[DuoRelay-Seite](../modules/duo-relay/index.md). Eine Module Identity ersetzt diese Einrichtung nicht.

## Blink hinzufügen

1. Erst nach erfolgreichem manuellem Test einen Blink-Controller anlegen.
2. Den gerade eingerichteten Kanal-1-Aktor als Ziel wählen.
3. Zum Beispiel je 2000 ms Ein-Dauer und Aus-Dauer setzen.
4. Aktivierung und Parameter speichern, ausstehende Controlleränderungen anwenden.
5. Start ausführen und wiederholtes Ein/Aus beobachten.
6. Stop ausführen und den tatsächlich verbleibenden Aktorzustand prüfen.

Erwartung: Nur der gewählte Kanal wird periodisch angesteuert.
Kein anderer aktivierter Controller darf denselben Zielaktor beanspruchen.

## Abschluss

Für einen reinen Test den Controller dauerhaft deaktivieren, speichern und anwenden,
bevor der Aufbau unbeaufsichtigt neu startet. Das aktuelle STOP allein ersetzt
keine dauerhaft deaktivierte Konfiguration. Neustartverhalten kontrollieren.
