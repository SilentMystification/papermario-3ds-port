# spec_dlc_classes: display-list cache for G_DL classes 2 to 7

Tags: [M] measured (source named), [C] read from code (file:line), [I] inference. Line numbers are at commit bd37d84c7.
Sources: diag_dl.md (worktree agent-a68b517c4ec639bc6, dl1 and dl2), review_render_pipeline.md, pm_gbi.c, pm_dlc.inc.c (included in pm_gbi.c:1726).

## 0. Correction to the premise
- dlc_try takes class 1 only: first test `(u32)l - gMapShapeData >= 0x40000` returns 0 [C pm_dlc.inc.c:441].
- pdl_try takes classes 2 and 3 [C pm_dlc.inc.c:902]. pdl_try_m takes classes 4 to 7 [C pm_dlc.inc.c:963, 1074]. A cache for classes 2 to 7 exists.
- Both are dead after the first compile. Cause (code path [C], match to the counters [I]):
  1. pdl_tpl_drive walks the main list. It sends each G_DL to pdl_tpl_gdl [C pm_dlc.inc.c:1350-1366].
  2. pdl_tpl_gdl calls pdl_try. On a miss, pdl_try calls pdl_compile_begin: pdl_compile_one = 1, pdl_depth = 1 [C 909-913, 869-870].
  3. pdl_tpl_gdl arms the compile and pushes the list on its own stack [C 1312, 1317-1322].
  4. pdl_tpl_drive pops the nested G_ENDDL itself [C 1358-1361]. pdl_comp_done has one caller, the G_ENDDL case of pm_gbi_run [C pm_gbi.c:2518]. pdl_compile_finish never runs. The flags stay set.
  5. With pdl_compile_one = 1: pdl_tpl_drive refuses [C pm_gbi.c:2459, pm_dlc.inc.c:1327]. pdl_try and pdl_try_m are skipped at every G_DL [C pm_gbi.c:2546, 2552].
- Counters that match: pdl_try calls per class in frames 1 to 299: class 2 1, class 7 37. After frame 299: 0 calls [M diag_dl dl1]. `pdl 0 pdlm 0` in WORLD [M review_render_pipeline.md:17].

## 1. The classes [C pm_gbi.c:1518, 1531-1540]
| Class | Address range | Content | State the list output depends on |
|---|---|---|---|
| 0 | the main list now walked | per-frame list, rebuilt each frame | everything. Not cacheable by address [I] |
| 1 | gMapShapeData, 0x40000 | static map geometry | matrix (`combined`), geom mode, lighting, vertex ring. dlc handles it: box cull on `combined`, matrix id, lit test [C pm_dlc.inc.c:457-489]. Invalidate: pm_dlc_gen [C 442, 446] |
| 2 | TextureHeap, 0x68000 | map texture setup lists, state ops only [I from the pdl_kind_s gate, C 578-603] | tile, TMEM, TIMG, TLUT, combiner, othermode, segment table (ptr_of uses segbase [C pm_gbi.c:361-365]). Content: pm_pdl_heap_gen [C 780] |
| 3 | .data render mode and combine lists (SolidCombineModes ... Gfx_RM3_CLOUD, +0x200) [C 1522, 1580-1585] | othermode and combiner, about 3 to 5 ops [I] | combiner, othermode. Content static |
| 4 | pm_world_entity_heap, 0x18000 | entity models, geometry and G_MTX [I] | matrix stack, vertex data, colours. Content: pm_pdl_entity_gen [C 781] |
| 5 | heap_generalHead, 0x180000 | model and mesh-animator lists [C 982-983] | same as 4. pm_pdl_genheap_gen [C 782] |
| 6 | heap_spriteHead, 0x100000 | sprite lists | same as 4. pm_pdl_sprite_gen [C 783] |
| 7 | none of the above | unknown [I] | unknown. heap gen 0 [C 784] |

Why the caches exclude classes:
- dlc (class 1) needs static vertex data in a known buffer. It caches geometry with a box per group [C pm_dlc.inc.c:2-7].
- pdl_try (2, 3) caches state-only lists. pdl_refuse_ops refuses VTX, TRI, MTX, G_DL, MOVEMEM, rects [C 748-760]. Matrix and vertex data are not in the key, so geometry cannot be cached there [I].
- pdl_try_m (4 to 7) accepts geometry. Its replay runs every op of the list again through pm_gbi_run (PSO and RUN both call pm_gbi_run) [C 1092-1104]. It saves 0 ops and adds dls_hash, gbi_state_get and a full state hash per call [C 1079-1081]. It is not a candidate [I].

