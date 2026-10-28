# Dein erstes EnvNode

Du möchtest zum Beispiel Temperatur, Luftfeuchte und Luftdruck an einem Ort messen
und die Werte später in deiner Hausautomation verwenden. EnvNode verbindet dafür
ein Mainboard mit den passenden Sensoren. Die Software auf dem Gerät liest die
Sensoren aus; du richtest sie über deinen Browser ein.

**Das erste Ziel ist ein echter, aktueller Messwert in der Weboberfläche.**
Anschließend kannst du die Messwerte an andere Systeme weitergeben oder den Aufbau
erweitern. Für diesen Einstieg verwenden wir ein Mini-Mainboard mit einem BME280.
Der Sensor liefert Temperatur, relative Luftfeuchte und örtlichen Luftdruck.

[So arbeitet EnvNode zusammen](../concepts/index.md){ .md-button }

## Was ist EnvNode eigentlich?

EnvNode ist eine modulare Plattform zum Selbstaufbau. Du wählst ein Mainboard und
ergänzt die Sensoren, Anzeigen oder Schaltausgänge, die du für deine Anwendung
brauchst. Bereits ein Mainboard mit einem einzelnen Sensor ist ein sinnvoller Aufbau;
eine vollständige Wetterstation ist dafür keine Voraussetzung.

Drei Begriffe begegnen dir dabei immer wieder:

| Begriff | Was bedeutet das für deinen Aufbau? |
| --- | --- |
| Hardware | Die bestückten Platinen, der ESP32, Sensoren, Kabel und Versorgung. |
| Firmware | Die Software, die auf dem ESP32 läuft, Messungen erfasst und die Weboberfläche bereitstellt. |
| Konfiguration | Deine Einstellungen: etwa WLAN, verwendeter Sensor, dessen Anschluss und später MQTT. |

Die Weboberfläche wird vom EnvNode selbst bereitgestellt. Auf deinem Rechner oder
Telefon öffnest du dafür einen Browser. Für die erste lokale Messung brauchst du
weder Home Assistant noch einen MQTT-Broker.

## Was brauchst du für das erste Beispiel?

| Bestandteil | Aufgabe / Auswahl |
| --- | --- |
| EnvNode Mini | Das Mainboard verbindet ESP32, Versorgung und Sensoranschlüsse. |
| Passendes ESP32-DevKit | Führt die Firmware aus; Pinbelegung und Bauform müssen zum Mainboard passen. |
| ORing Power | Wird auf dem Mainboard montiert und stellt dessen Versorgung bereit. |
| BME280-Sensorboard | Eine zu den 3,3-V-I²C-Anschlüssen passende Ausführung; Adresse 0x76 oder 0x77. |
| Passendes Sensorkabel | Verbindet GND, 3,3 V, SDA und SCL korrekt mit dem Mainboard. |
| Geeignete 5-V-Versorgung | Versorgt den geprüften Aufbau über einen vorgesehenen Eingang. |
| Rechner oder Telefon mit Browser und WLAN | Dient zur Einrichtung und Anzeige der Messwerte. |
| USB-Datenverbindung und Rechner zum Aufspielen | Erforderlich, wenn auf dem ESP32 noch keine passende Firmware installiert ist. |

Beim Selbstbestücken kommen die Bauteile und Werkzeuge aus den jeweiligen
Aufbauanleitungen hinzu. Ein bereits bestücktes Board wird anhand derselben
Anleitung geprüft. Ein Display und ein Erweiterungsmodul sind für dieses Beispiel
nicht erforderlich: Der BME280 wird direkt an einen I²C-Anschluss angeschlossen.

Prüfe vor der Auswahl die [Kompatibilitätsübersicht](../compatibility.md).
Sie unterscheidet dokumentierte Hardware, verfügbare Treiber und bestätigte Aufbauten.
Der dokumentierte BME280-Beispielbetrieb wurde mit Mini 0.3 aufgenommen; beim eigenen
Aufbau sind die tatsächliche Platinenrevision und das zugehörige Boardprofil maßgeblich.

## 1. Hardware aufbauen und prüfen

Beginne mit der [Mini-Aufbauanleitung](../mainboards/mini/index.md) für deine
Platinenrevision. Sie verweist auf die Versorgung mit
[ORing Power](../mainboards/oring-power/index.md) und führt durch Bestückung,
Orientierung und elektrische Prüfungen.

Arbeite zunächst ohne zusätzliche Module und Sensoren. Die Versorgung wird nach der
Aufbauanleitung geprüft, bevor du den Aufbau weiter ergänzt.

**Du kannst weitergehen, wenn:** der Aufbau die beschriebenen Prüfungen bestanden
hat und die Versorgung korrekt arbeitet.

## 2. Firmware bereitstellen und das Gerät erreichen

Ist noch keine passende Firmware installiert, folge
[Bauen und Aufspielen](../firmware/installation.md). Zum Programmieren verwendest du
den USB-Datenanschluss des ESP32-DevKit. USB-C an ORing Power dient der Stromversorgung.

Ein noch nicht eingerichtetes Gerät stellt ein WLAN mit dem Namen
`<Hostname>-Setup` bereit. Verbinde dich damit und öffne **http://192.168.4.1/**.
Richte anschließend dein WLAN ein. Danach erreichst du EnvNode unter der dort
zugewiesenen IP-Adresse; dein Rechner oder Telefon muss das Gerät in diesem Netz
auch erreichen können.

