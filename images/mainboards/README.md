# Current mainboard renderings

Generated on **18 October 2026** from Mini PCB **0.8.1** and Weather PCB **0.2.1**.
The matching `*-provenance.json` files record source SHA-256 fingerprints, camera
options, the transparent crop rectangle and the final PNG hash. These images
replace the older board views in the current assembly PDFs. The historical
connector close-ups remain representative connector references.

Use KiCad CLI 10.0 and the settings from
[HardwareDocumentationStyle.md](../../docs/HardwareDocumentationStyle.md):

```sh
kicad-cli pcb render --width 2000 --height 2000 \
  --side top --background transparent --quality high \
  --rotate '-25,0,5' --perspective --zoom 0.72 \
  --output board-top.png board.kicad_pcb
```

Crop only the transparent outer margin (Pillow RGBA `getbbox()`); retain all
geometry and shadows. Fit proportionally into the guide's existing image area.
Replace the old image object and alpha mask together. Board Identity remains
0.8 / 0.2; the patch suffix records silkscreen changes, not new electrical or
mechanical geometry. This is a rendering/documentation update, not a new
ERC/DRC or physical bring-up.

EnvNode-owned images and provenance: LicenseRef-EnvNode-Use-and-Redistribution-1.0;
see the root [LICENSE](../../LICENSE). Third-party KiCad component models retain
their original licenses and notices; their rendering does not relicense them.
No third-party model source is distributed in this directory.
