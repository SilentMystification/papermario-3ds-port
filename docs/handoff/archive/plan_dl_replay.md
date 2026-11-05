# plan_dl_replay: cheap replay of static display lists (rows D1 to D7)

Tags: [M] measured (source named), [C] read from code (file:line at HEAD e5cd35dc0), [I] inference.
Sources: console_numbers.md, diag_dl.md (dl1 to dl3 R3), spec_dlc_classes.md, review_render_pipeline.md, 3ds/src/pm_gbi.c, 3ds/src/pm_dlc.inc.c.
Plan only. No source edit was made for this file.

## 1. Facts
- Console yard: wall 69.3 ms, gbi walk 34.3 ms (49.5 %), 4545 ops per frame, 7.6 us per op, 2 tasks [M console_numbers 2-3].
- Azahar yard: 238.5 G_DL per frame. dlc replays class 1: 68 lists, 1120 ops, 109 groups. Interpreter ops 3223 to 3261 [M diag_dl dl2, R2].
- dlsrc per frame (Azahar, f9600-9899): main 2626, class 2 (TextureHeap) 1647 of which state 1553, class 3 (rodata) 605, other (class 7) 987 [M diag_dl R1].
- R2 Azahar walk 10.07 ms, ops 3261. R3 walk 28.0 ms, pdl hit 6, pdlm 11 per frame. R3 reverted [M diag_dl].
- Today pdl_try and pdl_try_m are dead after the first compile: pdl_compile_one stays 1 [C spec_dlc_classes 0]. All Azahar numbers above have the pdl path off.

## 2. Why the R3 per-op replay cost more than the interpreter
1. pdl_tpl_drive runs each op of a nested list as its own `pm_gbi_run(pc, 8u)` [C pm_dlc.inc.c:1380]. pdl_tpl_replay does the same for each PARAM op [C 1276].
   Each pm_gbi_run call does list-entry work [C pm_gbi.c:2449-2476]: gv_epoch++, PMDL("dlsrc"), dlc_enabled() (4 switch tests), sc_ok = sr_ok = 0 (full combiner and render sync at the next draw),
   dd_end, a host_range lookup (pc_lo and pc_hi start at 0 [C 2471, 2479]), and gv_streak_flush plus tex_streak_flush at the return [C 2523-2524].
   R3 made pdl_compile_one clear again, so pdl_tpl_drive ran the main list [C pm_gbi.c:2459]. Per-op entry cost on about 3000 ops is the likely main part of +18 ms [I].
2. gv_epoch++ per call stops triangle appends to the open batch, so batches close earlier [C spec 3, pm_gbi.c:1501] [I: closes not counted in R3].
3. pdl_try computes its key from a full GbiState: gbi_state_get (memset plus about 1.1 KB of copies) and a bytewise FNV over the struct and segbase [C pm_dlc.inc.c:847, 917-918; pm_gbi.c:1647-1687].
   About 1100 multiply-xor steps per G_DL probe, on about 170 probes per frame [I: size counted from pm_gbi.c:1605-1645].
4. The full-state key misses: 6 hits per frame [M R3]. Fields that the list never reads (prim, env, lights, ldrec ring, blk ring) differ per call site [I]. Each miss compiles:
   gbi_state_get, pdl_pso_capture and pm_pso_bake [C 849-872]. pso_n grows to PSO_N - 1 and then overwrites slot 511 [C 795-796].
5. pdl_try_m saves 0 ops: on a hit it runs every op again through pm_gbi_run [C 1102-1114], after dls_hash, gbi_state_get and a full hash [C 1089-1091].
6. Replay applied the full exit GbiState with gbi_state_apply [C pm_gbi.c:1689-1721]. That also sets sc_ok = sr_ok = 0, which forces a full sync at the next draw.