Die [erste Inbetriebnahme](../firmware/configuration.md) beschreibt Netzwerk,
Geräteeinstellungen und den Wechsel vom Setup-WLAN ins eigene WLAN im Detail.

**Du kannst weitergehen, wenn:** du die Weboberfläche deines Geräts öffnen kannst.

## 3. Dem Gerät seine Hardware zuordnen

Öffne **Hardware** und prüfe, welches Mainboard und welches Boardprofil angezeigt
werden. Das Profil legt fest, welche Anschlüsse die Firmware für deinen Aufbau
verwenden darf.

Die Hardwareidentität ist im **EEPROM**, einem kleinen Speicher auf der Platine,
hinterlegt. Ist dieser noch leer, wird die Identität einmalig passend zum Board
geschrieben. Diesen Vorgang nennt die Dokumentation **Provisioning**. WLAN- und
Sensoreinstellungen sind davon getrennt.

Folge dafür [Hardware und Provisioning](../firmware/provisioning.md).
Eine bereits gültige Identität wird zuerst geprüft; sie muss nicht bei jedem Start
neu geschrieben werden.

**Du kannst weitergehen, wenn:** das aktive Profil zu deinem realen Mainboard passt.
Die Hardwareerkennung richtet den BME280 noch nicht automatisch ein.

## 4. Den BME280 anschließen

Schalte die Versorgung aus. Für dieses Beispiel verwendest du einen Anschluss von
**I2C1** am Mini. I²C ist die gemeinsame Verbindung für digitale Sensoren; SDA und SCL
sind die beiden Signalleitungen neben Versorgung und Masse.

Prüfe am konkreten Sensorboard und Kabel die vier Verbindungen **GND, 3,3 V, SDA und
SCL** sowie die Pull-ups des Busses. Die Details stehen unter
[I²C-Sensoren anschließen](../sensors/i2c.md) und bei den
[Mini-I²C-Anschlüssen](../mainboards/mini/index.md#41-i2c-qwiic).
Ein passender Stecker allein bestätigt noch keine passende Kabelbelegung.

Schalte wieder ein und führe unter **Diagnose** einen Scan von I2C1 aus.
Der BME280 sollte dort auf **0x76 oder 0x77** antworten. Verwende anschließend genau
die Adresse deines Sensorboards.

**Du kannst weitergehen, wenn:** der angeschlossene BME280 auf dem gewählten Bus
erreichbar ist. Der Scan bestätigt die Verbindung; die Messung wird im nächsten
Schritt eingerichtet.

## 5. Die erste Messung einrichten

Öffne **Sensoren (Sensors)**, wähle einen freien Sensorplatz und konfiguriere:

| Einstellung | Auswahl für dieses Beispiel |
| --- | --- |
| Name | Zum Beispiel „Umgebung“ |
| Implementierung | BME280 |
| Bus | I2C1 |
| Adresse | Die ermittelte 0x76 oder 0x77 |
| Aktivierung | Sensor aktivieren |

Speichere die Konfiguration und wende ausstehende Sensoränderungen an, wenn die
Oberfläche das verlangt. Öffne anschließend **Messwerte (Measurements)** und warte
auf die erste Messung.

Du solltest Temperatur, relative Luftfeuchte und örtlichen Luftdruck unter deinem
gewählten Sensornamen sehen. Prüfe auch Einheit, Gültigkeit und Aktualität.
Ein ausdrücklich als Simulation eingerichteter Sensor ist keine Messung des BME280.

Die [Sensoranleitung](../firmware/sensors.md) zeigt Konfiguration und Messwertansicht
auch anhand des bestehenden BME280-Aufbaus.

**Das erste Ziel ist erreicht, wenn:** aktuelle, gültige Messwerte deines physischen
Sensors erscheinen und weiter aktualisiert werden.

## 6. Den funktionierenden Aufbau sichern

Starte das Gerät neu und kontrolliere, ob es wieder erreichbar ist und die Messung
erneut startet. Lade danach eine
[Konfigurationssicherung](../firmware/maintenance.md#konfiguration-sichern-und-wiederherstellen)
herunter. So hältst du den funktionierenden Ausgangszustand fest.

## 7. Messwerte weitergeben oder den Aufbau erweitern

Für die Weitergabe nutzt EnvNode **MQTT**: Das Gerät sendet Nachrichten an einen
**Broker**, der sie an interessierte Anwendungen verteilt. Home Assistant kann
beispielsweise diese Nachrichten empfangen und Messwerte anzeigen. Der Broker läuft
außerhalb von EnvNode und muss erreichbar sein.

Richte die Weitergabe erst mit einer funktionierenden lokalen Messung ein:
[MQTT und Home Assistant](../firmware/mqtt.md).

Weitere mögliche Schritte sind ein [Display](../displays/index.md), ein weiterer
[Sensor](../sensors/index.md) oder ein [DuoRelay-Modul](../modules/duo-relay/index.md).
Die [Gesamtübersicht](../concepts/index.md) erklärt, welche Rolle diese Erweiterungen
im System übernehmen.
