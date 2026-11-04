# agent_nds1: NDS1 phase A handoff (native draw, no collisions)

Plan: `C:\Users\Andrew\.claude\plans\you-are-not-an-glittery-nautilus.md`. Design: `docs/handoff/design_native_draw.md` (in the main checkout, untracked there). Worktree: `C:\Users\Andrew\Documents\GitHub\papermario-3ds-port-nds1`, branch `nds1` from `native`. Build lane `nds1`. Status file: `3ds/logs/perf/status_NDS1.txt`.

## State
- Phase A code: new files only (`3ds/include/pm_native.h`, `3ds/src/pm_native.c`, `3ds/tools/nat_host_test.c`, `3ds/tools/nat_host_test.sh`). `3ds/CMakeLists.txt` gets one line (`src/pm_native.c`). No edit to pm_gpu.c, pm_gbi.c, pm_tex.c, pm_dlc.inc.c, pm_warp.c, model.c. Those files are unchanged on `nds1`.
- Lane build `nds1` passes (`PM_LANE=nds1 sh 3ds/build.sh`). pm_native.o: text 2109 B. nat_emit 636 B, nat_order 268 B, nat_tmpl_add 748 B, nat_eye_copy 132 B, nat_layout_set 76 B. The linker drops nat_* from the ELF (no caller yet), so the size is read from the object.
- Host tests pass (`sh 3ds/tools/nat_host_test.sh`, 0.7 s): record size 16 B, nine census pairs build (pm_comb rc 0), order equals the reference sort in 200 random sets, emit equals the reference, eye-2 copy-plus-patch equals direct eye-2 emit (960 words patched = 60 draws x 16), no differing word outside the MTX patches.
- Azahar starts used on NDS1: 2 of 8 (census_yard, census_toad). Both passed the log checks.
- Lane build copy: the worktree lacked ignored generated sources. They were copied from the main checkout (`src/`, `include/`, `3ds/src/`, `3ds/include/`, `3ds/ld`, `3ds/shaders`, `3ds/stub_include`, CMake inputs, `assets/us`, `ver/us/build`). None of them is committed.
- Temporary diagnostic (`cenc` print in pm_gpu.c and `pm_dbg_mode` in pm_gbi.c) was reverted. It is not in the tree.

## Step 1: combine pairs and mode tuples (Azahar, census frame 200)
Source of the numbers: `build3ds/lane-nds1/last_log.txt`, lines `cenc f200`. One line per flush batch. The words are the SETCOMBINE state at the flush (not the effective state of a 2D rect batch, which uses rect_ec).

| Map | Batches (census) | Distinct combine pairs | Distinct tuples (cw + mode + flags) |
|---|---|---|---|
| kmr_20 yard (census_yard) | 46 | 7 | 19 |
| kmr_10 Toad Town (census_toad) | 26 | 8 | 15 |
| union | 72 | 9 | 28 |

Nine combine pairs (cw0 cw1):
```
fc121824 ff33ffff   fcffffff fffcf87c   fc127e24 fffff3f9
fc26a004 1f1093ff   fcffffff fffcf279   fcffb3ff ff2fffff
fcffffff fffe793c   fc20a204 ff13ffff   fc119623 ff2fffff
```
The most used pair is fc121824/ff33ffff (27 of the 72 batch lines). The mode words are geom (0x002004xx, 0x002200xx family), oh, ol and the flags z, graw, decal, blend, atest. Full list: the `cenc` lines in the lane log.

Result: only nine pairs and 28 tuples matter for the template table on these two maps. The table needs 64 entries (6-bit id) for all Toad Town and yard flags, and the plan said 15 to 30 templates. The 28-tuple count is the union of two maps, not the whole game.

## Step 2: code in pm_native.h / pm_native.c
- `NatRec` (16 B): tmpl u8, tex0 u8, tex1 u8, flags u8, mtx u16, nidx u16, idx_off u32, vtx_base u32. tex1 is in the record, so unit 1 is covered.
- Template table `nat_tmpl[64]`: key (cw0, cw1, geom, oh, ol, z, decal, blend, atest). `ordered` = blend or decal or z off. Alpha test is not ordered. `blk[96]` is the state block, filled in phase B.
- Draw block `nat_drw[32]` and patch table (`NatPatch`: TEX0, TEX1, IDX, NIDX, MTX). The layout is set with `nat_layout_set`. Word offsets are not yet taken from a captured record.
- Matrix pool `nat_mtx[2][96][16]` (encoded PICA words per eye, 12 KB) and `nat_tex_word[128]`, `nat_idx_base`.
- `nat_order`: runs of unordered records are sorted with a stable LSD counting sort (tex0, then tmpl). Ordered records keep their index and break the runs.
- `nat_emit`: copies the 96-word block once per template change, then copies the 32-word draw block per record and patches it. It logs every MTX patch (`NatMtxLog`).
- `nat_eye_copy`: memcpy the eye 1 span, then write the eye 2 palette words at the logged positions.
- Data: NatTmpl is 416 B (blk at offset 32 for the 32-byte alignment). The design said 384 B. The table is 26 KB. The per-draw data is records (16 B), one block (384 B) per bucket, draw block (128 B) and 64 B per draw matrix.