## 3. What a cheap replay must look like
- Class 2 and 3 lists hold state ops only (VTX, TRI, MTX, G_DL, MOVEMEM, rects refused [C pm_dlc.inc.c:754-766]). Their effect is a pure function of (list bytes, the entry fields that they read) [I].
- So the replay is a pre-baked delta, not ops: one compare of the read fields, one copy of the written fields, a fixed set of side effects. No per-op switch, no function pointer, no pm_gbi_run [I].
- Field table (new, pm_gbi.c after GbiState at 1645): `typedef struct { void* p; u16 n; u8 grp; } GbiFld; static const GbiFld gbi_fld[];`
  One row per interpreter global, grouped by the GBIG_* bits [C pm_dlc.inc.c:540]: TIMG (timg, timg_fmt, timg_siz, timg_stride, ld_valid, ld_x/y/h/w), TILE (tile_fmt, tile_siz, tile_tmem, tile_pal, tile_wrap, tile_shift),
  TMEM (ldrec, ldrec_n, blk, blk_n), TLUT (tlut, tlut_n, tlut_pal), TILESZ (tile_sz, td, m1, tile_x0, tile_y0, tile_fx, tile_fy, cur_tile), COMB (comb_w0, comb_w1 only), OMH, OML, GEOM, TEX, PRIM, ENV, FOG, BLEND, KEYS, PRIMZ, SCISSOR.
- Pack and compare: `gbi_pack(u32 mask, u8* dst)` and `gbi_same(u32 mask, const u8* src)` walk gbi_fld with memcpy/memcmp. `gbi_unpack(u32 mask, const u8* src)` is the replay copy. No GbiState copy, no hash.
- Read sets in pdl_op_rw are incomplete [C pm_dlc.inc.c:561-563]: LOADBLOCK and LOADTILE read tile_tmem/tile_siz and the ldrec/blk rings [C pm_gbi.c:3072-3101]. LOADTLUT reads tile_tmem and td [C 3102-3121].
  Fix: LOADBLOCK/LOADTILE rd = TIMG | TILE | TMEM. LOADTLUT rd = TIMG | TILE | TLUT | TILESZ. SETTILESIZE rd adds TIMG. Without this a replay can be wrong [I].
- Entry record (replaces PdlEnt at pm_dlc.inc.c:727-735):
  `typedef struct { u32 rmask, wmask; u16 nin, nout; u8 flags; u8* blob; } PdlVar;` (blob = packed read fields, then packed written fields, then comb w0/w1)
  `typedef struct { u32 addr, gen; u8 nvar, refused; PdlVar v[4]; } PdlDelta;` Four variants per address: one list called from 2 to 4 sites with different entry fields [I].
- Static test (cheap): class 2 gen pm_pdl_heap_gen [C pm_tex.c:738, 762; src/model.c:2420]. Class 3 is .data, gen 0. Address plus gen. No content hash per call.
  Class 4 to 6 gens exist [C pm_tex.c:739-752]. Class 7 has no gen [C pm_dlc.inc.c:790]: it needs dls_hash once per frame per address (frame stamp), not per call [I]. Class 7 is out of rows D1 to D6.
- Invalidation: gen differs: drop all variants. Read fields differ from every variant: compile a new variant (round robin over 4).
  Segment base: a class 2/3 list holds no G_DL, VTX or MTX. SETTIMG uses ptr_of, so TIMG values hold resolved pointers. A changed segbase gives a changed timg only through the list bytes, which the gen covers [I]. Keep segbase out of the key.
  Matrix: not read by state lists. Combiner, othermode, texture: they are fields in the read or write masks.
- Compile (on a miss): run the list once in the interpreter: `pdl_depth++; pm_gbi_run((void*)l, nops * 8u); pdl_depth--;` after gbi_pack(rmask), then gbi_pack(wmask). Return 1 (list done).
  This replaces pdl_compile_begin/finish, pdl_comp_arm/done and the stuck pdl_compile_one flag [C 849-905].
  Refuse the variant (flag, interpreter forever for that variant) when after the run dual_on or m1.valid changed, or a SETTIMG target is a frame buffer (pm_is_cfb) or zb_fake [C pm_gbi.c:3038-3041, 3151-3160, 3213-3229].
