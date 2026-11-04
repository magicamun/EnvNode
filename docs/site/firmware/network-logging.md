# Netzwerklogging über UDP

Lizenz: siehe [Lizenz und Nutzung](../license.md).

Der klassische ESP32-Teststand sendet Systemlogs und strukturierte Diagnosen
an rsyslog. Das bestehende Framework bleibt erhalten. RainDetector erfasst
weiterhin jedes Messfenster, normalerweise alle 250 ms. Die spätere Integration
von Analog und SCT013 hängt von gemessenen Ressourcenbudgets ab. SD/SX1262 und
ein Frameworkwechsel gehören zum späteren S3-Profil; der Anwendungskern bleibt
gemeinsam.

Unter **Netzwerklogging** die numerische Empfängeradresse, UDP-Port und die
Freigaben speichern. Für den vereinbarten Testaufbau: `192.168.20.10`, Port `514`
(Hostname des Servers: `iot-rsyslogd`). Hostnamen werden in der Firmware zunächst
nicht aufgelöst. Die Empfängeradresse ist installationsspezifisch und wird nicht
fest in die Firmware eingebaut.

Systemlogs und erweiterte Systemdiagnose sind getrennt wählbar. Die vorhandenen
Diagnose-Checkboxen beim Sensor, Aktor und Controller bleiben erhalten. Ein
Netzausfall setzt sie nicht zurück. Für RainDetector beide relevanten Quellen
aktivieren; die Systemdiagnose ist keine Voraussetzung dafür.

UDP bestätigt keinen Empfang. Pufferüberlauf, Netzwerkausfall und Serverausfall
können Lücken verursachen. Es gibt keine Nachlieferung aus persistentem Speicher.
Die Statusseite zeigt lokale Sende-/Verlustzähler, Queuefüllstände, Heapwerte und
maximale Anwendungs-/Logger-Servicezeiten. `sent_local` bedeutet ausschließlich,
dass der lokale Netzwerkstack das Datagramm angenommen hat. Ein nicht laufender
Empfänger kann daher trotz steigender sent_local-Zähler keine Daten speichern.

Gerätekennung, Bootkennung, Firmwarebuild, Streamsequenz, Uptime und optionale
UTC-Zeit ordnen Ereignisse zu. Sensorrevision und Gatezeiten bleiben in den
Rain-Ereignissen erhalten. Sequenzen und Zähler laufen bei 32 Bit über; Vergleiche
erfolgen modular. UDP kann außerdem umsortieren oder duplizieren.

Empfänger- und Rotationsbeispiele liegen im Repository unter `tools/rsyslog/`.
Systemlogs und Diagnosen werden separat gespeichert; die konkrete Aufbewahrung
wird dort verwaltet. Der aktuelle Stand und noch offene Hardwareprüfungen stehen
in `docs/UdpLoggingValidation.md`. Die lokale Validierung ersetzt keinen
Dauertest auf Weather 0.2 unter gleichzeitiger Web-/WLAN-/MQTT-Last.
