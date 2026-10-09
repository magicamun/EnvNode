"""Include only explicitly reviewed downloads; never copy the hardware tree."""
from pathlib import Path
from mkdocs.structure.files import File

DOWNLOADS = {
    "assets/downloads/EnvNode-LICENSE.txt": "LICENSE",
    "assets/downloads/EnvNode_Mini_Bestueckungs_und_Bringup_Rev0.8.pdf":
        "hardware/kicad/MainBoards/EnvNode Mini/Documentation/EnvNode_Mini_Bestueckungs_und_Bringup_Rev0.8.pdf",
    "assets/downloads/EnvNode_Weather_Bestueckungs_und_Bringup_Rev0.2.pdf":
        "hardware/kicad/MainBoards/Weatherstation/Documentation/EnvNode_Weather_Bestueckungs_und_Bringup_Rev0.2.pdf",
    "assets/downloads/ORing_Power_Bestueckungs_und_Bringup_Rev0.1.pdf":
        "hardware/kicad/DesignBlocks/ORing Power/Documentation/ORing_Power_Bestueckungs_und_Bringup_Rev0.1.pdf",
    "assets/downloads/DuoRelay_Bestueckungs_und_Bringup_Rev0.3.pdf":
        "hardware/kicad/Modules/FullSize/DuoRelay/Documentation/DuoRelay_Bestueckungs_und_Bringup_Rev0.3.pdf",
    "assets/downloads/AnalogHydroPressure_Bestueckungs_und_Bringup_Rev0.5.pdf":
        "hardware/kicad/Modules/FullSize/AnalogHydroPressure/Documentation/AnalogHydroPressure_Bestueckungs_und_Bringup_Rev0.5.pdf",
}

def on_files(files, config, **kwargs):
    root = Path(config.config_file_path).resolve().parent
    for target, source in DOWNLOADS.items():
        path = root / source
        if not path.is_file():
            raise FileNotFoundError(f"Approved documentation download missing: {source}")
        if files.get_file_from_path(target):
            raise ValueError(f"Duplicate documentation asset: {target}")
        files.append(File.generated(config, target, content=path.read_bytes()))
    return files