- Replay order (one function, about 30 lines):
  1. If wmask holds TILE, TLUT, OMH or COMB and gbi_same(wmask) is false: pm_gpu_flush() once. The interpreter flushes per op, a run of flushes with no draw between is empty closes [I].
  2. Save old tile 0 wrap words and old omode_h bits 12-13 and the cycle-type bits. gbi_unpack(wmask).
  3. TILE: if tile 0 wrap changed: pm_tex_set_wrap and st0_ep/st0_ok as at pm_gbi.c:3067-3068. OMH: texfilt change: pm_tex_set_filter [C 2797-2800].
  4. COMB: gbi_set_combine(w0, w1) (new helper, body moved from pm_gbi.c:2807 onward, it also sets comb_a/b/c/d, which are not in GbiState [C 2811-2814]).
  5. sync_combine() if COMB, OMH cycle type, or TILESZ was written [C 2804, 3209, 3213]. gv_epoch++ once. pdl_pso_cur = 0, pdl_tex_slot = -1 (as the interpreter at pm_gbi.c:2513).
  6. Do not clear sc_ok/sr_ok unless COMB, OMH, OML or GEOM was written [I: read their uses at pm_gbi.c before the row].
- Vertex batches: class 1 already replays from a static vertex area with a GPU matrix palette id (gmi) and box cull on `combined` [C pm_dlc.inc.c:458-511]. Classes 4 to 6 reuse that path in row D7 [I].

## 4. Rows (each at most 100 changed lines, in order, one composer-2.5 agent, branch dl-replay)
| Row | Change (file:line) | Lines | Gate |
|---|---|---|---|
| D1 | gbi_fld table, gbi_pack/gbi_same (pm_gbi.c after 1645). pdl_op_rw read-set fix (pm_dlc.inc.c:556-580). Shadow counter at the G_DL case (pm_gbi.c:2546): for class 2/3 targets that pass pdl_refuse_ops, keep 4 shadow variants (rmask bytes only, no replay). Line `pdlsh f.. calls, rmask0, vhit, vmiss, ops_in` every 300 frames. No draw change. | about 90 | diagnostic. Gate: vhit / calls >= 70 % in yard steady state, else stop and report |
| D2 | Delete pdl_try_m and the PdlM code: pm_dlc.inc.c:945-1124 (keep the enum at 943 and PdlMIns at 944, pdl_tpl uses them [C 1131, 1274-1282]), calls at pm_gbi.c:2553-2557 and pm_dlc.inc.c:1324-1328, report fields at 1388. Pure deletion; if the 100-line cap counts deletions, split at 1040. | -190 | picture equal, statehash equal |
| D3 | pdl_tpl_gdl: when no cache takes the child, run it with one `pm_gbi_run(next, 0)` (NOPUSH: then `*pc = (Gfx*)dlc_end`). Delete the sp > 0 per-op branch and the stack in pdl_tpl_drive (pm_dlc.inc.c:1338-1385). | about 40 | picture equal, statehash equal |
| D4 | Extract gbi_set_combine(w0, w1) from pm_gbi.c:2807 onward and gbi_tile0_wrap(w1) from 3065-3069. The cases call them. No behaviour change. | about 40 | statehash equal |
| D5 | PdlDelta/PdlVar replace PdlEnt (pm_dlc.inc.c:727-742). New pdl_try (907-940) with the compile run and the replay of section 3. Delete pdl_key (847), pdl_compile_begin/finish, pdl_comp_arm/done (849-905), their calls at pm_gbi.c:2518, 2552 and pm_dlc.inc.c:1323. `pdlcheck`: every 16th hit compiles again and memcmps the new blob with the stored one (count `drawbad`). | about 100 | walk down, ops down, picture and statehash equal |
| D6 | Pre-resolve the texture: store the tile 0 slot and gen in PdlVar after the compile run (as pdl_compile_finish does at 861-867). Replay sets pdl_tex_slot/pdl_tex_gen, pdl_lazy_tile binds it (819-836). | about 20 | tex hit count per frame down, walk down |
| D7 | Spec only first (read-only agent): dlc for classes 4 to 6 (dlc_try address gate at pm_dlc.inc.c:441, DLR_VTX at 103, gen per class from pdl_heap_gen). Count first which class 4-6 lists dlc_compile accepts. | spec | count >= 300 ops per frame |
Not in this plan: class 0 main list (2626 ops, rebuilt each frame), class 7 lists, pdl_tpl_replay PARAM per-op calls (1276) [I: next spec].

