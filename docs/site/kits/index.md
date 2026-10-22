# Bausätze und Aufbau

Für den ersten Aufbau beginnt die Dokumentation mit [EnvNode Mini](../mainboards/mini/index.md).
Die revisionsbezogene Bestückungs- und Bring-up-Anleitung enthält die detaillierten
Montage- und Prüfabläufe.

## Sinnvolle Reihenfolge

1. Platinenrevision und dazugehörige Anleitung abgleichen.
2. Mainboard und Versorgungsbaustein nach Anleitung bestücken und prüfen.
3. Versorgung ohne Sensoren und Erweiterungsmodule kontrollieren.
4. Firmware und Boardprofil prüfen.
5. Einen unterstützten I²C-Sensor anschließen und eine erste Messung prüfen.
6. MQTT einrichten; weitere Module einzeln ergänzen.

## Anschlusssets

Die [externen Anschlusssets](anschlusssets.md) beschreiben die Komponenten und die
einheitliche 1:1-Verdrahtung für JST-PH, JST-SH und die 5-V-Versorgung.

Artikelstammdaten, Beschaffung, Bestände und kaufmännische Bausatzstücklisten werden
in einer intern genutzten Lösung gepflegt. Die öffentliche Dokumentation beschreibt
revisionsbezogen den Aufbau, die Schnittstellen und die notwendigen Prüfungen.


## Aufbauanleitungen als Web und PDF (Pilot)

Die [zentrale Übersicht der Aufbauanleitungen](../guides/index.md) enthält die aktuellen Pilotfassungen für Mainboards, Versorgung, Module und Regensensor-Baugruppen. Beide Formate verwenden dieselbe Markdown-Quelle.
