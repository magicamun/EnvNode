# RainDetector mechanical revisions - 21 October 2026

Native KiCad 10.0.5 renders of Head 0.2 (ENP-00139) and Controller 0.4
(ENP-00138). Actual working-tree PCB geometry; no generated replacement parts.
Each board is shown from F.Cu and B.Cu with the shared ORing camera angles.
`render.py` reproduces the raw views; KiCad must be installed at the configured path.
PNG transparency is preserved; white-background product images add article/revision
labels for InvenTree. Product images were uploaded to the corresponding parts and
verified in the UI. Kits and assemblies were not changed.

The outline is 62.12 x 54.52 mm at the Edge.Cuts centerline (0.05-mm drawing
stroke excluded). Source hashes, camera settings and extracted reference positions
are in `provenance.json`. J1/J2 and Head C1 retain their former positions/orientations;
PCB pad-net assignments match the previous committed boards. Mounting-hole positions
are documented separately per board; the lower row differs by about 0.1 mm.
No new full ERC/DRC or physical mechanical/environmental qualification is claimed.

EnvNode-owned documentation and image layouts: see ../../LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).
The native render includes model geometry referenced by the PCB and provided by the
installed KiCad libraries. Third-party models and library data retain their original
licenses and notices; no model files are copied or relicensed by this update.
Renderings show intended geometry, not proof of coating, via sealing or manufacture.
