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
`firmware.json` mit Commit, UTC-Zeit, Prüfsumme und Beschreibung. Derselbe
Versionsstand wird beim nächsten erfolgreichen Lauf überschrieben. Der Release-Tag
markiert den ersten veröffentlichten Stand dieser Version; für den konkreten
Download ist der Commit in `firmware.json` beziehungsweise auf der Downloadseite
maßgeblich. Alte Versionen bleiben erhalten. Die Dateien auf docs.envnode.de
verlinken auf diese Release-Assets.

Lokale Vorschau ohne Netzwerk: `python tools/firmware_downloads.py preview`.
Der Workflow ersetzt die Platzhalterseite mit dem vollständigen Release-Katalog.

Für die Remote-Node später ein separates PlatformIO-Environment und einen eigenen
Versionsheader in TARGETS in `tools/firmware_downloads.py` aufnehmen, den Build und
Publish-Aufruf im Workflow ergänzen und eigene Änderungsbeschreibungen anlegen.
Auch die Zuordnung des Dateipräfixes im Build-Skript muss ergänzt werden.
