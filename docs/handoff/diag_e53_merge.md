# diag_e53_merge: E5-3 learned replay, texture fault, extra batches (branch nds1)

Read-only diagnosis. No game source edited. No Azahar run. Tags: [M] measured (from docs/handoff/diag_e53_batches.md and agent_nds1_e6.md), [C file:line] read in source, [I] inferred.
Line numbers: HEAD (nds1 tip) for code that exists now; `patch N` = line N of docs/handoff/nds1_e53_merge_wip.patch for code that the patch adds.

## 0. Evidence limits

- The census logs of the E5-3 retry (71 / 810 / 53) are not on disk. `build3ds/` has no `cen f200 summary` line. Per-draw tuples and the 7 extra batches cannot be read from a log. [M-missing]
- No E5-3 shot exists in `build3ds/azt/shots/`. The region_diff against HEAD (step 1 of section 1) cannot run from disk. The only HEAD shots present are `build3ds/azt/shots/mac01t/0000_1_2.ppm` and `0001_1_2.ppm`.
- The working tree is HEAD. The E5-3 patch is not applied (`git status`: only `3ds/src/map_restore_table.c`). All patch lines below are read from the patch text.

## 1. Texture fault (first, per the coordinator order)

### 1.1 Step 1, region_diff vs HEAD shot: NOT DONE

Needs one E5-3 shot and one HEAD shot at the same frame (mac01t frames 500 and 504 are the reference, status_NDS1.txt). Command: `python 3ds/tools/region_diff.py <a> <b>` (Git Bash). Not run. The code agent runs it after the fix in section 3.

### 1.2 Step 2, per-draw texture words: code evidence, not measured

Replay writes template words, not the words of each draw:

- `patch 113`: the learn NatRec is written with `tex0 = tex1 = NAT_TEX_TMPL` (0xFF). So no draw of the learned group carries its own texture id.
- `patch 221-222` (pm_native.c nat_emit): when `tex0`/`tex1` is 0xFF, the words 22..25 and 28..31 stay the template words. The template is taken from the first flush of the learn window (`rec_done`, pm_gbi.c:3765).

Unit 0 check, learn (patch 102-104, dlc_learn_end):
- `pm_gpu.c:1072` captures unit 0 words per flush: `if (rec_cap && rec_fwn < 16)`. Flushes 17 and up of a learn window are NOT captured and NOT checked. [C]
- Unit 1 (words 28..31) is never checked at learn. Capture would need `pm_tex_cur_slots` second output `cb` (already read at pm_gpu.c:1072) and `pm_tex_slot_words(cb, ...)`. Not done. [C]

Unit 0 check, replay (patch, dlc_nat_group): `pm_tex_slot_words(g->lnr_s0, sw)` against `grp+22` only. Unit 1 is not checked at replay. [C]

Result, by code: every draw of a learned group that has a unit 1 texture different from the first draw's unit 1 gets the template unit 1 texture. Every draw of a window with more than 16 flushes and a unit 0 change after flush 16 gets the template unit 0 texture. Census does not show this: census tuples (`dlc_lcen`, `ln_cen`) hold the LIVE ids (pm_gpu.c:510-519), not the emitted words. So the census cannot detect a wrong texture.

Count of wrong-texture draws: NOT MEASURED. Needs the TEXW log per draw (proposed in section 3, E1/E2).

### 1.3 Step 3, are the 7 extra batches the wrong-texture draws: NO

- A texture word does not change the replay flush count. The replay counts one tuple per NatRec draw (`pm_gpu.c:2677`, `cen_put` per tuple), whatever texture the draw uses.
- So the wrong texture and the extra batches come from two mechanisms. Section 2 gives the batch mechanism.

Step 4 (continue with the merge/split rule) is in section 2.

## 2. The 7 extra batches: mechanism and merge/split rule

### 2.1 Where batches are counted

- Live: one count per `flush_draw` that queues something. `cen_add` at `pm_gpu.c:1100` runs inside `flush_draw` (starts pm_gpu.c:952). Count = `(graw ? gni/3 : nverts/3)` triangles.
- Replay: `pm_gpu_rec_play` (pm_gpu.c:2668) calls `flush()` FIRST (line 2672), then counts one `cen_put` per NatRec draw (line 2677), then `draws_frame += nd` (line 2681).

So the replay costs: (a) one flush for the open live batch before it, if any, (b) `nd` counts, (c) the next live segment starts a new batch, because `gni` is 0 after the flush.

### 2.2 The live merge rule (what the live path does)

`grange_batch_open` (pm_gpu.c:2314-2321). A segment EXTENDS the open batch only when all hold:
1. `gni != 0` (the batch has indices),
2. `g_pal` set and `pal_n != 0`,
3. `pal_g[0] == gm` (game matrix in palette slot 0),
4. `g_ct == ct`, `g_use5 == use5`,
5. if use5: `g_fx` equal to `fx`,
6. `g_uvso`, `g_vp`, `g_cmul`, `g_cadd`, `g_zmix` equal (memcmp).

