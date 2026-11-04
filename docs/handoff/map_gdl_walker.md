# map_gdl_walker: how a baked model reaches NatRec, and the exact edits

Branch nds1, worktree papermario-3ds-port-nds1, base 9a6e5932d (P3a). Read-only on source. This document is the only file in the commit.
Tags: **[M]** measured in this run (Azahar, census_mac01). **[C file:line]** read in source. **[I]** inferred. **[D doc]** from a handoff note, not re-checked.

Runs of this investigation:
- NDS1 budget reset (`run_azahar.ps1 -ResetBudget NDS1`). Starts used on NDS1: 2 (HEAD census_mac01 runs 1 and 2).
- GDLW: 1 start (probe build, log only, reverted in source). Justification lines are in `3ds/logs/perf/status_GDLW.txt` (gitignored).
- Probe log: `build3ds/lane-nds1/last_log.txt` lines 175 to 236 (the last run wrote this file).

---

## 0. Root cause of the P3a refusal (reason 2) [M]

The index address word (0x227, parameter 0, "p0") is a GPU address, not a plain physical address.

| Value | Measured | Meaning |
|---|---|---|
| phys(gidx + rb_gi), first learned draw | 0x20389D6E | physical address of the first index of the window |
| raw p0 written by citro3d | 0x88389D6E | = 0x80000000 \| (phys - 0x18000000) |
| d (pm_gpu_rec_idx, pm_gpu.c:2514) | 0x003DE792 | phys(ia + ia_n) - phys(gidx + rb_gi) |
| rewritten p0 in the record | 0x88768500 | = 0x80000000 \| (phys(ia) - 0x18000000) |
| phys(ia), the value nat_rec_write takes as base | 0x20768500 | `pm_gpu_rec_ia_phys()` (pm_gpu.c:2533) |
| off = w[i] - base (pm_native.c:188) | 0x68000000 | refused: off >= 0x20000 |

Source: `build3ds/lane-nds1/last_log.txt` lines 175 to 176 and 180 (the `GDLW base` and `GDLW p0 raw` lines).

- The rewrite itself is correct. The record holds the right GPU address of the arena copy.
- The bug is the base. `pm_gpu_rec_ia_phys()` returns the raw physical address. The GPU word has bit 31 set, and it is relative to 0x18000000. The difference is 0x80000000 - 0x18000000 = 0x68000000. That is exactly the offset in every P3a refusal.
- Every learned capture of the probe run (12 of 12 printed draws, lines 175 to 236) has the same 0x68000000 offset. This matches "reason 2 for every capture" in agent_nds1.md.

The same GPU address form applies to the DLC2 draw path (section 1).

---

## 1. Where the index list of a DLC2 static group lives [M, C]

| Copy | Where | Address class | Source |
|---|---|---|---|
| sd (triangle indices, sn triangles, then the geometry-mode split counts after 3*ntri) | `e->grp[k].sd`, heap malloc | heap | pm_dlc.inc.c:228 (malloc), 230 to 260 (fill) |
| Bake copy of sd (3*sn u16) | linear pool, `nat_pool_put(sd, 3u*nt)`, the returned pointer is dropped | linear (pool 128 KB) | pm_dlc.inc.c:268; pool pm_gpu.c:2901 to 2905 |
| Draw copy, per batch | `gidx + gidx_n` (memcpy) | linear (linearAlloc GRING_IDX*2) | pm_gpu.c:2295, 2319; alloc pm_gpu.c:1348 |
| Vertices of the group | gring static area, `pm_gpu_gstat_alloc` | linear, `gring + 16*first` | pm_gpu.c:2251 to 2258 |

- sd values are absolute vertex slots in gring (`first + nv + i`, pm_dlc.inc.c:242), not local indices.
- The draw reads gidx, not the pool: `C3D_DrawElements(..., gidx + gi_start)` at pm_gpu.c:1149, with BufInfo base gring (pm_gpu.c:1146).
- The pool copy exists (P1) but no draw uses it yet. `DlcGrp` has no field for the pool pointer (pm_dlc.inc.c:9).

**Index address (0x227 p0) today:**
- DLC2 path: citro3d computes it from `gidx + gi_start`. Measured: phys(gidx + rb_gi) = 0x20389D6E gives p0 = 0x88389D6E [M]. The formula is p0 = 0x80000000 \| (phys - 0x18000000). The citro3d code is not in the repo [I].
- Record path: `pm_gpu_rec_idx` (pm_gpu.c:2504) copies the window indices to `ia` (linear, 128 KB, pm_gpu.c:2507) and adds d (pm_gpu.c:2514, 2518). The result is correct for the GPU. The base used later (pm_gpu.c:2533) is wrong (section 0).