## 5. Measurement per row (Azahar yard, one shot per row, PM_ROW=D<n>)
- Run: `$env:PM_LANE='dl'; $env:PM_ROW='D<n>'; powershell -File 3ds/run_azahar.ps1 -Timeout 90 -LockWait 180 -Debug "fast preset=yard phase fixedseed"`. Build: `PM_LANE=dl sh 3ds/build.sh`. Before the build run `python 3ds/tools/gen_map_restore.py`, then restore 3ds/src/map_restore_table.c.
- The until-line is the first `pdl f..` line (or `pdlsh f..` for D1) after frame 299. Read from the same 300-frame window:
  walk ms = taskpl bg + main. Ops interpreted = taskpl ops. Closes = flsplit tex closes and empty closes. Cache = pdl hit/comp/refuse/drawbad, dlcsplit hit (must stay 68).
- Baseline (R2, Azahar, f105): walk 10.07 ms, ops 3261, dlc hit 68 [M]. Tex closes 224.4, empty 161.4 per frame [M dl2].
- D5 expected [I]: ops down by about the class 2 + 3 state ops that hit (up to 1553 + 605 = 2158 [M R1]). Walk down 2 to 5 ms Azahar. Empty tex closes down (one flush per replay). drawbad 0.
  Console estimate [I]: 2000 ops x 4.8 us residual = up to about 9 ms walk. The user judges this on the console only after D6 passes.
- A row whose shot does not show its quantity change: revert its files before the next row (rule 7). D2, D3, D4 are no-change rows: walk within 0.3 ms of the row before, ops equal.
- Status line per row to 3ds/logs/perf/status_D<n>.txt: walk, ops, closes, cache counters, picture verdict, statehash verdict, starts used.

## 6. Test plan (per row D2 to D6; D1 is diagnostic, budget 3)
1. Yard picture (shotmode 4:80 as in dl2): Mario, ground with texture, shadow. Compare with the R2 shot build3ds/lane-dl/shots/0040.ppm. Any texture or colour change fails the row.
2. statehash with fixedseed, must equal the recorded bases:
   toad f100 94083528, f200 83b272fe, f300 aed188c8. battle f100 94083528, f200 dda688fa, f300 6b6fe648 [M diag_dl R2].
3. Pause menu text: open the pause menu, shot must show readable text in every tab name. Class 3 render-mode lists and texture lists run there [I].
4. File select flip: shot of the flip. Text strips correct, no stale palette (LOADTLUT path, pm_gbi.c:3113-3119) [I].
5. golden.py on 1 to 3 named scenes once at the merge: `python 3ds/tools/golden.py toad yard`. Not per row.
- Launch for 3 and 4: the agent greps 3ds/SLICES.md for "pause" and "PT86" for the existing switches. Do not guess a preset.
- Budget: 8 starts per row. One start per check. No repeat of the same shot.

## 7. Risks [I]
- A read field missed by gbi_fld or pdl_op_rw gives a wrong texture with equal statehash. Picture checks 1, 3, 4 and `pdlcheck` drawbad cover it.
- 4 variants can be too few at a list with many call sites: vmiss in D1 shows it before D5.
- D3 turns pdl_tpl_drive on again at D5 (pdl_compile_one no longer stuck). If D5 walk rises, the next step is a read-only diagnosis of tpl seg/fb counters, not a code agent.
