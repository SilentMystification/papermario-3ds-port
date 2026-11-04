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
