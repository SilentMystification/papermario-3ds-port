# Row EDGEAA: cutout edges show the background (osr_04 chains): the edge-AA blend writes depth before the content behind is drawn

(Model: Haiku. Base: main. Edits 3ds/src/nd_tmpl.c nd_render_state (one line) and its comment, 3ds/tools/nd_tmpl_test.c (:72-73).
Disjoint from TILEWIN2, SEAMHOST, DPCEN1. Reads this file and CLAUDE.md.)

## User facts
Console, osr_04 (Bowser's castle scene): the chains and some other textures: at the cutout edge the background shows instead of the
content behind it. The same fault was seen before.

## Cause (oracle code read)
- The chains are CI4 (osr_kusaritif, osr_kusari_uetif in assets/us/mapfs/tex/osr_tex.json), drawn with a TEX_EDGE render mode with
  the AA blender (G_RM_AA_ZB_TEX_EDGE: the ALPHATEST models). nd_edge_aa (nd_tmpl.c:286-288) matches it, and since KMR11W (09856e78b,
  2026-11-09) nd_render_state sets `o->blend = 1; o->atest = 0x1F` for it (nd_tmpl.c:302): every edge pixel with alpha 32 to 254 (the
  bilinear mix of an opaque and an alpha-0 texel) is blended with the frame buffer and writes depth (Z_UPD of the mode, :310). Whatever
  is behind the chain and drawn after it (other chains, sprites, entities, effects) then fails the depth test at those pixels, so the
  colour mixed in is the one already in the frame (the sky or the clear colour). Every texture type drawn with this mode has the path:
  CI4/CI8 cutouts, IA and RGBA16 (1-bit alpha) cutouts. Draw order decides where it shows; mac_01's cutouts mostly stand in front of
  walls drawn earlier, so the mix there is with the right colour.
- KMR11W added the blend for the dark rim of an alpha-0 key colour. TEXKEY2 (3de9c9d70) now gives every alpha-0 palette entry of all 28
  map texture archives the colour of its opaque neighbours (3ds/tools/texkey_bake.py), so a non-blended cutout no longer shows that rim
  for CI textures. The rule before KMR11W (blend 0, alpha test 0x30: the CVG_X_ALPHA texture edge, :303) writes depth only for pixels
  that are drawn at full weight, so nothing behind is hidden by a half pixel. IA and RGBA16 cutouts keep their own transparent texel
  colour (not baked): a thin rim of that colour may come back for them (named, the user's check).

## Start
`sh 3ds/tools/wt_new.sh EDGEAA`. PM_ROW=EDGEAA PM_LANE=EDGEAA on every build and bench call.

## Exact edits
1. nd_tmpl.c:302: `if (nd_edge_aa(m)) { o->blend = 1; o->atest = 0x1F; }` -> `if (nd_edge_aa(m)) { o->blend = 0; o->atest = 0x30; }` and
   replace the KMR11W comment above it by: `EDGEAA: an AA texture edge is a cutout (alpha test 0x30, no blend): a blended edge pixel
   wrote depth and hid what is drawn behind it later (osr_04 chains); the dark rim KMR11W blended away is fixed by TEXKEY2's palette
   bake.` Keep nd_edge_aa (other readers: grep nd_edge_aa).
2. nd_tmpl_test.c:72-73: delete the KMR11W comment and its `if (nd_edge_aa(om_l) && ...) { rs.blend = 0; rs.atest = 0x30; }` line
   (the rule equals the held words again).
3. grep 3ds/tools for other tests of the 0x1F edge rule (`nd_model_test.c`); if one asserts blend 1 / atest 0x1F for edge AA, change
   it to blend 0 / atest 0x30 with the same comment.

## Gate (CLAUDE.md 2; Azahar budget 4 of 8)
- Build, memcheck OK; `bench.py run nd_host --row EDGEAA --why "edge AA render rule"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`; one list `bench.py run spot_mac01_det,mdl5_iwa10_det --row EDGEAA
  --why "cutout edges without blend"`; `bench.py cmp` of both: write the diff counts (cutout edges change by design: no pixel
  pass/fail). The look is the user's check in osr_04 (chains) and mac_01 (trees, fences).

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line.
