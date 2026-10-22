"""Include only explicitly reviewed downloads; never copy the hardware tree."""
from pathlib import Path
from mkdocs.structure.files import File

DOWNLOADS = {
    "assets/downloads/EnvNode_I2C_Carrier_HalfSize_Jumper_Dokumentation_Rev0.1.pdf":
        "hardware/kicad/Modules/HalfSize/I2C Carrier HalfSize/EnvNode_I2C_Carrier_HalfSize_Jumper_Dokumentation_Rev0.1.pdf",
    "assets/downloads/EnvNode-LICENSE.txt": "LICENSE",
    "assets/images/oring-power/ORing-Power-testpoints.png":
        "images/oring-power/testpoints/ORing-Power-testpoints.png",
    "assets/downloads/EnvNode_Mini_Bestueckungs_und_Bringup_Rev0.8.pdf":
        "hardware/kicad/MainBoards/EnvNode Mini/Documentation/EnvNode_Mini_Bestueckungs_und_Bringup_Rev0.8.pdf",
    "assets/downloads/EnvNode_Weather_Bestueckungs_und_Bringup_Rev0.2.pdf":
        "hardware/kicad/MainBoards/Weatherstation/Documentation/EnvNode_Weather_Bestueckungs_und_Bringup_Rev0.2.pdf",
    "assets/downloads/ORing_Power_Bestueckungs_und_Bringup_Rev0.1.pdf":
        "hardware/kicad/Power/ORing Power/Documentation/ORing_Power_Bestueckungs_und_Bringup_Rev0.1.pdf",
    "assets/downloads/DuoRelay_Bestueckungs_und_Bringup_Rev0.3.pdf":
        "hardware/kicad/Modules/FullSize/DuoRelay/Documentation/DuoRelay_Bestueckungs_und_Bringup_Rev0.3.pdf",
    "assets/downloads/AnalogHydroPressure_Bestueckungs_und_Bringup_Rev0.5.pdf":
        "hardware/kicad/Modules/FullSize/AnalogHydroPressure/Documentation/AnalogHydroPressure_Bestueckungs_und_Bringup_Rev0.5.pdf",
    "assets/downloads/RainDetectorController_Bestueckungs_und_Bringup_Rev0.3.pdf":
        "hardware/kicad/Sensors/RainDetectorController/Documentation/RainDetectorController_Bestueckungs_und_Bringup_Rev0.3.pdf",
    "assets/downloads/RainDetectorHead_Bestueckungs_und_Bringup_Rev0.1.pdf":
        "hardware/kicad/Sensors/RainDetectorHead/Documentation/RainDetectorHead_Bestueckungs_und_Bringup_Rev0.1.pdf",
}


# Current PCB patch names for public downloads. Keep the existing URLs available
# for bookmarks; both names serve the same reviewed guide (ORing: base + supplement).
for _base, _patch in (
    ("EnvNode_Mini_Bestueckungs_und_Bringup_Rev0.8.pdf",
     "EnvNode_Mini_Bestueckungs_und_Bringup_Rev0.8.1.pdf"),
    ("EnvNode_Weather_Bestueckungs_und_Bringup_Rev0.2.pdf",
     "EnvNode_Weather_Bestueckungs_und_Bringup_Rev0.2.1.pdf"),
    ("ORing_Power_Bestueckungs_und_Bringup_Rev0.1.pdf",
     "ORing_Power_Bestueckungs_und_Bringup_Rev0.1.1.pdf"),
):
    DOWNLOADS[f"assets/downloads/{_patch}"] = DOWNLOADS[f"assets/downloads/{_base}"]


# Shared, reviewed Mini reference views also used by the Weather guide.
for _ref in ("ADC1", "J5"):
    for _view in ("oblique", "top"):
        _name = f"{_ref}-{_view}-annotated.png"
        DOWNLOADS[f"assets/images/connectors/{_name}"] = f"images/envnode-mini/connectors/{_name}"

def pdf_documents(config):
    """An explicit manifest selects only the latest approved source for each guide."""
    import yaml
    root = Path(config.config_file_path).resolve().parent
    documents = yaml.safe_load((root / "docs/pdf/documents.yml").read_text())
    sources, targets = set(), set()
    for document in documents:
        source, target = document["source"], document["target"]
        if source in sources or target in targets:
            raise ValueError("Duplicate PDF source or destination")
        if not target.startswith("assets/downloads/") or not target.endswith("_Markdown-Pilot.pdf"):
            raise ValueError(f"Invalid pilot PDF destination: {target}")
        for base, relative in ((root / "docs/site", source), (root, document["original"]), (root, document["images"])):
            path = (base / relative).resolve()
            if not path.is_relative_to(base.resolve()) or not path.exists():
                raise ValueError(f"Missing or invalid PDF input: {relative}")
        sources.add(source)
        targets.add(target)
    return documents


def on_files(files, config, **kwargs):
    for document in pdf_documents(config):
        if files.get_file_from_path(document["target"]):
            raise ValueError(f"Duplicate generated PDF: {document['target']}")
        files.append(File.generated(config, document["target"], content=b""))
    root = Path(config.config_file_path).resolve().parent
    for target, source in DOWNLOADS.items():
        path = root / source
        if not path.is_file():
            raise FileNotFoundError(f"Approved documentation download missing: {source}")
        if files.get_file_from_path(target):
            raise ValueError(f"Duplicate documentation asset: {target}")
        files.append(File.generated(config, target, content=path.read_bytes()))
    return files


# Reserved downloads are replaced after HTML generation. Any PDF error prevents
# a successful build and therefore prevents deployment of incomplete artifacts.
def on_post_build(config, **kwargs):
    import importlib.util
    root = Path(config.config_file_path).resolve().parent
    spec = importlib.util.spec_from_file_location("envnode_docs_pdf", root / "tools/docs_pdf.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.render_all(config, pdf_documents(config))
