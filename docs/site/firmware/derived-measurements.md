<!-- SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0 -->
<!-- See the repository root LICENSE. -->
# Abgeleitete Messwerte

EnvNode stellt abgeleitete physikalische Größen als separate Messkanäle bereit.
Ihre MQTT-Nutzdaten enthalten `derived: true`. Die Rohdatenkanäle bleiben verfügbar.
Diese Berechnungen führen keine Vorhersagen, Aggregationen oder Automatisierungen durch.

## Taupunkt und absolute Luftfeuchtigkeit

Instanzen von AM2302, BME280, SHT4x und SHTC3 stellen `dew_point` (°C oder die
konfigurierte Temperatureinheit für die Anzeige) und `absolute_humidity` (g/m³) bereit.
Sie verwenden dieselbe Sensor-ID, denselben Erfassungszeitplan und denselben Zeitstempel
wie der zugrunde liegende Sensor. Ein zusätzlicher Sensorplatz oder eine
sensorübergreifende Quellenauswahl ist nicht erforderlich.

Der Dekorator `HumidityDerivedSensor` kombiniert Temperatur und relative Luftfeuchtigkeit
aus derselben umschlossenen Sensorinstanz und demselben Aufruf von `sample()` oder `service()`.
Fehlende Eingangswerte ergänzt er niemals aus dem Snapshot-Cache oder einer anderen Instanz.
Eine unvollständige Erfassung erzeugt ungültige abgeleitete Werte; findet keine Erfassung
statt, entstehen keine zusätzlichen Nachrichten. Separate simulierte Temperatur- und
Luftfeuchtigkeitsinstanzen erzeugen diese Kanäle daher nicht.

Für die Temperatur T in °C und die relative Luftfeuchtigkeit H in Prozent gilt:

- γ = ln(H / 100) + 17.62 T / (243.12 + T)
- Taupunkt = 243.12 γ / (17.62 − γ)
- Dampfdruck e = (H / 100) × 6.112 × exp(17.62 T / (243.12 + T)), in hPa
- Absolute Luftfeuchtigkeit = 216.7 e / (273.15 + T), in g/m³

Dies sind Magnus-Näherungen über flüssigem Wasser (einschließlich unterkühltem Wasser),
keine Frostpunktberechnungen. Die Eingangswerte sind auf −45…60 °C und 0…100 % relative
Luftfeuchtigkeit begrenzt. Bei 0 % relativer Luftfeuchtigkeit ist die absolute
Luftfeuchtigkeit null und der Taupunkt ungültig, sodass kein unendlicher Zahlenwert
veröffentlicht wird. Fehlende, ungültige oder außerhalb des zulässigen Bereichs liegende
Eingangswerte führen zu `valid: false` ohne `value`. Gültige Ergebnisse erhalten die
Qualität `good`, wenn beide Eingangswerte diese Qualität haben; andernfalls übernehmen
sie die schlechtere Eingangsqualität (`estimated`, `below_measurement_range` oder
`degraded`). Die Ableitung allein verringert die Qualität nicht.

Quellen: [Sensirion, Luftfeuchtigkeit im Überblick](https://sensirion.com/resource/application_note/sht/glance),
[Vaisala, Taupunktberechnung](https://docs.vaisala.com/r/M212417EN-L/en-US/GUID-EBE2B115-3C98-4C8C-9F2D-A2FF1EFECFCC).

## Sonnenstand

Wähle **Solar Position** (`solar_position`) auf einem freien Sensorplatz. Die Quelle
benötigt weder GPIO- noch I²C-Ressourcen. Ihr Standardintervall beträgt 60 Sekunden und
lässt sich wie bei anderen periodischen Sensoren konfigurieren. Sie stellt folgende Werte bereit:

- `solar_azimuth`: Winkel in Grad im Uhrzeigersinn ab geografisch Nord: Nord 0°,
  Ost 90°, Süd 180°, West 270°; Wertebereich [0, 360).
- `solar_elevation`: Winkel in Grad über dem geometrischen Horizont; nachts negativ.

Beide Werte verwenden den konfigurierten Breiten- und Längengrad sowie die synchronisierte
UTC-Zeit, unabhängig von der eingestellten Anzeigezeitzone und der Sommerzeit. Eine
Höhenangabe ist nicht erforderlich. Die Quelle wird berechnet und ist nicht simuliert.
Wenn Standort und synchronisierte UTC-Zeit gültig sind, erhalten die Ergebnisse
`derived: true`, `simulated: false` und `quality: good`. Sind diese Voraussetzungen
nicht erfüllt, erhalten sie `quality: degraded`.

Die Berechnung des Sonnenmittelpunkts verwendet die
[angenäherten Sonnenkoordinaten des USNO](https://aa.usno.navy.mil/faq/sun_approx)
und die [Transformation in Horizontkoordinaten](https://aa.usno.navy.mil/faq/alt_az).
Atmosphärische Refraktion, Gelände, Horizontabsenkung und Beobachterhöhe werden nicht
berücksichtigt. Die Implementierung akzeptiert die Jahre 1800–2199, soweit diese mit
`time_t` auf der jeweiligen Plattform darstellbar sind. Ein fehlender oder ungültiger
Standort oder ein nicht definierter Azimut im Zenit oder Nadir macht das Wertepaar ungültig.
Vor der Zeitsynchronisierung verwirft der SensorManager wie bei physischen Sensoren
sämtliche Messwerte. Sobald Standort und Zeit gültig sind, liefert die Quelle wieder
gültige Werte, ohne dass sie zuvor deaktiviert werden muss.

## Integration und Grenzen

Alle Kanäle verwenden die bestehende Verarbeitungskette SensorManager → Snapshot-Cache → MQTT.
Home Assistant Discovery berücksichtigt die neuen Kanäle und bildet ungültige abgeleitete
Werte über das Werte-Template auf einen nicht verfügbaren numerischen Zustand ab.
Eigenschaften, Anzeigequellen und die Quellenauswahl für Schwellwerte verwenden ihre
üblichen stabilen Schlüssel. Der Taupunkt verwendet die konfigurierte Temperatureinheit
für die Anzeige.

Die Metadaten der Sensorimplementierungen erlauben nun sechs Kanäle (der BME280 nutzt alle sechs).
Discovery verwendet 32-Bit-Masken; eine Prüfung beim Kompilieren gleicht diese mit der
Anzahl der Messwerttypen ab. Bestehende Messwert-IDs und Einheiten-IDs bleiben unverändert;
die vier neuen Messwerttypen und zwei neuen Einheiten werden angehängt.