## Step 3: host tests (3ds/tools/nat_host_test.c)
Gates: 1 record size and offsets, 2 template plan equals a direct pm_comb_build for the nine pairs (and dedupe by key), 3 order equals the reference, 4 emit and eye copy. Not covered here: the matrix product order (already gate 2 of ndt_host_test.c) and the fp24 seam (gate 3 of ndt_host_test.c).

## Step 4: integration hooks for phase B (NOT applied; line numbers at nds1 HEAD 02e901772 plus this branch)
1. `src/model.c:1406`, `1515`, `2073`: G_PM_MODEL group header and close. Write one NatRec per static plain model (`nat_rec_put`, TARGET_3DS) here, instead of the walk.
2. `3ds/src/pm_gbi.c:3714` (mg_run, called at 3844): the group walk. For a native model, skip the walk and use the record. Delete the walk in the same commit (slice 6 for the rest).
3. `3ds/src/pm_gbi.c:390`: comb_w0 and comb_w1 are static. Phase B needs an accessor (or the record writer reads them) to call `nat_tmpl_add`.
4. `3ds/src/pm_gpu.c:1064`: the census site is the flush of the template. Capture the 96-word block here (pm_gpu_rec_begin / end, pm_gpu.c:2449 and the capture code) and store it in `nat_tmpl[id].blk`.
5. `3ds/src/pm_gpu.c:834`: `Mtx_Multiply(&pcomp[gm], &gv_post, &game)`. Encode the result into `nat_mtx[0][slot]` once per frame. Same product, same order (seam rule 1).
6. `3ds/src/pm_gpu.c:229`: SEAM product `Mtx_Multiply(&shader_3d, post, &game)`. Eye 2 uses the same order with the eye 2 post matrix.
7. `3ds/src/pm_gbi.c:456` (upload_mvp): eye shift. Phase B moves the eye choice into the matrix pool, not into the walk.
8. `3ds/src/pm_nusys.c:80-81` (gbi_task): the two walks. New: one build of records, `nat_order`, `nat_emit(eye 0)`, and when stereo `nat_eye_copy(eye 1)` plus target register patch (0x110 to 0x11E, pm_gpu_eye at `pm_gpu.c:1747`).
9. `3ds/ld/3dsx.ld.in:90` (`.data.pmhot`, data) and the CF4 text span at line 31: add the nat_ hot functions for nat_emit, nat_order, nat_pass, nat_eye_copy. Not verified in this phase.

## Open items
- The template block (96 words) is not captured. Phase A stores the key and the plan only.
- Draw block word offsets and the patch values (TEX0 address word 0x85, TEX1 0x95, index 0x227, draw count) are not read from a captured record.
- `nat_tmpl_add` calls `pm_comb_build(cw0, cw1, 0, 0)`. The real call (pm_gbi.c:1070) passes `two` and `use_t1`. Phase B must pass the same values, or a template will not equal the old plan.
- The census words are SETCOMBINE state at the flush. 2D rect batches use rect_ec. Phase B needs the effective combine for those.
- Eye 2 target register patch (0x110 to 0x11E) is not in the code.
- Dynamic families (sprites, HUD, text, effects) are not covered in phase A.
- NatTmpl is 416 B, not 384 B (see step 2).
- The GPU busy-time probe (design section 6) has not been built.

## Next steps (phase B, after the other branches merge)
1. Capture one template block per census tuple with the rec capture path. Check the block against the old path (census equal).
2. Wire hooks 1 to 4 (model.c and pm_gbi.c first), census for Toad Town and yard, then reference scenes.
3. Wire hooks 5 to 8 (matrix pool and eye copy), seam count check, then the console delta.

## Commands
- Host tests: `sh 3ds/tools/nat_host_test.sh`
- Lane build: `PM_LANE=nds1 PM_ROW=NDS1 sh 3ds/build.sh` (run `python 3ds/tools/gen_map_restore.py` first, then `git checkout -- 3ds/src/map_restore_table.c`)
- Census: `PM_ROW=NDS1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_yard -Row NDS1 -Build nds1` (and `census_toad`)