## 2. Ops per class
- No per-class op count in the yard data [M diag_dl dl2: only G_DL per class: class 1 68, class 7 32.5, others about 138 per frame].
- The counter exists. Switch `dlsrc` counts interpreter ops per class of the running list, split geometry / flow / state [C pm_gbi.c:2499, 1541-1547]. Report line `dlsrc` every 300 drawn frames [C 2061, 1586-1593]. Tool: 3ds/tools/dlsrc_report.py. No patch is needed.
- Limit: when pdl_tpl_drive runs, each nested op goes through `pm_gbi_run(pc, 8u)` [C pm_dlc.inc.c:1368] and line pm_gbi.c:2456 sets the class range to that 8-byte op. All nested ops then count as class 0 [C]. Today pdl_tpl_drive is off (section 0), so the counts are valid. Measure in row R1, before row R3.
- Best candidate: class 2 [I]. Basis: texture lists cost about 30 us each, 2.7 of 8.7 ms gbi in Azahar town [M HANDOFF_2026-10-22.md:124]. The texture-class batch closes cost 1.75 ms per frame [M]. pdl replay binds tile 0 from the cached slot, without TMEM emulation [C pm_dlc.inc.c:813-830, 926-927]. R1 confirms or rejects this. If R1 shows class 2 state ops < 300 per frame, stop and report.

## 3. Class 2 change
Path: the existing pdl_try. Two defects block it. R2 fixes the key. R3 fixes the stuck compile.

Defect A, key [C]: the key is the hash of the read set plus the combiner [C pm_dlc.inc.c:868, 908]. The replay applies the full exit state, every field [C 915, pm_gbi.c:1689-1720]. A field that the list neither reads nor writes (geom, prim, env, scissor, lights, cur_tile, cimg) gets its value from the compile frame. That changes the picture [I].
Defect B, segment writes [C]: pdl_op_rw maps every G_MOVEWORD to a FOG write [C 573], so pdl_refuse_ops accepts G_MW_SEGMENT. segbase is not in GbiState [C pm_gbi.c:270, 1605-1645]. A replay skips the segment write [I].

Cache key inputs (after R2): list address, heap gen (pdl_heap_gen), full GbiState hash, segbase table hash.
Recorded at compile (no change): nops, exit GbiState, combiner words, texture slot and gen, PSO id [C 840-862].
Replay (no change): gbi_state_apply(exit), combiner, sync_combine, PSO id, lazy tile 0 bind [C 915-929].
Invalidation (no change): heap gen differs, or key differs: recompile [C 909-913]. Texture slot gen differs: texbad, no bind from the slot [C 824].
Why the picture stays the same [I]: a state-only list is a pure function of (entry state, segment table, list bytes). The new key holds all three. The list draws nothing (refused ops). The texture side effect is the slot bind, guarded by the slot gen. Matrix, vertex buffer and lights are not read by a state-only list. Lights are in GbiState anyway.

### R2 patch (pm_dlc.inc.c, about 6 lines)
1. Line 753, append to the condition: `|| (op == G_MOVEWORD && ((l[i].words.w0 >> 16) & 0xff) != G_MW_FOG)`.
2. After line 838 (`static int pdl_comp_nopush;`), add:
```c
/* pdl key: the replay applies the full exit state, so the full entry state and the segment table must match */
static u32 pdl_key(const GbiState* s) { return gbi_state_hash(s) ^ gbi_state_fnv(segbase, sizeof segbase) * 31u; }
```
3. Line 868: replace `gbi_state_in_hash(&pdl_comp_gbi_st, pdl_readset_of(l) | GBIG_COMB)` with `pdl_key(&pdl_comp_gbi_st)`.
4. Line 908: replace `gbi_state_in_hash(&pdl_try_gbi_st, pdl_readset_of(l) | GBIG_COMB)` with `pdl_key(&pdl_try_gbi_st)`.
Expected effect: none in the picture or counters, because pdl_try is dead until R3 [I].

