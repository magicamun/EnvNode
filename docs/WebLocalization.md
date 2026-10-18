<!-- SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0 -->
<!-- See the repository root LICENSE. -->
# Weboberfläche: Sprache und feste Texte

## Zu übersetzen

- Navigation, Überschriften, Beschriftungen, Schaltflächen und barrierefreie Namen.
- Hilfetexte, Validierungsfehler, Bestätigungsdialoge und Ergebnisnachrichten.
- Sichtbare Zustands-, Qualitäts- und Messgrößenbezeichnungen der Weboberfläche.
- Dynamisch erzeugte Beschriftungen und Fehlermeldungen in Browser-Skripten.

Diese Texte stehen zentral in `firmware/include/WebTextCatalog.inc` mit stabiler
TextId, englischem Fallback und deutscher Übersetzung. Die gespeicherte Locale
steuert die Darstellung; Deutsch verwendet `de-DE` (Eingabe auch `de_de`).

## Nicht zu übersetzen

- HTTP-Headernamen, HTTP-Routen, Feldnamen, DOM-IDs, CSS-Klassen, JSON-Schlüssel und Protokollwerte.
- MQTT-Topics, Payloads, Discovery-Daten und stabile Eigenschafts-/Enum-Codes.
- Benutzerdefinierte Geräte-, Sensor-, Aktor-, Controller- und Wertnamen sowie
  Enum-Beschriftungen und die vom Benutzer verfassten Display-Inhalte.
- Diagnoseprotokolle, Log-Level-Codes und vom Gerät gemeldete Rohfehler.
- Firmware-/Build-Kennungen, Hardware-/Treibernamen, UUIDs, Adressen, Busnamen,
  Zeitzonenwerte, Format-Platzhalter sowie standardisierte Einheitensymbole.

Eine übersetzte Beschriftung darf niemals den gespeicherten oder übertragenen
Wert verändern. Die Übersetzung gehört ausschließlich in die Präsentationsschicht.

HTTP-303-Weiterleitungen verwenden zentral `sendSeeOther` aus
`firmware/include/HttpRedirect.h`. Der Headername `Location`, der Statuscode und
das Weiterleitungsziel sind unabhängig von der eingestellten UI-Sprache.
