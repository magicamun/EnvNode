# EnvNode product asset contract

This contract applies to every current PCB revision which is intended to become
a webshop product. It makes documentation images reproducible and prevents a
product page from silently using assets from another hardware revision.

## Source of truth

`docs/board-views.yml` is the catalogue of documented PCB revisions. A current
entry becomes subject to this contract when it contains:

```yaml
shop_assets: true
```

The entry must identify the guide, the exact KiCad PCB source and its SHA-256
digest. Changing the PCB therefore requires an explicit review and a new asset
generation run. Archived revisions are not eligible as webshop sources.

## Generated assets

The command

```shell
python -m pip install -r requirements-docs-render.txt
python tools/product_assets.py generate
```

creates the following files under
`docs/site/assets/product-boards/<board-id>/`:

| File | Projection | Component models | Purpose |
| --- | --- | --- | --- |
| `3d-assembled-front.png` | perspective | enabled | assembled example, front |
| `3d-assembled-back.png` | perspective | enabled | assembled example, back |
| `3d-bare-front.png` | perspective | disabled | delivered bare PCB, front |
| `3d-bare-back.png` | perspective | disabled | delivered bare PCB, back |
| `2d-assembled-front.png` | orthographic | enabled | placement overview, front |
| `2d-assembled-back.png` | orthographic | enabled | placement overview, back |
| `2d-bare-front.png` | orthographic | disabled | PCB overview, front |
| `2d-bare-back.png` | orthographic | disabled | PCB overview, back |
| `asset-manifest.json` | — | — | provenance and hashes |

“Bare” means that all component model references were removed from a temporary
copy of the KiCad board. Copper, mask, silkscreen, footprints and pads remain
unchanged. Source PCB files are never modified.

The assembled views are examples of the completed board. If a component has no
KiCad 3D model, it is absent from these renders. Missing models must not be
invented or silently replaced with visually similar components.

## Product-page use

For a PCB-only article, a bare front view is the primary image. Assembled views
must be labelled “Beispielaufbau – Bauteile nicht enthalten”.

For a kit article, generated PCB views are supporting documentation only. The
eventual primary image must show the actual delivery state. Until a real kit
photo exists, a reviewed deterministic collage may be used. AI-generated or
decorative parts must never be used to represent kit contents.

Real photographs use these optional names in the same directory:

- `kit-contents-photo.jpg` — all supplied parts in the supplied quantities
- `assembled-reference-photo.jpg` — an actually assembled reference board

Their absence does not block technical documentation, but it blocks the final
shop release of a kit unless the release review explicitly accepts a temporary
collage.

## Required workflow

For every new or revised module, mainboard, sensor board or kit:

1. Add or update the current entry in `docs/board-views.yml`.
2. Set `shop_assets: true` only after selecting the webshop revision.
3. Run `python tools/product_assets.py generate --id <board-id>`.
4. Run `python tools/product_assets.py check`.
5. Review both sides and all assembled/bare views for orientation, clipping,
   revision markings and missing 3D models.
6. Reference the reviewed assets from the documentation and webshop catalogue.
7. Commit the generated PNG files and `asset-manifest.json` together with the
   PCB/documentation change.

The documentation CI runs the check command. Missing assets, stale PCB hashes,
invalid manifests and modified generated files fail the build.

## What is deliberately not automatic

The generator cannot prove that the physical kit contains every BOM line, that
component markings are readable, or that a photograph matches the delivery.
Those remain human release checks. Product pricing, legal information and shop
publication are outside this asset contract.
