# Firmware-Downloads pflegen

Lizenz: siehe LICENSE im Repository.

Die Veröffentlichung läuft im Workflow `.github/workflows/docs.yml` auf `main`.
Beide produktiven Ziele werden gebaut; erst nach erfolgreichen Builds werden die
Downloads aus `firmware/bin/` veröffentlicht. Auch lokale Builds exportieren ihre
OTA-Images automatisch in diesen sichtbaren, von Git ignorierten Ordner. Native Tests und serielle Diagnosevarianten sind keine
Download-Ziele.

Bei jeder neuen Firmware-Version eine Datei `<ziel>/<version>.md` mit einer kurzen,
verständlichen Beschreibung der wesentlichen Änderungen hinzufügen. Die Version
kommt aus dem jeweiligen FirmwareVersion.h, niemals aus einem zweiten Versionsfeld.
Fehlende Beschreibungen brechen die Veröffentlichung ab. Keine vollständige
Commitliste einfügen. Historische Versionen werden erst ab Einführung dieses
Workflows gesammelt; alte Images werden nicht nachträglich aus ungeprüften Ständen gebaut.

GitHub Releases `firmware-<ziel>-<version>` halten Anwendungsimage, LICENSE und
`firmware.json` mit Commit, UTC-Zeit, Prüfsumme und Beschreibung. Jede neue
Versionsnummer (einschließlich Patch-Version) wird einmal veröffentlicht. Bereits
veröffentlichte Versionen werden vollständig übersprungen; Image, Beschreibung und
Metadaten bleiben erhalten. Für eine neue öffentliche Firmware ist daher die
Versionsnummer im jeweiligen Header zu erhöhen und eine Änderungsbeschreibung
hinzuzufügen. Git-Commits ohne Versionsänderung dürfen weiterhin gepusht werden,
veröffentlichen aber kein neues Firmware-Image.

Neue Releases bleiben bis zum erfolgreichen Asset-Upload als Entwurf verborgen.
Ein unterbrochener Upload kann beim nächsten Lauf fortgesetzt werden. Bereits
veröffentlichte Versionen aus dem bisherigen Ablauf bleiben unverändert erhalten.
Lokale Builds exportieren weiterhin den aktuellen Entwicklungsstand nach
`firmware/bin/` und ersetzen dort die Datei derselben Version. Alte öffentliche
Versionen bleiben erhalten; docs.envnode.de verlinkt die Release-Assets.

Lokale Vorschau ohne Netzwerk: `python tools/firmware_downloads.py preview`.
Der Workflow ersetzt die Platzhalterseite mit dem vollständigen Release-Katalog.

Für die Remote-Node später ein separates PlatformIO-Environment und einen eigenen
Versionsheader in TARGETS in `tools/firmware_downloads.py` aufnehmen, den Build und
Publish-Aufruf im Workflow ergänzen und eigene Änderungsbeschreibungen anlegen.
Auch die Zuordnung des Dateipräfixes im Build-Skript muss ergänzt werden.
