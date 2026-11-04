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
