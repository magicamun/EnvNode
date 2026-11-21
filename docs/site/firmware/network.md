# Netzwerk einrichten

Unter **Netzwerk** legst du fest, mit welchem WLAN sich EnvNode verbindet und
wie es seine IPv4-Adresse erhält. WLAN-Hostname und [Gerätename](device.md)
sind getrennte Einstellungen.

Für den ersten Zugriff über das Setup-WLAN folge
[Erste Verbindung](configuration.md#2-weboberflache-offnen).

## Felder der Netzwerkseite

| Feld | Bedeutung |
| --- | --- |
| Hostname | Netzwerkname des EnvNode: bis zu 63 Buchstaben, Ziffern oder Bindestriche; kein Bindestrich am Anfang oder Ende. Im eigenen Netz eindeutig wählen. |
| WLAN-SSID | Name des WLANs, mit dem sich das Gerät verbinden soll. |
| WLAN-Passwort ändern | Nur mit dieser Auswahl wird das gespeicherte Passwort ersetzt. |
| Neues WLAN-Passwort | Neues Passwort für das gewählte WLAN; wird bei gesetzter Änderungsoption übernommen. |
| Adressmodus | DHCP oder statische IPv4-Konfiguration. |

Das gespeicherte Passwort wird nicht als Klartext im Formular angezeigt. Ein
leeres Passwortfeld allein löscht es nicht. Für eine neue WLAN-Verbindung mit
Passwort die Änderungsoption ausdrücklich setzen und das passende Passwort eingeben.

## DHCP oder feste Adresse

Mit **DHCP** bezieht EnvNode die Netzparameter vom Router. Die angezeigten
aktuellen Adressen sind schreibgeschützt und dienen der Kontrolle. Für den
Einstieg ist dies die einfachste Auswahl. Eine feste Zuordnung im DHCP-Server
kann dem Gerät ebenfalls eine wiederkehrende Adresse geben.

Bei **Statische IPv4** trägst du die Netzparameter selbst ein:

| Feld | Auswahl |
| --- | --- |
| IPv4-Adresse | Eine für dieses Gerät vorgesehene freie Adresse im eigenen Netz. |
| Subnetzmaske | Muss zum lokalen Netz passen. |
| Standardgateway | Router für Verbindungen außerhalb des lokalen Netzes. |
| Primärer DNS-Server | Namensauflösung, etwa für Broker und Zeitserver. |
| Sekundärer DNS-Server | Optionaler weiterer DNS-Server. |

Eine formal gültige Konfiguration belegt noch keine erreichbare Verbindung.
Falsche Adresse, Netzmaske oder DNS-Angaben können einzelne Dienste oder den
Browserzugriff verhindern.

## Speichern und Verbindung prüfen

1. Hostname, SSID und bei Bedarf das neue Passwort eintragen.
2. Adressmodus und dazugehörige Parameter prüfen.
3. **Netzwerkeinstellungen speichern** und die Rückmeldung zur erforderlichen
   Laufzeitaktion beachten. Bei Netzwerkeinstellungen ist ein Geräteneustart erforderlich.
4. Nach dem Neustart Rechner oder Telefon wieder mit dem vorgesehenen Netz verbinden.
5. Die aktuelle EnvNode-Adresse im Router oder in der seriellen Ausgabe ermitteln
   und die Weboberfläche darüber öffnen.
6. Auf **Status** Verbindung, Hostname, Adressmodus und tatsächlich zugewiesene
   IPv4-Adresse kontrollieren.

Eine bestehende Browserverbindung kann beim Wechsel abbrechen. Die gespeicherten
Parameter überstehen Neustart und Firmwareupdate. Ein Factory Reset entfernt sie.

## Wenn die Verbindung nicht gelingt

| Beobachtung | Erste Kontrolle |
| --- | --- |
| Setup-WLAN erscheint erneut | SSID, Passwort und Erreichbarkeit des vorgesehenen WLANs prüfen. Der Setup-Modus kann auch nach einem erfolglosen Verbindungsversuch mit gespeicherten Daten starten. |
| Alte Browseradresse antwortet nicht | Nach Neustart die tatsächlich aktive Adresse feststellen. |
| Lokale Oberfläche erreichbar, Broker oder Zeitserver nicht | DNS, Gateway und Erreichbarkeit des jeweiligen Dienstes prüfen. |

[Sprache & Zeit einrichten](language-time.md) · [MQTT](mqtt.md) ·
[Erste Einrichtung fortsetzen](configuration.md#4-gerat-sprache-und-zeit)
