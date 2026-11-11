# Row KZN04: shaded sprites drawn on the GPU (kzn_04 15-16 fps on the O3DS XL)

(Model: Sonnet, user approved 2026-11-11. Reason: the change of the shaded sprite path is a design (two texture units and a TEV
combine in place of a CPU decode per sprite per frame) with choices Haiku cannot make. Non-byte-equal shading is accepted by the user.)
## User facts
"The room kzn_04 is very laggy." XL (main de4f4b8c8): fps 15.7/16.6, cls tex 23.4 ms (mac_01 under 2), ndc trect 5.4 per frame.
## Cause (oracle code read, 141c5e371; diagnostic bench kzn04_shadestat merged)
kzn_04 sets SetSpriteShading (src/world/area_kzn/kzn_04/main.c:15): every NPC (12 fire bars), Mario and the partner draw shaded. Per
shaded sprite per frame: create_shading_palette (src/sprite_shading.c:66) draws the 16x1 shading palette rect, then tex_shaded0
(3ds/src/nd_tex_set.c:569) -> load_shaded_sprite (:647-676) builds a 256-entry lut on the CPU: for each sprite palette colour i (tile 1
palette sp) and shading colour j (tile 0 palette sh) the second-cycle combine cc_cycle1(t0, t1), and pm_tex_load_shaded (3ds/src/pm_tex.c
:1819) keys a decoded texture by the hash of that lut. The fire bars move the light every frame, so sh changes, the lut changes, and the
load is a miss: a full decode and upload per sprite per frame.
## Goal
No per-frame CPU decode for a shaded sprite: the sprite texels (sprite palette, stable: cached as today by raster and palette) and the
16 shading colours (tile 0, per frame: 16 entries) go to the GPU as two inputs, and the TEV computes the cc_cycle1 combine. If the
index pairing (i, j) per texel cannot be expressed with two units on the PICA (no dependent texture read), use the nearest form the
user accepts: the shading as a per-sprite colour (the mean of the 16 shading colours weighted by the sprite's palette use), and say so.
## Start
`sh 3ds/tools/wt_new.sh KZN04` (build3ds/wt/KZN04, branch KZN04 from main). PM_ROW=KZN04 PM_LANE=KZN04 on every call.
## Places
Edits: 3ds/src/nd_tex_set.c tex_shaded0 (:569) and load_shaded_sprite (:647); 3ds/src/pm_tex.c pm_tex_load_shaded (:1819) and new
helpers next to it; the sprite TEV setup only after HC8 merges (HC8 edits pm_gpu.c pm_gpu_spr_build :3796 and nd_model.c :389-410: do
not edit those until HC8 is merged; `git merge main` before the completing commit). Not nd_tex_set.c ensure_tile (HC7 merged code).
## Gates
- memcheck OK; `python 3ds/tools/bench.py run nd_host --row KZN04 --why "..."`: PASS.
- Byte-equal (base, run, cmp) on scenes without shading: spot_mac01_det and mdl5_iwa10_det (bases stored at b8bc06613).
- `python 3ds/tools/bench.py run kzn04_shadestat --row KZN04 --why "..."`: the shadestat line shows the miss count per 300 frames
  near 0 (it was about 3 or more per frame). Write calls, miss and us in the commit. Azahar list: kzn04_shadestat, spot_mac01_det,
  mdl5_iwa10_det on one slot (3 starts, plus the bases if not stored).
- The look of the shading in kzn_04 and the fps: the user's console check.
## Stop and end
A failed gate or a tool fault: commit and hand back the exact output (CLAUDE.md 4-5). At the end delete this row file in the
completing commit. Hand back: the ready hash, the gate results, the memcheck line, and which form (two units or per-sprite colour).
