# plan_model_ops: model state ops in mac_01 (area 1 map 2)
Plan only. Read at HEAD 3141a0913. Rules: hw_guidelines.md. Console = Azahar x 2.86. Texture close = 97 us (console).

## 1. Data (console, build 29f504d7b, opsplit f2482/f2782/f3082, mean)
- gbi 31.7 ms, 2145 ops per frame. 1101 of them are nested in dlc replays (dlcsplit ops). About 1040 ops are top-level.
- Producer appendGfx_model (0x00161328): 1696 to 2041 ops, 29.8 to 38.7 ms (sampled).
- Top ops: 0a G_PM_TEX 10.5 ms for 88 binds (119 us per bind; the close is in it). ff dlc 6.8 ms for 96 replays.
  06 tri 3.3 ms. 01 vtx 2.7 ms. fc SETCOMBINE 3.0 ms. da MTX 1.4 ms. d9 GEOMETRYMODE 1.4 to 1.6 ms.
- Classes: tex 14.8 ms/597, cc 3.8 ms/239, geo 2.7 ms/470, mtx 2.2 ms/197, dl 1.7 ms/289, oth 1.8 ms/102.
- rmsplit n 89, tbsplit n 88 fb 0, draws 86. One model makes one bind and one draw.
- pm_gpu_note_mvp puts the matrix in the triangle palette (GV1c). A matrix change does not close a draw. A texture change does.
- pm_gpu_set_mode returns early when the mode does not change. A same GEOMETRYMODE costs only dispatch.
- No Azahar opsplit data for mac_01 is in the tree. The tour10 run has one opsplit line, and that line is on dgb_01.
  The repeat rates that follow are estimates. R0 measures them.

## 2. Per-op cost per model
Per model the main list has: PipeSync (G1 skip), lights (0 or 3 ops), G_PM_TEX or 4 untextured ops,
SolidCombineModes SETCOMBINE (fog or alpha test), G_PM_RM, fog 2 ops, MTX, pan SETTILESIZE, G_DL shape (dlc),
POPMTX, PipeSync. That is about 11.7 top-level ops per model.

## 3. Rows
| row | op class removed | ops/frame | console us/frame | file, function | risk | Azahar check |
|---|---|---|---|---|---|---|
| R0 | none (counters) | 0 | 0 (phase off) | pm_gbi.c os_report, new cold fn mops_count; azt_scenarios.json | none | mac01: "mops" line present, 2 windows; picture same as before |
| R1 | repeated G_PM_TEX (bind and its close) | -S (S = bind same) | -119 x S | pm_gbi.c gbi_run_body case G_PM_TEX, tex_op, tb_inline | medium | mac01: 0a us down >= 35 x S; tour10 and yard PPM equal to R0 |
| R2 | equal SETCOMBINE, TEXTURE, GEOMETRYMODE | -150 to -250 | -1500 to -2500 | pm_gbi.c g2_same | low | mac01: cc+geo us down >= 500; sks up; PPM equal |
| R3 | MVP multiply for an equal matrix | 0 (op stays) | -500 to -1000 | pm_gbi.c load_mtx, case G_POPMTX | low | mac01: mtx us down >= 175; PPM equal |
| R4 | bind body (plan_c1 C1-2 TexBind record) | 0 | -800 to -1500 | pm_gbi.c tb_run | medium | gate: after R1, Azahar 0a us per bind > 40; tbcheck 0 |
| R5 | equal light MOVEMEM/MOVEWORD | R0 decides | R0 decides | pm_gbi.c g2_same | medium | only if R0 lights_same > 60 per frame |

Order: R0, R1, R2, R3, then R4 if its gate passes. R5 only if R0 gives the count.
Sum R1 to R4: about -5.5 to -9 ms console. gbi goes to about 22 to 26 ms.
This plan does not reach 17 ms. The other 13 ms is dlc replay and vtx/tri work of the shape geometry.

### R0 counters and scenario
- Put the test in mops_count (cold, noinline). Call it from the loop only when pm_ph_on. Count what would be skipped. Do not skip.
  Fields: bind, bind_same (w1 == last bind and tex_gen unchanged), cc_same, tex_same, geo_same, mtx_same (64 bytes equal), lt_same.