`pm_gpu_grange` (pm_gpu.c:2323): refuses (returns 0) on `!gv4_on || !gpu_ok`, `r_zmark`, `r_decal && r_blend`, pm_wide matrix test, and `gidx_n + nidx > GRING_IDX` (GRING_IDX = 65536, pm_gpu.c:116). It flushes (`if (nverts > 0) flush();`, pm_gpu.c:2336) before extending or starting. A key change with `gni > 0` closes the batch by flush.

Texture: NOT in the grange key. A texture change inside a group splits only through the flush sites in pm_gbi.c (e.g. 2862, 2917, 2996, 3044, 3228 are `pm_gpu_flush()` calls; which of them a texture bind hits was NOT traced). [I]

Triangle limit: no separate limit. Only `GRING_IDX` (65536 indices = 21845 triangles) and `pm_gpu_gidx_room` (dlc_static_group, pm_dlc.inc.c:357). [C]

Order: merge is by key, not by list order. A later segment with the same key merges with an earlier open batch if no flush happened between them. [C]

### 2.3 The live path inside a group

`dlc_static_group` (pm_dlc.inc.c:350) loops over the group's batches (`bat_q`, `sd` ends) and calls `dlc_seg` (pm_dlc.inc.c:327) per segment. `dlc_seg` calls `pm_gpu_grange` at pm_dlc.inc.c:338. So live merges across segments AND across groups (C3), as long as the key holds and no flush happens.

### 2.4 The replay merge rule (patch 120-125, dlc_learn_end)

Adjacent learn draws merge only when: `tmpl`, `tex0`, `tex1`, `mtx` equal; `idx_off + 2*nidx` contiguous; `nidx` sum <= 0xFFFF; cen `h`, `id0`, `id1` equal (ca[0..2]).

- `patch 120`: `a->tex0 == b->tex0 && a->tex1 == b->tex1` is VACUOUS. `patch 113` set both to 0xFF for every draw. The real texture key is cen `id0`/`id1` (ca[1], ca[2]).
- The E5-1 key (`nat_seg_add` in pm_dlc.inc.c:296-322, `same` at pm_dlc.inc.c:310) compares gm, ct, use5, fx, vp, uvso, cmul, cadd, zmix, and NOT `rm` and `sl` (the E5-1 render mode and slot fields are stored at 302-313 but not compared). So the mirror key is not the live key. [C]

### 2.5 Why merging did not reduce the count (retry 71 before and after)

- Pre-merge run 1: 71 / 810 / 54. Retry: 71 / 810 / 53 [M]. The merge changed the distinct count only. So the merge fired on no replayed draw that reached the census, or the count of merged pairs equals zero at frame 200.
- Therefore the merge is not the batch lever. The replay flush (2.1 a) and the lost cross-group merge (2.2, `gni` reset) are the candidates. [I]

### 2.6 Cause (inferred, not measured)

Each replay closes the open live batch (pm_gpu.c:2671) and leaves `gni == 0`, so the next live segment cannot extend it (grange_batch_open condition 1). Live would have merged across the group boundary (C3). The replay adds up to 2 flush boundaries per replayed group and counts `nd` tuples. The live group in the learn frame counts its own flushes.

Measured link: replay on = 71, replay off = 64 (diag_e53_batches.md runs 1 and 2). Count per replay not measured (14 replays at frame 200, [M]). The extra 7 are consistent with about 0.5 boundary per replay. Not proven. The decision log line needed: per replay, `gni` and `nverts` before the flush, and `nd`.

## 3. Edit list (for the code agent)

Option A (recommended, gate-passing): remove replay for static groups. Census 64 / 810 with replay off is measured [M, runs 2 and 3]. The draw path is then HEAD's (`dlc_static_group`), so the picture equals HEAD [C].

- A1. `3ds/src/pm_dlc.inc.c`, dlc_try, line 639 (patch 68): replace `(!rec_on && dlc_nat_group(e, (DlcGrp*)g, gmi, gg)) || dlc_static_group(e, g, gmi, gg)` with `dlc_static_group(e, g, gmi, gg)`. 1 line.
- A2. `3ds/src/pm_gbi.c`: delete `dlc_nat_group` (patch 146-167, 22 lines). 22 lines.
- A3. `3ds/src/pm_gbi.c`, dlc_learn_end (HEAD 3639): remove the NatRec, census, and merge code the patch adds (patch 97-127). Restore the HEAD TEXW block (log only, no refusal) and the HEAD `use_t1` condition. Lines in patch 97-127. Up to 30 lines.
- A4. `3ds/src/pm_gpu.c`: delete `pm_gpu_rec_sa`, `pm_gpu_nat_addr_at`, `pm_gpu_rec_pool` (patch 205-232 area of the pm_gpu.c hunk). Restore `if (off_active || !ia) return 0;` at pm_gpu.c:2670 (HEAD, the `!ia` test is the only change). Up to 30 lines.
- A5. `3ds/src/pm_native.c`: revert nat_emit tex lines (patch 221-222) and delete `nat_pool_ptr` (patch 223 area). `3ds/include/pm_native.h`: delete `NAT_TEX_TMPL` and the `nat_pool_ptr` prototype (patch 9-10).
- A6. `3ds/src/pm_dlc.inc.c`: delete the `dlc_lnr` block, the `lnr_*` fields of DlcGrp (patch 9 area line 9-10 of the second hunk), and the `DLC_LNR_MAX` / `dlc_lnr_n` lines in dlc_reset and dlc_learn (patch 40-62).
- Gate: census_mac01 64 / 810 (distinct 52 expected). Picture: region_diff of an E5-3-free run is not needed, because the path is HEAD. Run one census_toad for 26 / 338 / 23. Rule 7: the replay path is deleted in the same commit.