---

## 2. What the DLC2 static draw emits per group today [C]

Call chain, per group in view (dlc_try loop, pm_dlc.inc.c:510 onward):
1. `dlc_box_out` skip (dlc_try loop, pm_dlc.inc.c:510 onward). Skipped groups emit nothing.
2. `dlc_static_group` (pm_dlc.inc.c:300): lighting and fog checks, `pm_gpu_gidx_room`, then for each batch b (`g->bat_q`, `ends[b]`): `dlc_set_geom` then `dlc_seg`.
3. `dlc_seg` (pm_dlc.inc.c:278) calls `pm_gpu_grange` (pm_gpu.c:2278). This queues indices into gidx and palette entry 0 (gm). **No PICA word is written here.**
4. `grange_batch_open` (pm_gpu.c:2269) extends the open batch across groups when gm, ct, vp, uvso, cmul, cadd, zmix and fx are equal. This is the cross-group merge (return 2). A new batch is return 1.
5. The PICA words are written at `flush()` / `flush_draw` (pm_gpu.c:938), which runs when a batch closes. Per draw the words are:
   - palette 0x2C1, 16 words (uniform, from the palette of gm),
   - unit 0 and unit 1 texture blocks 0x081 and 0x091 (4 words each), address words 0x085 and 0x095 (via `dc_bind`, pm_gpu.c:827, and citro3d dirty bits),
   - index 0x2C0, index address 0x227, count 0x228, trigger 0x22F (`C3D_DrawElements`, pm_gpu.c:1149),
   - TEV, fragment and other state words, only when they changed (citro3d dirty bits) [D agent_nds1.md:94, 108].

**Shared across groups today:**
- The palette and the batch: groups with the same gm and render state share one draw (grange_batch_open).
- State words: the GPU keeps register state between draws, so unchanged state is not sent again (citro3d dirty bits).
- Not shared: the index address and count per batch (each batch is one draw), and the 0x081, 0x091 blocks when the texture changes.

I did not decode the citro3d flush word by word. The per-draw list above follows agent_nds1.md:108 and the nat_kind table (pm_native.c:53).

---

## 3. Shortest route to emit a baked model by NatRec [C, I]

Data the DLC2 path already has for a group (per DlcEnt / DlcGrp):

| Needed for a NatRec | Available | Where |
|---|---|---|
| index address (idx_off) | yes, after edit E3 | pool pointer per group (new field), offset 6*t per batch |
| index count (nidx) | yes | 3 * (ends[b] - t), dlc_static_group, pm_dlc.inc.c:300 onward |
| batch boundaries | yes | `g->bat_q`, `ends` (sd + 3*ntri), dlc_batch_compile at pm_dlc.inc.c:111 to 140 |
| geometry per batch | yes | `e->gop`, `g->goff`, `g->ng` |
| game matrix (mtx) | yes | `dlc_gmi()` (pm_dlc.inc.c, before dlc_set_geom) |
| vertex base | yes | first vertex of the group (gstat alloc) |
| texture handles (tex0, tex1) | yes, at emit time | `pm_tex_cur_slots` (pm_tex.c:673), then `pm_tex_census_id` for the block |
| template key (state tuple) | yes at emit time | the same args as `nat_tmpl_add` (pm_native.c:19), from `rec_g0` state words |
| **template block (state words, `t->blk`)** | **no** | only a capture produces it: `nat_tmpl_strip` on the words of `rec_done` (pm_gbi.c:3746, 3800) |
| texture block words (0x081, 0x091 data) | no, only from a capture | `nt->grp` of the template (pm_gbi.c:3803 to 3805) |

**Missing piece: the template block.** The PICA state words cannot be made from the DLC data. They come from citro3d. So the emit still needs one capture per template key (decision (a) in agent_pmod.md round 4). The per-draw data (index, count, matrix, texture handle) comes from the DLC2 data and no longer from a record.

The capture-derived per-draw path (rec_done NatRec write, pm_gbi.c:3800 to 3810, and rec_play nat branch, pm_gbi.c:3844) is then not needed. Only the template capture remains.

---

## 4. Rule that maps bake groups to record draws (nd and ngrp) [C, I]

- `nd` (record) = number of 0x227 writes in the window = number of new batches (pm_grange return 1) in the window. Checked in pm_gpu_rec_idx: `nd != pm_dc_c[DC_RAW] - rb_raw` (pm_gpu.c:2521).
- `ngrp` (bake) = `e->ngrp` = groups of the list (pm_dlc.inc.c, dlc_compile (e->ngrp)). This is not nd. A group can give zero, one or several batches, and cross-group merge (grange_batch_open) lowers nd.
- The map is per view:
  - a group with `dlc_box_out` true gives no draw,
  - a group that is not static (`g->sd` NULL, lit with no gv_lit_ok, no room) runs in the walker and gives draws of its own.