## Follow-up (coordinator request, phase A part 2)
- Item 3 done: `nat_tmpl_add` takes `t1only` and calls `pm_comb_build(cw0, cw1, two, t1only)` with `two = ((oh >> 20) & 3) == 1` (pm_native.c `nat_pm_comb`), the same rule as pm_gbi.c:504 and the call at :1070. `t1only` is part of the template key. Host gate 2 still passes.
- Item 4 (eye-2 target registers): the target is not a word in the record span. `pm_gpu_eye` (pm_gpu.c:1747) calls `C3D_FrameDrawOn(cur_target)`, which writes the framebuffer registers. The record path marks 0x112 to 0x117 and 0x11C to 0x11E as foreign (pm_gpu.c:2528, rec_foreign). So the eye 2 span copy needs no register patch. Phase B calls `C3D_FrameDrawOn(targetR)` before it plays the eye 2 span, as `pm_gpu_eye` does now.
- Item 5 (rect batch): a 2D rect batch does not use `comb_w0`/`comb_w1` as its state. flush_impl sets `combine = rect_ec`, `rect_z = rect_zb` and `rect_2d = 1` for the batch (pm_gpu.c:625 to 628). The template key for a rect batch must come from `rect_ec` and `rect_zb`, not the SETCOMBINE words.
- Items 1 and 2 (capture of real PICA words, draw block offsets, patch values) are NOT done. No switch prints captured words. `rec_done` (pm_gbi.c:3650) holds the capture (`w`, `n`) after `pm_gpu_rec_end`. A dump needs a call from that function, which is an old file. Phase A rules forbid the edit. Choose one: (a) a temporary, uncommitted hook in `rec_done` that calls a dump function in pm_native.c, reverted before commit (the same method as the step 1 `cenc` dump); or (b) stop until the other branches merge.

## Phase B step 1 result (2026-11-04, census_toad, Azahar start 3 of 8)
Quantity: word layout of two captured static plain model records (rec_done, temp dump, reverted, not committed). Fixture: `3ds/tools/nat_fixture_toad.txt`. Check: `python -I 3ds/tools/nat_layout_check.py 3ds/tools/nat_fixture_toad.txt` (PASS).

Measured:
- One captured record is 278 words (75 PICA commands) for slot 25, and 272 words (72 commands) for slot 15. Both on census_toad.
- Commands that match between the two models: 0x047, 0x048, 0x065, 0x229 to 0x25E (state), 0x04F, 0x064, 0x06F, 0x0D0 to 0x0F8, 0x100 to 0x130, 0x203 (36 words), and the draw trigger 0x22F. These are template words.
- Words that differ, by register: 0x081 (unit 0 texture block, 4 words: dims, param, border, address), 0x2C1 (palette uniform, 16 words, c = 15), 0x227 (index address), 0x228 (index count).
- Words that differ because the TEV key differs (template key, not per draw): 0x0C0, 0x0C8, 0x0E0, 0x0E1, 0x0FD. Slot 25 has three extra TEV commands (0x0E0, 0x0FD, 0x0E1), so the two templates have different combine pairs.
- Unit 1 block (0x091) is equal in the two records (same unit 1 texture here).

Consequence for phase A (the design is wrong here):
- NAT_TMPL_WORDS = 96 cannot hold the template. The state is about 240 to 260 words per record. Use the record size as the template size (about 1.1 KB per template, 64 templates = about 72 KB in RAM).
- A per-draw block of 32 contiguous words is wrong. The per-draw words sit at different offsets of the record. Per draw the emit must write the per-draw command group only: palette 0x2C1 (16 words), unit 0 texture block 0x081 (4 words), index address and count 0x227, 0x228 (2 words), and the draw trigger (0x22F). About 30 words per draw. Template state is not repeated because the GPU keeps register state between draws.
- The palette offset and the texture offset are per template (they move with the TEV command count), so the patch offsets are stored per template.

Not done in this run: the host test on the real bytes (pm_native.c and nat_host_test.c still use the 96 and 32 word layout), hooks 1 to 9, census equal, region diff, old path deletion. Each depends on the layout decision above.

Decision needed (not taken here):
1. Adopt the record-size template plus per-draw command group layout above (recommended). Phase A code changes to NAT_TMPL_WORDS about 256 and a per-draw group, with the host test on the fixture bytes.
2. Stop phase B until the layout is reviewed.

Azahar starts used on NDS1: 3 of 8 (census_yard, census_toad, census_toad with the dump).

