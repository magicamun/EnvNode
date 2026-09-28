#!/usr/bin/env python3
"""Generate the clean EnvNode vector logo from outlined glyphs and geometry."""

from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parent
OUTPUT = ROOT / "envnode-logo_clean-v1.svg"
ICON_OUTPUT = ROOT / "envnode-icon_clean-v1.svg"
FONT = Path("/System/Library/Fonts/Supplemental/Arial Rounded Bold.ttf")


def outlined_wordmark() -> str:
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_svg = Path(temp_dir) / "wordmark.svg"
        subprocess.run(
            [
                "hb-view",
                str(FONT),
                "EnvNode",
                "--font-size=390",
                "--margin=0",
                "--foreground=000000",
                "--background=00000000",
                "-O",
                "svg",
                "-o",
                str(temp_svg),
            ],
            check=True,
        )
        source = temp_svg.read_text(encoding="utf-8")
    definitions = re.search(r"<defs>(.*?)</defs>", source, re.S)
    if definitions is None:
        raise RuntimeError("hb-view did not generate SVG glyph definitions")
    return definitions.group(1).strip()


glyphs = outlined_wordmark()

svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink"
     viewBox="0 0 2172 724" width="2172" height="724" role="img"
     aria-labelledby="title description">
  <title id="title">EnvNode</title>
  <desc id="description">EnvNode Wortmarke mit tropfenförmigem Leiterbahn-Signet</desc>
  <defs>
    <linearGradient id="dropGradient" x1="72" y1="0" x2="490" y2="0" gradientUnits="userSpaceOnUse">
      <stop offset="0" stop-color="#080909"/>
      <stop offset="0.48" stop-color="#101111"/>
      <stop offset="0.53" stop-color="#c18d05"/>
      <stop offset="1" stop-color="#f2c72f"/>
    </linearGradient>
    <linearGradient id="goldGradient" x1="0" y1="190" x2="0" y2="520" gradientUnits="userSpaceOnUse">
      <stop offset="0" stop-color="#f6d243"/>
      <stop offset="0.5" stop-color="#d9a814"/>
      <stop offset="1" stop-color="#b77d00"/>
    </linearGradient>
    <linearGradient id="goldTextGradient" x1="0" y1="70" x2="0" y2="385" gradientUnits="userSpaceOnUse">
      <stop offset="0" stop-color="#f5cf39"/>
      <stop offset="0.48" stop-color="#dfb01d"/>
      <stop offset="1" stop-color="#bd8304"/>
    </linearGradient>
    {glyphs}
  </defs>

  <!-- Geometric symbol: one drop outline and a small circuit tree. -->
  <g fill="none" stroke-linecap="round" stroke-linejoin="round">
    <path d="M275 590 C266 552 239 535 197 511 C121 468 87 414 96 339 C104 266 159 206 275 93 C391 206 446 266 454 339 C463 414 429 468 353 511 C311 535 284 552 275 590Z"
          stroke="url(#dropGradient)" stroke-width="43"/>
    <g stroke="url(#goldGradient)" stroke-width="25">
      <path d="M275 574 V414"/>
      <path d="M275 414 V265"/>
      <path d="M275 414 L169 349"/>
      <path d="M275 414 L381 349"/>
    </g>
  </g>
  <g fill="url(#goldGradient)" stroke="#9a6800" stroke-width="4">
    <circle cx="275" cy="414" r="44"/>
    <circle cx="275" cy="234" r="38"/>
    <circle cx="169" cy="349" r="35"/>
    <circle cx="381" cy="349" r="35"/>
  </g>
  <g fill="#080909">
    <circle cx="275" cy="234" r="14"/>
    <circle cx="169" cy="349" r="12"/>
    <circle cx="381" cy="349" r="12"/>
  </g>

  <!-- Outlined Arial Rounded wordmark; no external font is required. -->
  <g transform="translate(520 155) scale(.86)">
    <g fill="#090a0a">
      <use xlink:href="#glyph-0-0" x="0" y="369.046875"/>
      <use xlink:href="#glyph-0-1" x="260.125" y="369.046875"/>
      <use xlink:href="#glyph-0-2" x="490.546875" y="369.046875"/>
    </g>
    <g fill="url(#goldTextGradient)">
      <use xlink:href="#glyph-0-3" x="701.921875" y="369.046875"/>
      <use xlink:href="#glyph-0-4" x="998.234375" y="369.046875"/>
      <use xlink:href="#glyph-0-5" x="1233.796875" y="369.046875"/>
      <use xlink:href="#glyph-0-6" x="1477.546875" y="369.046875"/>
    </g>
  </g>
</svg>
'''

OUTPUT.write_text(svg, encoding="utf-8")

icon_svg = '''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 640 640" width="640" height="640" role="img" aria-labelledby="title description">
  <title id="title">EnvNode</title>
  <desc id="description">Tropfenförmiges EnvNode-Leiterbahn-Signet</desc>
  <defs>
    <linearGradient id="dropGradient" x1="72" y1="0" x2="490" y2="0" gradientUnits="userSpaceOnUse">
      <stop offset="0" stop-color="#080909"/><stop offset="0.48" stop-color="#101111"/><stop offset="0.53" stop-color="#c18d05"/><stop offset="1" stop-color="#f2c72f"/>
    </linearGradient>
    <linearGradient id="goldGradient" x1="0" y1="190" x2="0" y2="520" gradientUnits="userSpaceOnUse">
      <stop offset="0" stop-color="#f6d243"/><stop offset="0.5" stop-color="#d9a814"/><stop offset="1" stop-color="#b77d00"/>
    </linearGradient>
  </defs>
  <g transform="translate(45 4)">
    <g fill="none" stroke-linecap="round" stroke-linejoin="round">
      <path d="M275 590 C266 552 239 535 197 511 C121 468 87 414 96 339 C104 266 159 206 275 93 C391 206 446 266 454 339 C463 414 429 468 353 511 C311 535 284 552 275 590Z" stroke="url(#dropGradient)" stroke-width="43"/>
      <g stroke="url(#goldGradient)" stroke-width="25"><path d="M275 574 V414"/><path d="M275 414 V265"/><path d="M275 414 L169 349"/><path d="M275 414 L381 349"/></g>
    </g>
    <g fill="url(#goldGradient)" stroke="#9a6800" stroke-width="4"><circle cx="275" cy="414" r="44"/><circle cx="275" cy="234" r="38"/><circle cx="169" cy="349" r="35"/><circle cx="381" cy="349" r="35"/></g>
    <g fill="#080909"><circle cx="275" cy="234" r="14"/><circle cx="169" cy="349" r="12"/><circle cx="381" cy="349" r="12"/></g>
  </g>
</svg>
'''

ICON_OUTPUT.write_text(icon_svg, encoding="utf-8")
print(OUTPUT)
print(ICON_OUTPUT)