- Rule for the emit: walk the groups in list order, skip as dlc_try does, and for each batch b with ends[b] > t call the same merge test as grange_batch_open (gm, ct, vp, uvso, cmul, cadd, zmix, fx). A batch that merges adds its indices to the open NatRec draw. A batch that does not merge starts a new NatRec. The result has the same draw count as the census (64 batches on census_mac01).
- For the 0x227 word, each NatRec is one draw: idx_off = pool offset of the first index of the draw, nidx = count.

I did not check the merge across dlc_try calls in one frame. The census batch count is the test.

---

## 5. Unit-1 handle 0x27 differs per record [M, C]

- Every learned record in the probe run has unit 1 handle 0x27 (`P3a pre ... tex 000x/0027`, `build3ds/lane-nds1/last_log.txt`, all rows). 0x27 is the aux slot (`aux`, pm_tex.c:673 area, `rec_g0[6] >> 16`).
- `nat_tex_set(h, blk)` (pm_native.c:205) stores the 4 words of the template block for handle h. The second record with handle 0x27 and a different block fails the memcmp (refuse tex, P3a line 219).
- The block is the data of the 0x091 command of the template prototype: registers 0x091 to 0x094 (`nat_kind` goff 26, data 28 to 31, pm_native.c:53). These are the unit-1 size, parameter (wrap and filter), border and type words. The address (0x095) is not in the block (it is a separate write, patched by the record).
- The block changes with per-draw render state (for example the wrap or filter set by G_SETTILE before a draw), and the table is keyed by the handle only. Two templates with the same handle and a different state then conflict.
- Which word differs was not read. A print of the two blocks would name it (one probe line in `nat_tex_set`).

**Treatment:** key the table by the block content, not by the handle (edit E2). The NatRec tex fields then carry a block id. No patch is needed, and the state stays in the table. The alternative (put the block in the template key) changes the key count and the template count (PM2 log: 13 keys in census_toad, design doc section 0).

---

## 6. Edits, in order (each one agent, about 30 tool calls)

Common gates for every edit:
- Build: `python 3ds/tools/gen_map_restore.py`, then `PM_LANE=nds1 PM_ROW=<row> sh 3ds/build.sh`, then `git checkout -- 3ds/src/map_restore_table.c`.
- Census: `PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_mac01 -Row <row> -Build nds1 -Try`. Reference: 64 batches, 810 triangles. Distinct is 52 or 53 (section 7). Batches and triangles must not change.
- Census toad: `census_toad` 26 / 338 / 23 / 0.
- Justification line first in `3ds/logs/perf/status_<row>.txt`.
- Host tests: `sh 3ds/tools/nat_host_test.sh`.
- Commit each edit on the nds1 branch. Do not commit `3ds/src/map_restore_table.c` or status files.

Note on the default path: nat replay runs only with the debug string `recplay` (pm_gbi.c:3621, default off). Edits E1 to E3 change records only in log and in the built NatRec. The default draws do not change. E4 and E5 change draws.

**E1. GPU form of the arena base (fixes reason 2).**
- File: `3ds/src/pm_gpu.c`, function `pm_gpu_rec_ia_phys` (line 2533).
- Change: return the GPU address form of the arena start: `return ia ? (0x80000000u | ((u32)osConvertVirtToPhys(ia) - 0x18000000u)) : 0u;` Add a comment: GPU index address = phys - 0x18000000, bit 31 set (measured, section 0).
- Users: pm_gbi.c:3804 (rec_done), pm_gbi.c:3851 (rec_play). No other change.
- Gate: census_mac01 summary unchanged (64/810/52 or 53). Log: `P3a write refused` lines = 0. `P3a tmpl ... grp_eq` lines appear with the index compare. One start, PM_ROW=E1 row, justification line first.
- Expected: records now pass nat_rec_write. The next refusal is tex (E2).

**E2. Texture block keyed by content.**
- Files: `3ds/src/pm_native.c` (`nat_tex_set`, replace with `nat_tex_id(const uint32_t* blk)`: linear search of NAT_TEX_MAX entries for an equal 4-word block, insert if free, return id or -1), `3ds/src/pm_gbi.c` (rec_done, lines 3803 to 3805: call nat_tex_id for unit 0 (block nt->grp+22) and unit 1 (nt->grp+28), pass the two ids to nat_rec_write instead of the handles).
- nat_emit (pm_native.c, `nat_tex[r->tex0]`, `nat_tex[r->tex1]`) keeps working: it reads the table by id.
- NatRec tex0 and tex1 are u8 (pm_native.h), so the table must hold at most 255 blocks. Check NAT_TEX_MAX.
- Gate: census unchanged. `P3a refuse tex` count = 0 on census_mac01. Host test: add two blocks with equal handle and different content, expect two ids; same content, expect one id.

