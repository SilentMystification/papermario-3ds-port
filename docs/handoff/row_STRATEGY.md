# Strategy review of plan v3 (ORACLE8, 2026-11-12, code read of main b8b314758). The user decides. plan_endgame.md is not changed.

## 1. Facts from the code
- LISTCEN2 on main: active builder calls 3768, of which 464 are the macro definitions in include (gbi.h 382, gbi_custom.h 61,
  gs2dex.h 18, gt.h 3). They go when the headers go. Call sites to convert: about 3300. gMainGfxPos uses (3198) are the first
  argument of the same calls: they are not extra work.
- Sample (src/effects 127 files and src/world, active lines only, LISTCEN2 rule): 1720 calls. 7 value calls make 73 % of them:
  gSPMatrix 294, gSPPopMatrix 237, gDPPipeSync 239, gDPSetPrimColor 188, gSPSegment 129, gDPSetTileSize 86, gDPSetEnvColor 82.
  State calls (combine, render mode, cycle type, geometry, texture load and tile, filter) make about 15 %. Draw calls (vertex,
  triangles, rectangles, fill) make about 8 %. The static lists of the effects are called through nd_rec_dl (321 sites, S7EFX).
- 10 sampled files (6 effects, 4 world): flashing_box_shockwave, water_block, disable_x, gather_magic, shattering_stones,
  bombette_breaking, flo_14/bubbles, nok_04/npc, hos_01/model_fx, kmr_21/main. Idioms:
  I1 "effect draw": PipeSync, Segment 0x09, Matrix push, PrimColor and EnvColor, static list, PopMatrix (7 of the 10 files).
  I2 "colour only": PrimColor around a model or list (hos_01/model_fx).
  I3 "texture rectangle": LoadTextureTile + TextureRectangle pairs (kmr_21/main).
  I4 "built geometry": Vertex + 2Triangles under a matrix (nok_04/npc, water_block).
- The value calls need no pipeline decision: each maps one to one to a native record (nd_xf_push, nd_xf_pop, a colour value,
  a segment base of the next list, nothing for PipeSync, a uv scale for SetTileSize). The state calls need NdPsoDesc and
  NdBindDesc (NPSO): there the call order matters and a person or a careful model reads the sequence.
- User rules that limit the options: no culling, no toggles, no stubs. Done = LISTCEN2 all 0 and elfgate func 0 data 0 gfxnames 0.

## 2. Options
| option | what it does | counters moved | rows | hours (6 coders) | risk | end proof |
|---|---|---|---|---|---|---|
| v3 as accepted | Haiku hand edit per function family, golden master per file | all | 179 | 50 to 65 | low per row, many rows | reached |
| A codemod for value calls | one host tool rewrites the 7 value calls (and gSPDisplayList, nd_rec_dl) per file, the output is committed, golden master per file proves it. State calls stay hand edits | C1, C2 about 60 % of sites by tool | about 150 (1 Sonnet tool row, 10 tool run rows replace about 35 hand rows) | 42 to 55 | the tool can be wrong in many files at once: the golden master of each file catches it (exact) | reached |
| B macros emit native records | gbi.h macros call native functions, call sites keep their names | none of C1, C2, C7 (the names and gMainGfxPos stay). C8 and C11 fall only if the walker goes | few at first, then all of A or v3 later | short at first, then the same | the state calls still need a run-time state model, which is the walker in a new name | NOT reached: LISTCEN2 build and gfxpos stay above 3000, elfgate gfxnames stays. It is a step, not an end |
| C data first (GSET before call sites) | CONV2 tables for every static list before phase C | C5 22103, C6 2107, C12 first | same rows, new order | same total, but C5 (the largest counter) falls by about 20000 in 6 rows | CONV2C (Sonnet) is on the path | reached |
| D1 N64DEL first | already in v3 phase H, running | C3 702, C4 769 | 3 | 3 | low | part of it |
| D2 dead code first | a scan lists the functions with list calls that the linker drops (not in pm_3ds.elf .symtab): delete them before conversion | C1, C2 of dead functions | 1 scan row plus 1 delete row per group | 2 to 4 | low. The count is not known before the scan (sample: draw_tiled_image and draw_adjustable_tiled_image have no caller) | part of it |
| D3 drop unused features | remove effects or maps that no script uses | some | n/a | n/a | the user rule says no culling | not allowed |

## 3. Pick: A + C + D2 inside plan v3 (not B)
- A cuts the hand work where it is mechanical (the 7 value calls, about 60 % of the 3300 sites) and keeps hand work only where
  the call order decides the pipeline state. The golden master per file stays the proof, so the risk of a tool error is caught.
- C first in time: the GSET rows remove the largest counter (C5, 22103) with the fewest rows. They already wait only for CONV2C.
- D2 is cheap and removes work before anyone converts it.
- B is fast at the start but does not reach the user's end proof. It keeps gbi.h names, gMainGfxPos and a run-time state model.

## 4. What changes in plan v3 with the pick
- New rows: DEADSCAN (Haiku, a scan of source functions with list calls against pm_3ds.elf symbols, prints the dead ones),
  DEADDEL1..n (Haiku, delete per group), CODEMOD (Sonnet design core: the rewrite tool for the 7 value calls and the static list
  call, with a host test per idiom), CODEMOD-FX1..8 and CODEMOD-WLD1..2 (Haiku tool runs per GOLD group).
- Removed or smaller: NFX1..NFX15 and NWLD1..5 shrink to the state-call edits left after the tool run (about 8 rows instead of 20).
  Phase C large groups keep their hand rows (HUD, messages, imgfx: more state calls per site).
- Order: GSET1..6 right after CONV2H, before the phase D waves. DEADSCAN at once (no blocker).
- The critical path stays the harness: HOSTCORE2 (Sonnet) -> GMHARN -> GMCOV -> GOLD. No option shortens it: it is the proof.
  Where Sonnet saves rows: CODEMOD (one Sonnet row replaces about 12 Haiku hand rows). Opus adds no value here.
- Estimate with the pick: about 150 rows, 42 to 55 h (v3: 179 rows, 50 to 65 h). Most of the saving is in phase D.

## 5. Not known by code read (named, not guessed)
- The number of dead sites (D2): the scan row gives it.
- The share of value calls outside src/effects and src/world (HUD, messages, imgfx, models): the sample covered 1720 of 3300.
  The CODEMOD row prints the share per group before its run rows are written.