Option B (keep replay, texture first). Only if the user wants replay. Batches stay above 64 unless the replay boundary is removed (not in this list).

- B1. `3ds/src/pm_gpu.c:1072`: remove `rec_fwn < 16`. Add `rec_over = 1` when the window has more flushes than 16. dlc_learn_end refuses when `rec_over`. 6 lines.
- B2. `3ds/src/pm_gpu.c:496`: add `static uint32_t rec_fw1[16][4];`. At pm_gpu.c:1072 also capture `pm_tex_slot_words(cb, rec_fw1[rec_fwn])`. 5 lines.
- B3. `3ds/src/pm_gpu.c:2516`: add `pm_gpu_rec_fw1(uint32_t (*out)[4])` returning the unit 1 words. 4 lines.
- B4. `3ds/src/pm_gbi.c` dlc_learn_end (patch 102-104): also require each `fw1[q] == nat_tmpl[tid].grp + 28`. Refuse otherwise. 4 lines.
- B5. `3ds/src/pm_gbi.c` dlc_learn_end merge (patch 120): delete `a->tex0 == b->tex0 && a->tex1 == b->tex1 &&` (vacuous). 1 line.
- B6. `3ds/src/pm_gbi.c` dlc_nat_group (patch 152-154): also compare `pm_tex_slot_words(g->lnr_s1, sw1)` against `grp+28`. Store `lnr_s1` in DlcGrp at dlc_learn (pm_dlc.inc.c, patch 35). 6 lines.
- B7. Gate for B: census 64 / 810 is NOT expected (the replay boundary remains, section 2.6). Gate: region_diff of mac01t frames 500 and 504 against the HEAD shots, 0 px on the viewed region. Then census for the count.

## 4. Replay vs producer records (question 3)

Learn-and-replay is needed only because the record is built from a live window at run time. A producer record avoids the learn window, the TEXW check, and the unit 1 gap, because the texture and combiner are known before the first draw.

Producer: `src/model.c`, `appendGfx_model` (line 1384 start).
- Header op: lines 1515-1517 (`G_PM_MODEL` w0; w1 = pmSlot bits 0-7, static matrix bit 8 (`finalMtx == &savedMtx`, not billboard), plain opaque bit 9, pmMtxVer bits 16-31). Length set at line 2073.
- Texture data already held: `textureID + textureVariation` (model.c:1433-1434, `TextureHandles[...]`), also used by the tag at 1508 (`pm_gbi_ms0_mark`).
- Render mode: `renderMode` (model.c:1508).
- Combiner: set in the DL words, not in a variable. `gDPSetCombineMode` at model.c:1583. A producer record reads the combiner from the DL words, not from a stored value. [C]
- Vertices and indices: NOT in `appendGfx_model`. It emits `gSPDisplayList(modelNode->displayData->displayList)` (model.c:2056). The triangle list is in the DL and in the static bake (`g->sd`, `g->bat_q`, `g->pidx`; pm_dlc.inc.c:350-403, bake fields at pm_dlc.inc.c:6-12). So the producer needs the bake output, not model.c alone.

Proposal (not a code change now): at bake time (dlc_static_build, pm_dlc.inc.c), write the NatRec draw list from `sd`/`bat_q` (one draw per live-merge key, section 2.2), with the texture ids from the model header (`textureID + textureVariation`), and the template key from the group. This removes the learn window. The producer data needed: `sd`, `bat_q`, `ng`, `goff`, the gop list (`e->gop`), and the model texture id. Gate: census 64 / 810 and the picture.

Caution: the bake-time draw list still needs the batch boundaries of section 2.2 (key changes, gidx room). The replay boundary of section 2.1 disappears only when no replay runs.

## 5. Summary of answers

1. Texture fault: real in code. Unit 1 never checked. Unit 0 checked only for 16 flushes. Replay emits template words. Census hides it. Count not measured.
2. The 7 extra batches: not the wrong-texture draws. They are the replay flush boundary (2.1 a, 2.6), plus the lost cross-group merge. Merge of learn draws does not change the count (71 before and after). Inferred, not measured.
3. Exact live merge key: section 2.2 (pm_gpu.c:2314-2321). Texture is not in the key. Splits: key change, gidx room, nverts flush, r_zmark, r_decal with r_blend, pm_wide matrix test.
4. Edit list: section 3. Option A gives 64 / 810 with the HEAD draw path (picture equal to HEAD). Option B needs B1-B7 and is not expected to reach 64.
5. Producer: section 4. Texture and render mode are in model.c. Vertices and indices come from the bake, not from model.c.