### R3 patch (pm_dlc.inc.c:1308-1313, pdl_tpl_gdl, about 9 lines)
Replace:
```c
    if (!pdl_compile_one && dlc_on && pdl_try(next)) {
        if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) *pc = (Gfx*)dlc_end;
        return 1;
    }
    if (pdl_compile_one && pdl_comp_sp < 0) pdl_comp_arm(*sp, ((w0 >> 16) & 0xff) == G_DL_NOPUSH);
    if (!pdl_compile_one && dlc_on && pdl_try_m(next)) {
```
with:
```c
    if (!pdl_compile_one && dlc_on && pdl_try(next)) {
        RS_ADD(RS_PDL, 1);
        if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) *pc = (Gfx*)dlc_end;
        return 1;
    }
    if (pdl_compile_one && pdl_comp_sp < 0) { /* this walk pops G_ENDDL itself: run the list in the interpreter, whose G_ENDDL ends the compile */
        pdl_comp_arm(0, 1);
        pm_gbi_run(next, 0);
        if (((w0 >> 16) & 0xff) == G_DL_NOPUSH) *pc = (Gfx*)dlc_end;
        return 1;
    }
    if (!pdl_compile_one && dlc_on && pdl_try_m(next)) {
        RS_ADD(RS_PDLM, 1);
```
Why it ends the compile [C]: pm_gbi_run(next, 0) runs `next` with sp 0 and no end [C pm_gbi.c:2469]. At its G_ENDDL, pdl_comp_done with nopush = 1 calls pdl_compile_finish and sets pc to dlc_end [C pm_dlc.inc.c:889-892]. The next op is G_ENDDL at sp 0: return. dlc_end as pc is an existing pattern [C pm_gbi.c:2542]. A class 2 list holds no G_DL (refused [C 753]), so no nested push.
Side effect [I]: pdl_tpl_drive and pdl_try_m run again. pdl_tpl_drive walks nested ops one at a time through pm_gbi_run [C 1368]. Each call bumps gv_epoch [C pm_gbi.c:2450], which can stop triangle appends to an open batch [C pm_gbi.c:1501]. pdl_try_m adds probe cost with 0 ops saved (section 1). The walk time can rise. R3 measures this. It does not fix it.

## 4. Test plan (PM_ROW=<row> in every shell, budget per CLAUDE.md rule 5)
Run: `$env:PM_LANE='dl'; $env:PM_ROW='<row>'; powershell -File 3ds/run_azahar.ps1 -Timeout 90 -LockWait 180 -Debug "fast preset=yard phase <switches>"` [M diag_dl Commands].
- R1: switches `dlsrc`. Read the `dlsrc` line: ops per class / n frames, per kind. Read `dlsrc2` vtx and tri per class. Write the table to 3ds/logs/perf/status_R1.txt. No code. No picture needed (counter row, diagnostic budget 3).
- R2: yard with `phase`. Expect `pdl f..` hit 0, comp 0 (unchanged) and walk unchanged within noise. statehash toad and battle equal to the pre-R2 binary. golden.py toad yard battle: 0.00.
- R3: yard with `phase`. Expected change:
  - `pdl f..` line: hit > 0 per frame, about the class 2 + class 3 G_DL count from R1. comp near 0 after warm-up. texbad near 0.
  - `taskpl`: pdl field > 0 (new RS_PDL count in the tpl path). ops drops by about the class 2 + 3 state ops from R1.
  - `dlcsplit`: interpreter ops below 3223 per frame. dlc hit stays 68.
  - Walk below 10.06 ms. If walk rises: revert R3. Next step is a read-only diagnosis of pdl_tpl_drive and pdl_try_m cost (section 3 side effect). Do not start a code agent.
  - Picture: the yard shot must show the textured map. statehash toad and battle equal to R2. golden.py toad yard battle: 0.00.
- Scenes in golden.py: toad (preset=toad f251), yard (preset=yard f249), battle (preset=toad battle f300) [C 3ds/tools/golden.py:31-33].

## 5. Rows (each at most 100 changed lines)
| Row | Change | Lines | Gate to next row |
|---|---|---|---|
| R1 | measure ops per class with `dlsrc` | 0 | class 2 state ops >= 300 per frame |
| R2 | pdl key = full state + segbase, refuse non-FOG MOVEWORD | about 6 | statehash and golden equal |
| R3 | end the compile in pdl_tpl_gdl, RS_PDL/RS_PDLM counts | about 9 | pdl hit > 0, ops down, walk not up, picture equal |
| R4 (only if R3 hits but probe cost shows) | word-wise FNV in gbi_state_fnv (pm_gbi.c:1680-1685) | about 4 | walk down vs R3 |
Open items [I]: pdl_try_m saves 0 ops (delete it or make it skip ops; needs its own spec). pdl_tpl_drive one-op walk (pm_dlc.inc.c:1368). dlsrc class attribution under pdl_tpl_drive (pm_gbi.c:2454-2457).
