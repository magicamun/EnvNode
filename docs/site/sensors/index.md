# I²C-Sensoren

Normale I²C-Sensoren können direkt an den I²C-Anschlüssen des Mainboards betrieben werden.
Auf EnvNode Mini stehen I2C0 und I2C1 mit jeweils zwei vierpoligen JST-SH-Anschlüssen bereit.

## Anschluss vorbereiten

1. Prüfe Versorgungsspannung und Logikpegel des Sensorboards. Die Mini-Anschlüsse
   führen GND, 3,3 V, SDA und SCL.
2. Prüfe die Pinbelegung des Kabels und beider Stecker. Ein passender Steckertyp allein
   garantiert keine passende Verdrahtung.
3. Ermittle den verwendeten Bus und die Sensoradresse. Auf I2C0 sind `0x50` für die
   Board Identity sowie `0x52` und `0x53` für optionale Module Identity vorgesehen.
4. Prüfe vorhandene Pull-ups auf allen beteiligten Boards. Die Mini-Pull-ups werden
   über normalerweise offene Lötjumper zugeschaltet.
5. Verbinde den Sensor bei ausgeschalteter Versorgung.

## Firmware

Die Firmware enthält unter anderem physische Implementierungen für SHT4x und SHTC3.
Konfiguriere einen Sensorplatz mit der passenden Implementierung, dem Bus und der Adresse.
Prüfe anschließend Diagnose und Messwerte, bevor du die MQTT-Integration einrichtest.

Ein beliebiger I²C-Sensor ist nicht automatisch unterstützt. Ein konkreter Sensor muss
zur verfügbaren Firmware-Implementierung passen. Ein durchgehend geprüfter Beispielablauf
mit einem ausgewählten Sensor wird als nächster Schritt ergänzt.

[Anschlüsse des EnvNode Mini](../mainboards/mini/index.md#i2c-anschlusse)
