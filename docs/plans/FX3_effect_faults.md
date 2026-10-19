# FX3 effect sheet faults (read-only study)

Date: 2026-10-19. Method: code reading, file times, the sheets and raw shots. No build. No Azahar run.
Status words: STALE = the image is older than the fix. LIKELY = code evidence, not run. OPEN = no single cause found.

## Finding 1: most black faults are old images (STALE)

File times of 3ds/logs/fx/raw/*_a.ppm (the sheets are drawn from these files, the sheet time 02:54 only says when the sheet was drawn):

| Shots | Raw time | Relation to the fixes |
|---|---|---|
| effects 0 to 25, 27 to 39, 81 to 102, 104 to 120, 122 to 126 (80 shots, includes 98 red_impact and 125 star_outline) | 10-18 20:15 | before 5b1433077 (23:39, "stage 0 never reads the PICA buffer colour") |
| 93 effect_63, 112 peach_star_beam | 10-18 23:38 | with the 5b1433077 build, before H06b3 (06:59) and H06b2 (07:15) |
| 63 snowfall, 26, 40 to 80, 103, 121 | 10-19 02:54 | after 5b1433077, before H06b3 and H06b2 |

Root cause of the old black: the PICA buffer colour reads 0 in stage 0. A plan that put ENV in the buffer colour of stage 0 gave ENV = 0. The form (PRIM - ENV) * X + ENV is built as INTERP(PRIM, ENV, X) (pm_comb.c lower(), `teq(B, D)`) and reads ENV from the buffer colour. Result: black where ENV or PRIM should show. This is the FX2e fault (peach beam black cylinder). 5b1433077 and `build_lead` (pm_comb.c, one passing stage first) fixed it.

### (a) fx2_beam: STALE image, not a fault
- 3ds/logs/fx/fx2_beam.png has the time 10-18 20:34. It is a before and after picture of the FX2e investigation. It shows the black cylinder.
- The sheet cell (sheet_10 slot 112) comes from raw/112_peach_star_beam_*.ppm at 23:38, built with the fix. It shows a white beam.
- Same effect and same combiner path: fx2_beam.png is effect 112 peach_star_beam. D_09004FE8_3EAF18 combiner (PRIM, ENV, SHADE, ENV) is the INTERP path above.
- H06b3 needs constant B, C, D and a variable A. H06b2 needs B = COMBINED. The beam has a variable C (SHADE). Neither change touches the beam.
- Verdict: stale image. Proposed check: fxtest=112 fresh shot (row FX3a covers it).

### (d) star_outline (effect 125) and (e) red_impact (effect 98): LIKELY stale, same cause as (a)
- Raw shots 10-18 20:15, before 5b1433077.
- star_outline combiner (PRIM, ENV, TEXEL0 or SHADE, ENV): INTERP path, ENV from the buffer colour. Black rays with yellow glints is the FX2e signature. PRIM (255, 255, 120) and ENV (255, 120, 0) are never black (src/effects/star_outline.c:48-54).
- red_impact combiner (PRIM, ENV, TEXEL1, ENV), alpha PRIMA * TEXEL0: same INTERP path (assets/us/effects/gfx/D_090009A0_359D50.gfx.inc.c). Needs a fresh shot to judge. It also reads TEXEL1 (tile 1, 4b multi tile at TMEM 0x100, shifts 14): check tile 1 is valid at the draw if black remains.
- Test: fxtest=98,125 fresh shots (FX3a). If black remains, A/B with the switch `nocomb` (legacy path) and compare.

## Finding 2: snowfall (effect 63) is a real fault, the shot is new (02:54)

Evidence:
- Combiner (assets/us/effects/gfx/D_09000C10_38DC80.gfx.inc.c): cycle 1 colour (0 - 0) * 0 + 1 = white, alpha (T1A - T0A) * LODF + T0A. Cycle 2 colour COMBINED, alpha COMBINED * PRIMA. Two cycle, G_TL_LOD, blender G_RM_PASS then G_RM_ZB_CLD_SURF2. Texture I4, tiles 0 and 1 the same 64 x 64 image, tile 2 a 16 x 16 round blob. Vertex colour 255 white.
- The colour is a constant 1. Black dots mean colour 0 with alpha shape right. The raw triangle clusters show 8 triangles per flake (D_09000D50, 24 vertices), so the shape is the alpha.
- Checked and fine: the setup list `D_09000C00_38DC70` ("todo wut dis", 16 bytes) has no gsSPEndDisplayList. It falls into D_09000C10_38DC80. build3ds/pm_3ds.map: C00 at 0x004adc88, C10 at 0x004adc98, adjacent. The fall through works.
- Checked and fine: I4 gives GPU_LA8 with alpha = intensity (pm_tex.c:111). Shifts (V12) are stored (pm_gbi.c:195, 578, 1967).
- Not in any audit log: grep of 3ds/logs/*.txt for "LODF+T0A" finds only fmt 0/2 (another effect). The snowfall combiner (fmt 4/0, I4) was never audited, so planerr is unknown.
- Candidates, most likely first:
  1. Constant ONE in stage 0 and the 2 cycle pass. `TN_ONE` is name PM_NM_ZERO with inverted operand (pm_comb.c src_of(), `ninv ^= 1`, op PM_O_1MC). The stage constant is shared by the colour and alpha group of one stage. name_colour (pm_comb.c:374) returns 0 for PM_NM_ZERO, so the constant itself is right. Open: the alpha group of the same stage shares the stage constant, and the cycle 2 stage reads PREVIOUS. Check the real stage and operand table in the `comb new` plan dump.
  2. LOD fraction: `term_c` (pm_comb.c:60) and `cc_v` (pm_gbi.c:683) have no case 13 (LOD_FRACTION), it reads 0. Alpha becomes T0A, correct for equal tiles. Real hardware blends tile 0 and tile 2 by distance, so far flakes should use the 16 x 16 blob. The port has no LOD. This changes size and softness, not the colour.
  3. Blender: G_RM_ZB_CLD_SURF2 maps to blend on (sync_render, pm_gbi.c:482). Not likely to give black.
- Test (one start): fxtest=63 with `shots`, plus `nocomb` as A/B (pm_gbi.c:861 `pm_debug_has("nocomb")` forces the legacy path). Read the `comb new` line of the snowfall combiner (plan USED or not, planerr, nst). Dots white with `nocomb` = plan fault (candidate 1). Dots still black with `nocomb` = texture or blend fault: add `rmoff` as a second A/B.

## Finding 3: effect_63 (effect 93), black blobs: OPEN, may be partly correct

- Shot 23:38, with the 5b1433077 fix.
- Combiner D_09001A08 and D_09001B70: cycle 1 (T1 - T0) * T0 + ENV, alpha (T1 - PRIM) * ENV + T0A. Cycle 2 (COMBINED - T1) * T0 + PRIM. Both colour groups have a signed A - B with a variable A and B.
- pm_comb.c lower() last lines (`SUBTRACT A, B` then `MODULATE or MULADD`) clamp A - B at 0. The negative part is lost. The old audit line (3ds/logs/c2r_fx_log.txt #1405) for this form: INEXACT, plan no, planerr 1000, nst 6. FX2d fixed one step (0 - B*C wrap) but the 2 cycle signed form is not folded. FX2d says "OPEN LEAD" in SLICES_DONE.md and was moved done.
- This is the signed subtract class of H06b2 and H06b3 (fixed only for constant B, C, D and for K - COMBINED). A variable (T1 - T0) is the next case.
- Unknown: is a black puff the real look? ENV and PRIM come from `part->unk_38..unk_44` and `unk_28..unk_34` (src/effects/effect_63.c:226). A dark smoke may be right. Compare with the decomp colour init before a fix.
- Test: fxtest=93, read the `comb new` line for fc20aa01 12eab2c7 (plan USED or no, planerr). Log the PRIM and ENV values of the draw (one line). If ENV is near black the blob is correct.

## Finding 4 (b): flame (effect 27, red rectangle)
Row FX2a (3ds/SLICES.md) works it. Link only: raw/027_flame_*.ppm is from 10-18 23:24, before the FX2a commits 5e0c34b2f, a41fc15ca (10-19 06:14 and 06:57). The sheet is stale for FX2a too. FX3a re-shoots it.

## Finding 5 (f): sheet resolution

File: 3ds/tools/fxtest_report.py, function `report()` and function `upright()`.
- `upright()` line 143: `im.transpose(Image.ROTATE_90).resize((200, 120))`. The raw shot is 240 x 400. After the rotation it is 400 x 240. The resize halves it. Remove the `.resize(...)` (full size 400 x 240).
- Line 171: `sheet = Image.new('RGB', (1200, 4 * 134), (30, 30, 30))` becomes `(3 * 800, 4 * 254)`.
- Line 174: `cx, cy = (j % 3) * 400, (j // 3) * 134` becomes `(j % 3) * 800, (j // 3) * 254`.
- Line 179: `sheet.paste(upright(load(f)), (cx + q * 200, cy + 14))` becomes `(cx + q * 400, cy + 14)`.
- Result: 2400 x 1016 px, 11 files, about 4 times the bytes (1 to 2 MB each). The label text at line 175 stays 10 px: use `ImageFont.load_default(size=16)` if it must be larger.
- Also add one line to the header of the table: the raw file time of each shot, so a stale shot shows. Optional.

## Proposed rows for 3ds/SLICES.md

```
[ ] FX3a | (sonnet) USER-REVIEW 2026-10-19 of the effect sheets. 80 raw shots date from 10-18 20:15, before the stage 0 buffer colour fix (5b1433077). Black in fx2_beam, 98 red_impact and 125 star_outline is that old fault. Step 1: fxtest_report.py upright() keeps the full 400 x 240 and the sheet cell becomes 800 x 254 (lines 143, 171, 174, 179, docs/plans/FX3_effect_faults.md). Step 2: re-run `python 3ds/tools/fxtest_report.py run 0-25,27-39,81-102,104-120,122-126` in chunks and rebuild the sheets and docs/plans/FX1_table.md. Step 3: list each effect that is still black or SOLID | new sheets at 2400 px width, 98, 112 and 125 not black, a list of the remaining faults as new rows | 1 chunked run, PM_ROW=FX3a
[ ] FX3b | (sonnet) snowfall (effect 63) shows black dots in triangle clusters, white flakes expected. Combiner (0-0)*0+1 then COMBINED, alpha (T1A-T0A)*LODF+T0A, I4, 2 cycle LOD. Check pm_comb.c name_colour for the PM_NM_ZERO stage constant, the `comb new` line of the combiner, A/B with `nocomb` and `rmoff`. Details in docs/plans/FX3_effect_faults.md finding 2 | fxtest=63 shot shows white flakes, no `comb new` INEXACT line for the combiner | 2 starts
[ ] FX3c | (sonnet) effect_63 (effect 93) black blobs. Decide if the black is real (log PRIM and ENV of the draw). If not, the signed form (T1 - T0) * T0 + ENV and (COMBINED - T1) * T0 + PRIM needs a plan like H06b2 and H06b3 (pm_comb.c lower(), last 3 lines clamp A - B at 0). Finding 3 in docs/plans/FX3_effect_faults.md | fxtest=93 `comb new` line is PLAN-OK with planerr 0 and a shot with the decomp colours | 2 starts
```

Link: flame (effect 27) stays with FX2a, its raw shots are also stale (finding 4), FX3a re-shoots it.