- Write one line "mops f%d ..." in os_report every 300 frames, divided by 300.
- Add azt scenario mac01 (copy yard): warp=mac_01:0 phase, until the second opsplit line. Run with -Try. Save the shots as the baseline.

### R1 skip a repeated bind
- Add a static tex_gen. Increment it in tex_op (on entry), in tb_inline, in rm_apply when it writes omode_h,
  at gbi_run_body entry (sc_ok reset), on each dlc_try, pdl_try, pdl_try_m and pdl_tpl_drive hit,
  and at every write of td[], cur_tile, comb_*, tex_on outside tex_op. Find them with grep (for example ens_clear and the cfb composite).
  Add pm_gbi_tex_dirty() for pm_tex unload and evict, and call it there.
- In the loop before the prologue: if op == G_PM_TEX, w1 == tb_last and tex_gen == tb_gen, then pc++ and RS_ADD(RS_TBSK), and continue.
  After tb_run: tb_last = w1, tb_gen = tex_gen.
- No switch (CLAUDE.md rule 7). Compare the PPM against the R0 baseline for the A/B picture test.
- This is the batching row. With equal state, no close occurs, so consecutive models join one draw. Explicit batching is not needed.

### R2 G2 extension
- SETCOMBINE: comb_set and comb_w0 == w0 and comb_w1 == w1 and sc_ok. Read the full handler. Also require each side-effect field
  to hold the value the handler would write (box_folded == 0, use_t1, box_prod). If you cannot prove one, do not skip.
- TEXTURE: tex_on, tex_ss, tex_ts equal, and sc_ok.
- GEOMETRYMODE: ((geom & (w0 & 0xffffff)) | w1) == geom.

### R3 equal matrix
- In load_mtx: if the new top is equal (memcmp 64 bytes) to the top before the op, do not call upload_mvp. Keep the push and the stack as they are.
- In G_POPMTX: if the restored top is equal to the popped top, do not call upload_mvp. Identity models give the hits.

## 4. Rows that do not pay
- Per-model state cache in src/model.c (skip combine, RM, texture or matrix at the producer): do not do this. The render list puts
  sprites, entities and custom gfx between models, and shape lists change geom. The producer cannot see that state.
  The interpreter shadow state (R1 to R3) is exact, and it is one producer per row.
- Precomputed per-model packed DL (G_PM_MODEL): it removes only dispatch, about 6 ops x 2 us x 88 = about 1 ms. The model.c
  emit has fog, shroud, tint, custom gfx, panner and billboard branches. It has the same risk as the dropped retained draw. Do not do it.
- Sort opaque models by texture: it removes closes (up to about 50 x 97 us), but it changes draw order. At Z ties the picture
  is not identical. Do not do it.
- MTX skip before the prologue: the push and pop pairs need the stack. R3 removes only the multiply.
- C1-3 (entity and animator RM swap): mac_01 has 37 ops from 0x00164d8c. Under 0.3 ms. Do it later, for other scenes.
- 17 ms target: dlc replay 6.8 ms and vtx/tri 6.0 ms are geometry. They stay after this plan. Look at them in plan_dl_architecture.

## 5. Hardware
- Each skip is a few compares on 2 or 3 cache lines (tb_last, tb_gen, tex_gen, comb_w0/w1, geom) before the prologue (rules 2, 6).
- mops_count and the counters are cold and noinline, and they run only when the phase is on (rules 1, 10). No division in hot code.
- Each row commit gives the Azahar quantity and the mac01 opsplit and prod lines (rule 9).

## 6. Recommendation (one Haiku code agent)
1. R0: counters and the mac01 scenario. Baseline shots and opsplit.
2. R1: repeated bind skip. Gate: mac01 0a us down >= 35 x bind_same; PPM equal.
3. R2: G2 extension. Gate: cc+geo us down >= 500; PPM equal.
Then R3. Revert a row that fails its gate or changes one picture byte.

### Critical files
- 3ds/src/pm_gbi.c
- src/model.c
- 3ds/src/pm_tex.c
- 3ds/tools/azt_scenarios.json
- include/gbi_custom.h