## Phase B layout update (2026-11-04, coordinator decision: recommended layout)
Code: `3ds/include/pm_native.h`, `3ds/src/pm_native.c`, `3ds/tools/nat_host_test.c` (round trip on the fixture, PASS).
- Template = captured record minus the per-draw commands. Fixture: 240 words (record A, 278 minus 38) and 234 words (record B, 272 minus 38). Capacity 256 words, 1216 B per NatTmpl.
- Per-draw group, 38 words, emitted after the template for each draw: 0x2C0 index (0x5a), 0x2C1 palette (16 data words from `nat_mtx[eye][slot]`), 0x081 unit 0 block (4 words from `nat_tex[tex0]`), 0x091 unit 1 block (`nat_tex[tex1]`), 0x227 index address, 0x228 index count, 0x22F trigger.
- The record has two 0x2C1 writes of 16 words. The first is the same in both models and stays in the template. The last one (with its 0x2C0 index before it) is the per-draw palette. This is a finding of the layout, not a guess.
- Host test gates: record size, template key and plan, order, layout round trip (each record's emitted commands equal its captured commands as a multiset), eye copy (32 palette words differ, all in the patches).

Defaults chosen (coordinator said pick and note):
1. Commands in the template keep their order. The group is placed after the whole template, so trailing state (0x245, 0x229, 0x253, 0x231, 0x25E after the trigger in the capture) now runs before the trigger. Not verified on hardware. Check with a shot before any console run.
2. Template copy happens once per template change in the emit order. Skip-the-copy rule for repeated templates is in place (`cur`).
3. The texture block for unit 1 is always written (6 words per draw), even when the template does not bind unit 1.

Not done in this run (steps 2 to 4):
- Hooks 1 to 9 (model.c, pm_gbi.c, pm_gpu.c, pm_nusys.c): not started. The per-frame inputs (palette from the matrix product, texture words, index arena) have UC3 machinery (`pm_gpu_rec_pt_copy`, `pm_gpu_rec_idx`, `pm_tex_cur_slots`) that a hook can reuse. Not wired.
- Old path deletion for these models: not done (nothing replaced yet).
- Gates: census equal on census_yard and census_toad, region diff, fsplit `gbi` and `dl`, hot loop size from the map: not run.
- Lane build of this code: not rebuilt. pm_native.c compiles in CMake, but nothing calls it yet.

## Retirement handoff (step 2 not started, 2026-11-04)
State: branch nds1 at 998693eb8. Tree clean except untracked `3ds/src/map_restore_table.c.bak` (never commit). Lane build `nds1` built before the layout change (pm_native not called yet). Azahar starts on NDS1: 3 of 8 (census_yard, census_toad, census_toad dump). No budget reset needed.

Next steps, in order:
1. Capture to template: in `rec_done` (pm_gbi.c, after `pm_gpu_rec_end`, when `r == 1 && whole`) call `nat_tmpl_strip(id, w, n)`. Key id from the existing rk[] slot or from the flush key (cw0, cw1, mode words). Store `rec_g0`/`comb_w0`/`comb_w1` (pm_gbi.c:390) for the key. One template per key, at most 64.
2. Per-draw inputs for a record: palette words from the matrix product (pm_gpu.c, pcomp at the flush; `pm_gpu_rec_pt_copy` gives patch positions), texture words `nat_tex[h]` from `pm_tex_cur_slots` and the slot address, index address and count from `pm_gpu_rec_idx` (`io`, `ic`, arena base). The UC3 record machinery already computes all of these; reuse, do not write a second encoder.
3. Hooks (plan numbering): 1 `model.c` G_PM_MODEL header (≈line 1515 and 2073) writes a NatRec; 2 `pm_gbi.c` `mg_run` (≈3756) skips the walk for native models; 3 `comb_w0/w1` accessor; 4 `pm_gpu.c` census site (≈1064) gets the template words; 5 `pm_gpu.c` palette product (≈834 and 229) writes `nat_mtx[0]`; 6 SEAM product order unchanged; 7 `pm_gbi.c` upload_mvp (≈456) eye shift moves to `nat_mtx`; 8 `pm_nusys.c` gbi_task (≈80) builds once, `nat_emit` eye 0, `nat_eye_copy` eye 1 when stereo; 9 `3ds/ld/3dsx.ld.in` hot span for nat_emit, nat_order.
4. Old path for these models is deleted in the same row as hook 2.
5. Gate: census equal (`cen f200` multiset) on census_yard and census_toad with `azt.ps1 <scenario> -Row NDS1 -Build nds1`, justification line first in `3ds/logs/perf/status_NDS1.txt`. Region diff and fsplit after census passes.

Open risk: trailing state after the trigger now runs before it (default 1). The first picture must show it.

## Step 2 status (code agent, 2026-11-04): no code change, design gaps found

No source change. No Azahar start. Tree at 5f2e56011 plus this section.

Facts from the read (line numbers at 5f2e56011):
1. Capture key. rec_try (pm_gbi.c ~3597) stores rec_g0 and the slot texture words. comb_w0/comb_w1 (pm_gbi.c:390) and the mode words are not in RecKey. Step 2 must copy them at capture start, next to rec_g0, or the template key is wrong.
2. Palette words. The 0x2C1 body data is the 16 words of pcomp[gm] (pm_gpu.c:834, uploaded at 987 and 1009 with C3D_FVUnifMtx4x4). So nat_mtx[0][gm] must hold pcomp[gm], and gm must be below NAT_MTX_MAX (96). pal_tab (pm_gpu.c:2194, 2273) sets the table per frame. The gm range is not checked yet. Check it before the slice.
3. Replay bypasses the census. pm_gpu_rec_play (pm_gpu.c:2555) calls GPUCMD_AddRawCommands and never calls cen_add. cen_add has one call site (pm_gpu.c:1064, flush_draw). So census_yard and census_toad cannot show the replayed draws as they are. The census gate needs a census line for the raw path (per draw: template id, tex0, tex1, triangle count) before the gate can compare the old and new multisets.
4. Replay refuses on a moving model. rec_play (pm_gbi.c ~3750) requires dlc_gmi() == k->gmc. Any moving model fails that check, so the palette is the only per-draw input that breaks replay. pm_gpu_rec_play copies the captured body and patches nothing except the uniform dirty bits.
5. The eye loop is in gbi_task (pm_nusys.c:81), which runs pm_gbi_run twice when stereo. Step 2 changes that loop. It is not a local edit.

Decision needed before code:
A. Palette patch at play time (smallest): in pm_gpu_rec_play, copy pcomp[gm_now] (16 words) into the body at each palette data position from the patch table, and drop the gmc refusal for static records. This keeps the captured body and needs no NatRec. It does not meet the plan's per-draw NatRec design.
B. Full nat design as written in the plan (templates, NatRec, nat_order, nat_emit, nat_eye_copy). It needs items 1 to 5 solved first, plus a census line on the raw path. This is several code agents of work.
Recommendation: A first as a measured slice, then B. Ask the user (AskUserQuestion, rule 10).

## N3 and N4 step 1 (code agent, 2026-11-04)

### N3 (committed 003ef9bcc, gates passed)
- cen_add (pm_gpu.c) hashes prim only for combine 1, 5, 6, 7.
- rec_g0[11] (pm_gbi.c) holds prim when leg_mode is 1, 5, 6 or 7, else 0. RecKey grp is 12 words. ndt host test passes, nat host test passes.
- Census multisets (keys included) equal, replay on and off: census_toad (26 batches, 338 tris, 23 distinct, over 0) and census_yard (46 batches, 510 tris, 39 distinct, over 0).
- Region diff yard_shot against yard_shot_rp (frames 200, 204): 0 px differ. Shots not yet read by a person.
- Known limit: leg_mode is set by sync_combine, which runs lazily. At rec_g0_set the value can be one draw behind. Replay of a record whose combine changes mid-group is not measured.
- Scenarios yard_shot and yard_shot_rp added (unverified, need -Try), catalog rows added.

### N4 step 1 (capture to template, log only, not committed as final: see commit message)
- rec_done (pm_gbi.c) calls nat_tmpl_strip on each stored record, id (slot-1)&63, log line `N4 strip slot n nd rc`. No draw uses the template.
- Measured on census_toad_rp (start 1 of the NDS1 budget reset, one start used since N3): 13 captured records. 12 single-draw records split with rc 234 (n 272 words, template 234 words). 1 record (slot 36, n 376, nd 2) refused with rc -1 (two draws in one record).
- Build: pm_native.c compiles in the lane. The redefinition warnings in the build log come from include/PR headers, not from pm_native.h.

### Wired now
- Capture of templates from records (log only).
- Not wired: hooks 1 to 9 of the plan (step 4 above), NatRec writer, nat_emit call, nat_eye_copy, nat_mtx fill, old path deletion.

### Next steps for N4 (in order)
1. Split or refuse multi-draw records. Decide with the user (AskUserQuestion): refuse keeps the old path for slot 36 type records; split needs one NatRec per draw.
2. Template key from state, not defaults: nat_tmpl_add needs geom, oh, ol, z, decal, blend, atest, t1only at capture time. Add an accessor in pm_gbi.c (rec_g0 has cw0, cw1, oh, ol; z, decal, blend, atest come from the mode words and the flags, see pm_gpu.c flush_impl).
3. Per-draw inputs: palette from pcomp[gm] (nat_mtx), tex words (nat_tex from pm_tex_cur_slots), index address and count (pm_gpu_rec_idx io and ic).
4. Hook mg_run (pm_gbi.c, near the G_PM_MODEL walk) for a native model, then gbi_task (pm_nusys.c:80-81) with nat_order, nat_emit eye 0, nat_eye_copy eye 1 plus C3D_FrameDrawOn(targetR) before the eye 2 span.
5. Gates after the wiring: census equal on census_toad and census_yard (replay on versus the census of the old path), region diff of yard_shot against a native shot, hot loop size from build3ds/lane-nds1/pm_3ds.map (under 8 KB), host tests.
6. Delete the old walk for these models in the same commit as hook 2.

### Budget
- NDS1 budget after the reset: 7 of 8 starts used (N3 six, N4 step 1 one). One start left.

### Open items
- Trailing state after the trigger runs before it under the group layout (plan default 1). Needs a shot before any console run.
- Eye 2 target registers: no patch needed (see Follow-up item 4).

## N3b and handoff (code agent, 2026-11-04, coordinator decisions applied)
- Decision applied: multi-draw records are SPLIT (one template, one per-draw group per draw). Not built yet.
- N3b (pm_gbi.c rec_g0_set): sync_combine() runs before leg_mode is read for rec_g0[11]. Gate: census_toad_rp equals census_toad (26 batches, 338 tris, 23 distinct, over 0), keys equal. Starts used: 2 of 8 after reset.
- NDS1 starts used after reset: 2 of 8 (N3b). Six left.

### N4 remaining, in order (none of these is wired)
1. Template key from state: rec_done needs geom, oh, ol, z, decal, blend, atest, t1only. Read them from the record start (rec_g0[9], [10] hold oh, ol; geom, z, decal, blend, atest from pm_gpu flush state, needs one accessor in pm_gpu.c).
2. Split: for nd > 1, one per-draw group per draw from the record patch and index tables (pm_gpu_rec_idx io/ic). Template emitted once.
3. NatRec writer: one NatRec per draw at rec_done (tmpl, tex0, tex1, mtx, nidx, idx_off).
4. Per-draw inputs: palette nat_mtx[eye][slot] from pcomp[gm] (pm_gpu.c flush, product at pm_gpu.c:834), tex words nat_tex from pm_tex_cur_slots, index address io/ic.
5. Hook 2: mg_run (pm_gbi.c, G_PM_MODEL walk) skips the walk for native models; delete that walk in the same commit.
6. Hook 8: gbi_task (pm_nusys.c:80-81): nat_order, nat_emit eye 0; stereo: nat_eye_copy eye 1 after C3D_FrameDrawOn(targetR).
7. Hook 9: linker hot span for nat_emit, nat_order (3ds/ld/3dsx.ld.in).
8. Gates: census equal (census_toad, census_yard, replay path), region diff yard_shot vs native shot, host tests (sh 3ds/tools/nat_host_test.sh, sh 3ds/tools/ndt_host_test.sh), hot loop size under 8 KB from build3ds/lane-nds1/pm_3ds.map.

## N4 step 2 and handoff (code agent, 2026-11-04)
- Commit 330c6c760 (N4 step 1): template key from entry geom (rec_gin), omode words, combine words, render mode (pm_gpu_rec_rmode), use_t1. nat_tmpl_add then nat_tmpl_strip fill the template in rec_done. census_toad_rp: 26 batches, 338 tris, 23 distinct, over 0 (equal to census_toad).
- Step 2 (this commit): nat_tmpl_strip (pm_native.c) takes the LAST nd palettes as draws (nd = count of 0x227). The first 0x2C1 of a capture stays in blk. Per-draw kinds 0x081/0x091 may be absent in later draws. The template cap NAT_TMPL_WORDS is 320 (pm_native.h). Host test: all pass (template 240 and 234 words on the fixture).
- Result on census_toad_rp (N4 starts 4 to 7, lane nds1): 12 of 13 captured records give rc 234. Slot 36 (n 376, nd 2) gives rc -3.
- Cause of rc -3: slot 36 has 66 state words between draw 0 and draw 1. 66 = one command with 64 data words (header c 63). The per-draw group (38 words) does not carry them. So the split decision does not cover this record as written.
- OPEN DECISION (user): (a) the 64-word block is per-draw state: extend the group (NAT_GRP_WORDS) to carry it, or (b) the record stays on the old path (rc -3, rec_ref). Ask with AskUserQuestion before N4 step 3. Recommended: (b) for now.
- Budget: NDS1 reset once by the code agent (run_azahar.ps1 -ResetBudget NDS1, user instruction). Starts used after reset: 4 (N4 starts 4 to 7; N4 start 3 was void: build error, step 1 binary ran). The reset lost the earlier count of 8 starts. Recorded in status_NDS1.txt.
- Not done: N4 steps 3 to 7 (NatRec writer at rec_done, nat_emit replay for plain static models, hooks 2, 8, 9, eye 2 copy in gbi_task, delete the old path, region diff, hot loop size, default switch). Nothing in the draw path changed; rec_play still runs.
- Commands: lane build `PM_LANE=nds1 PM_ROW=NDS1 sh 3ds/build.sh` (gen_map_restore.py first, then `git checkout -- 3ds/src/map_restore_table.c`). Census `PM_ROW=NDS1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_toad_rp -Row NDS1 -Build nds1 -Try`. Host test `sh 3ds/tools/nat_host_test.sh`. Strip lines: `grep "N4 strip" build3ds/lane-nds1/last_log.txt`.
- Next steps: (1) user decision on slot 36; (2) NatRec writer and per-draw inputs (step 2b, pm_gpu_rec_pt_copy, pm_gpu_rec_idx, pm_tex_cur_slots); (3) nat_emit replay for plain static models; (4) hooks 2, 8, 9; (5) eye 2 copy; (6) delete old path; (7) gates.

## N4 2b done and handoff (code agent, 2026-11-04, commit 34ef68056)
- nat_rec_write (pm_native.c) fills one NatRec per draw at rec_done (pm_gbi.c), after the census tuples: record words = n + npt + 8 * ncen (4 words per draw). mtx = rec_gm of the window (gm cap). tex0/tex1 = rec_g0[6] (slot + 1). idx_off = 0x227 p0 minus nat_idx_base = phys of the index arena (pm_gpu_rec_ia_phys). nidx = 0x228 p0.
- nat_tex_set fills nat_tex from the template prototype (grp 22 and 28) per capture generation (memset in rec_reset). A conflict refuses the record (rec_ref).
- A record with rc <= 0 has zero NatRec words. The consumer must check the rc (add a flag to RecKey before use).
- Gates: nat_host_test.sh all pass. census_toad_rp: 26 batches, 338 tris, 23 distinct, over 0 (same as before). Lane nds1 build passes. gen_map_restore.py was NOT run before this build (rule 9 deviation).
- Correction to the orchestrator decision D2: pcomp_eye does not exist. The eye shift is applied in the projection (pm_gbi.c:459 to 464), not in pcomp. Eye 2 palette = pcomp[gm], the same product as eye 1. nat_mtx[1] = nat_mtx[0] is valid.
- Not done: nat_emit replay wiring (rec_play still runs), hooks 2/8/9, eye 2 copy, bucketing, old path delete, region diff, hot loop size, stereo shot, default switch. Azahar starts left for NDS1: 3 (this run used 1 of the 4 left).
- Next: wire nat_emit into rec_play for qualifying records (flag in RecKey), then region diff on toad, then stereo shot.

## N4 steps 1 and 2 (per-eye fill, nat replay branch), handoff (code agent, 2026-11-04, retiring)
- pm_gpu.c: pal_mul is the shared palette product (pcomp_calc uses it, same order). pm_gpu_pal_fill(m, gm, eye) writes nat_mtx[eye][gm]. pm_eye_idx is 1 for the right eye pass (pm_gpu_eye), 0 otherwise.
- pm_gbi.c: RecKey.nat (set when nat_rec_write succeeds). rec_play has a nat branch: palette fill from gm_tab[dlc_gmi()] for pm_eye_idx, NatRec mtx and nat_idx_base set, nat_emit, pm_gpu_rec_play with gm ~0. Non-nat records keep the old call. Log line "N4 nat replay" (first 4).
- Gates so far: host tests pass. nds1 lane build passes (gen_map_restore run before, file restored). census_toad_rp (recplay on): 26 / 338 / 23 / 0 (equal).
- OPEN: the nat branch did not run in census_toad_rp (no "N4 nat replay" line). 13 records have "N4 strip ... rc 234". Find which condition blocks: rec_play state/geom/tex checks, or k->nat unset (gmv == ~0 or nat_tex_set conflict, both give rec_ref). Count both with a log line before any picture gate.
- OPEN: no region diff yet. Needs a scenario with recplay and shots (census_toad_rp has no shot). Baseline = commit d44a10944 with the same scenario.
- Not done: hooks 2/8/9, eye 2 copy in gbi_task, bucketing, old path delete for qualifying records, hot loop size, stereo shot, default switch.
- Azahar budget NDS1 after the reset: 3 starts used of 8 (census2, census3 = 2; plus census 1 gate earlier after reset? counted as 2 here). Check status_NDS1.txt before the next start.

## N4 step 1 result (code agent, 2026-11-04, handoff)
- Counters: pm_gbi.c NW_* enum, nat_why_report (line "N4 why fNNN ..." and "grpw", "wr", "wr2" at cen f200). pm_native.c nat_wr_why[0..4] per refusal case of nat_rec_write.
- census_toad_rp, run 3 (-Try, scenario status unverified in azt_scenarios.json): 26 / 338 / 23 / 0 (same as census_toad).
- Why the nat branch never ran: no record got k->nat = 1 (b_nat 0). Build refusals: b_write 10 (nat_rec_write case 2, index offset), b_tex 2 (nat_tex_set conflict), b_tmpl 1 (rc -3, record stays non-nat, state 1).
- nat_rec_write case 2 sample: off-base = 0x68000390 - 0x20747D00, so the 0x227 index address is outside the window index copy (pm_gpu_rec_idx rewrite). Cause not known. Open.
- The one non-nat state 1 record: lookup 160 of 160 refused at the group compare. Words that differ: 5 (ctx+0x20 flags, 160 of 160, sample ff80019d vs ff80019f: bit 1), 6 (tex slot ids, 160 of 160, sample 000d000a vs 000d0012), 0 (118 of 160, not sampled), 3 (1 of 160).
- Census equality is trivial: rec_play never replayed, so the live path ran.
- Open (step 1 fix not done): (a) why 0x227 addresses fall outside the window index copy, (b) nat_tex_set conflict in 2 records, (c) group compare: words 5 and 6 are per-frame state. Idea, not done: for nat records only, skip word 6 (nat replay takes tex handles from NatRec tex0/tex1) and mask word 5 to bit 0. Needs a check that word 0 is also safe.
- Starts: 5 of 8 used for NDS1 (3 left). Shots: 0.
- Commands: python 3ds/tools/gen_map_restore.py; PM_LANE=nds1 PM_ROW=NDS1 sh 3ds/build.sh; git checkout -- 3ds/src/map_restore_table.c; PM_ROW=NDS1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_toad_rp -Row NDS1 -Build nds1 -Try. Log: build3ds/lane-nds1/last_log.txt.

---

# NDS1 handoff (code agent, map_gdl_walker E1 to E6)

State: branch nds1, worktree papermario-3ds-port-nds1. Pushes: none.

## Commits
- 7c82c3f45 E1: pm_gpu_rec_ia_phys returns the GPU index address (phys - 0x18000000, bit 31).
- bc43871e7 E2: nat_tex_id keys texture blocks by content (replaces nat_tex_set). Host test test_tex_id added.
- a7c40bec7 E3: DlcGrp.pidx (pool copy pointer per group), nat_gpu_addr(phys). Host test checks 0x20389D6E -> 0x88389D6E.

## Gates run (census_mac01 / census_toad, PM_ROW=E1..E3)
- E1 census_mac01: batches 64, tris 810, distinct 52. P3a write refused 0 (was every draw). P3a refuse tex 16.
- E2 census_mac01: batches 64, tris 810, distinct 53. P3a refuse tex 0. nat=1 draws (P3a tmpl draw lines) 52.
- E3 census_mac01: 64 / 810 / 53. E3 census_toad: 26 / 338 / 23. Both pass.
- Host test (sh 3ds/tools/nat_host_test.sh): all pass after E2 and E3.
- Open from E2 log: grp_eq 0 on 46 of 52 nat draws (index window differs from sd). grp_eq 1 count 0. Not diagnosed. Needs an investigation agent.

## Not done
- E4 (DLC2 group to NatRec builder). Open design points: the batch merge test in grange_batch_open (pm_gpu.c:2269) reads global g_* state (gni, g_pal, g_vp, ...). E4 must store vp, uvso, cmul, cadd, zmix, ct, fx per batch, or mirror the merge on a local list. Per-batch params come from ep_p after gv_params_calc (pm_dlc.inc.c:284) at dlc_seg time.
- E5, E6: depend on E4.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_LANE=nds1 PM_ROW=<row> sh 3ds/build.sh; restore 3ds/src/map_restore_table.c from a copy (never commit it).
- Census: PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_mac01 -Row <row> -Build nds1 -Try
- Host: sh 3ds/tools/nat_host_test.sh
- Budget: 3ds/run_azahar.ps1 -ResetBudget NDS1. Justification lines: 3ds/logs/perf/status_NDS1.txt.

## Next step
Opus plans E4 (state per batch and merge design). A code agent builds E4 and runs the census gate with a count log line.

## NDS1 handoff after E4a and E4b (map_e4.md section 5)

State: branch nds1. E4a and E4b committed. E5, E6 and the recplay removal are NOT started.

Commits:
- 381ad1329 E4a: pm_native.c nat_pool_put packed (no 32-byte round). nat_host_test.c bake check now asserts packed copies. Gate: nat_host_test all pass; census_mac01 64 / 810 PASS (distinct 53, noise).
- f2f01068c E4b: pm_dlc.inc.c nat_seg_add (per-draw NatKey, mirror of grange_batch_open), nat_key_reset per dlc_try, dlc_gr set in dlc_seg (2 = grange extended the open batch, -1 = culled). pm_native.c/.h nat_pool_off. Log NATB at frame 200 (groups, draws, mis).

E4b result (census_mac01, Azahar, 1 start, shot not needed: no draw change):
- census 64 / 810 PASS.
- NATB draws sum 44 over 90 DL lines, mis 0 (mirror agrees with grange on every static segment).
- Gap to 64: census batch count dlc_c.batch also adds an estimate (pm_dlc.inc.c dlc_static_group, ng + 1 - nbat per drawn group). The builder covers static segments only. The 64 gate in map_e4.md section 5 (E4b) does not compare like with like. Do not use it as the E4b gate.
- Host test for E4b (merge and gap cases) NOT written: nat_seg_add is static in pm_dlc.inc.c (one translation unit with pm_gbi.c) and cannot run on the host. The live mirror check (mis) replaces it.

Open for E5 (map_e4.md section 5, E5):
- Needs the template id per group (rec_done capture, pm_gbi.c ~3800 nat_tmpl_add / nat_tmpl_strip) and the replay of nat_emit output (pm_gbi.c ~3845, k->nat branch, pm_gpu_rec_play). Read those lines first.
- Stereo (yard_stereo_r) must match too.
- The 2-group merged draws (grange merge) need pool contiguity: E4a made it possible. Gap count not yet measured (E4a contiguity check not added).
- Gates: census_mac01 64/810/52 (distinct), census_toad 26/338/23, shot diff on build3ds/azt/shots/mac01t frames 500 and 504, census_toad_shot 0 px via 3ds/tools/region_diff.py.
- After the gates pass: remove recplay, nat_rec_write nat branch, RecKey.nat, rec_play nat branch. Keep nat_tmpl_strip and the template table.

Commands:
- Gen: /c/Python314/python 3ds/tools/gen_map_restore.py
- Build: PM_LANE=nds1 PM_ROW=NDS1 sh 3ds/build.sh ; then git checkout -- 3ds/src/map_restore_table.c
- Census: PM_ROW=NDS1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_mac01 -Row NDS1 -Build nds1 -Try
- Host: sh 3ds/tools/nat_host_test.sh ; sh 3ds/tools/ndt_host_test.sh
- Log: build3ds/lane-nds1/last_log.txt
- Azahar starts used on NDS1: 2 of 8 (reset with run_azahar.ps1 -ResetBudget NDS1).

Notes:
- $S (scratchpad) is not set between Bash calls. Set it in each call.
- The NATB sum check is `awk '{d+=$8; m+=$10} END {print d, m}'` on the NATB lines.