**E3. Pool pointer per group and GPU base of the pool.**
- Files: `3ds/src/pm_dlc.inc.c` (DlcGrp, line 9: add `const u16* pidx;`; line 268: `g->pidx = nat_pool_put(sd, 3u * nt);`), `3ds/src/pm_native.c` (new `uint32_t nat_pool_gpu_base(void)`: GPU form of nat_pool_base, same as E1).
- dlc_free: no change, the pool is reset with the map (nat_model_reset).
- Gate: census unchanged. Host test: `pidx` offset equals the sd index of each group; GPU base has bit 31 set and is below 0x80000000 + 0x20000 for the first group.

**E4. DLC2 group to NatRec (per batch, merged by the grange rule).**
- File: `3ds/src/pm_dlc.inc.c` (new function next to dlc_static_group, line 300 onward): for a group in view, compute the NatRec list the same way as dlc_static_group does (same batch loop, same grange merge test), with idx_off = (pidx - pool base) + 6*t, nidx = 3*(end - t), mtx = dlc_gmi(), tex0 and tex1 from pm_tex_cur_slots through E2, tmpl from the state key. Output: NatRec array plus nat_emit call. No draw is emitted yet.
- Gate: host test (counts of NatRec per DL equal the census batch count of the same DL when run by the walker).

**E5. mg_run arm for ok groups (P3 main path, eye 0 only).**
- Files: `3ds/src/pm_gbi.c` (mg_run, pm_gbi.c:3869, G_DL case near line 3916: for a static group with `sd` and the template captured, call the E4 builder and nat_emit instead of dlc_static_group), `3ds/src/pm_dlc.inc.c` (remove the walker path for these groups in the same commit, rule 7).
- Template capture: keep the capture once per template key (rec_try, rec_done) as decision (a). The capture is needed only for the state block.
- Gate: census_mac01 64 / 810 / 52 (or 53, batches and triangles must match exactly). census_toad 26 / 338 / 23. Shot `census_toad_shot` (until shot 1 frame 204): the subject (a Toad Town model) is in the picture, and region diff against `build3ds/azt/shots/census_toad_shot/0000_0_8.ppm` is 0 px on the walker-free models (read the picture). The region diff script is `3ds/tools/region_diff.py`.
- Pass only with the picture (the shot shows the subject).

**E6 (later, not in this file's scope).** Eye 1 in the same walk (P5), v2 draw (P6), delete old path (P7). See design_producer_models.md section 10.

---

## 7. Distinct count: 53 against 52 [M]

| Run | Build | batches | tris | distinct |
|---|---|---|---|---|
| reference (agent_pmod.md round 4, one run) | c49063a3b | 64 | 810 | 52 |
| HEAD run 1 (NDS1) | 9a6e5932d | 64 | 810 | 53 |
| HEAD run 2 (NDS1) | 9a6e5932d | 64 | 810 | 53 |
| probe run (GDLW, log lines in pm_gpu_rec_idx, reverted) | 9a6e5932d + probe | 64 | 810 | 52 |

- Between c49063a3b and HEAD, the code changes are the P3a log lines (a4f5eb8ff, 9a6e5932d) and one docs commit (00337b62a). Checked with `git diff c49063a3b HEAD -- 3ds/src`. No draw code changed: the P3a compare is read-only, the refusals return as before, and p3a_log only prints.
- Batches (64) and triangles (810) equal in all four runs. Only the distinct count moves (52 or 53).
- The distinct count is a count of census tuples (census id is a texture content hash, pm_tex.c:675, not an address). A value that flips with a log-only edit is run-to-run or timing sensitivity, not a draw change. I did not isolate the cause. [I]
- Decision: treat distinct as 52 to 53 for the census gate. Use batches and triangles as the exact gate. Repeat the reference at c49063a3b if a single value is needed (one start, NDS1).

---

## 8. Open decisions

1. Template block source (decision (a) of agent_pmod.md round 4): one capture per template key, kept until P7. Recommended. The per-draw NatRec path from the record is removed.
2. Which word of the unit-1 block differs (section 5). Needed only to explain the conflict; E2 works without it.
3. Scope of the census distinct gate (section 7): use batches and triangles as the exact gate.
