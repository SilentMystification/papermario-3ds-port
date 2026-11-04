# map_e4: why grp_eq is 0, the merge state, and the edits E4 to E6

Branch nds1, worktree papermario-3ds-port-nds1, HEAD 649caee9a (docs only on top of a7c40bec7). Tags: [M] measured (Azahar, census_mac01, PM_ROW=GRPE, 1 start), [C file:line] read in source, [I] inferred.

## 1. grp_eq: the flag is a group index, not a mismatch flag [M, C]

Source: `p3a_log` (pm_gbi.c:3723 to 3738). `hit` is set to `j` (the bake group index) when `pm_gpu_rec_ia_eq` returns 1 (memcmp of the arena span with `gg->sd`). The log prints `hit` as `grp_eq`. So:

- `grp_eq 0` = the draw matched bake group 0 (memcmp equal, n = nidx).
- `grp_eq -1` = no single bake group matches.
- `grp_eq 1` never occurs because every single-group record has `ngrp 1`.

Count in the GRPE run (52 P3a lines, `build3ds/lane-nds1/last_log.txt`):

| grp_eq | ngrp | draws |
|---|---|---|
| 0 | 1 | 46 |
| -1 | 2, 3, 4 | 6 (2 with ngrp 2, 2 with ngrp 3, 2 with ngrp 4) |

So the 46 draws are matches, not failures. The 6 draws are merged draws (see 3).

Three draws with the printed values (GRPE lines in the same log):

| draw | nidx (record) | idx_off bytes (arena word) | window (gidx, u16) | record first 12 | bake g0 sn (count) | bake sd first 12 |
|---|---|---|---|---|---|---|
| 1 | 6 | 48 (24) | 207..213 (6) | 16518 16519 16520 16518 16520 16521 | 2 (6) | 16518 16519 16520 16518 16520 16521 |
| 2 | 24 | 64 (32) | 213..237 (24) | 16522 16523 16524 16522 16524 16525 16526 16527 16528 16526 16528 16529 | 8 (24) | same 12 values |
| 3 | 18 | 256 (128) | 420..438 (18) | 16674 16675 16676 16674 16676 16677 16674 16677 16678 16674 16678 16679 | 6 (18) | same 12 values |

Findings:
- The index VALUES are equal. Both use absolute static slots (>= 16384 = GRING_VTX, pm_gpu.c:116, `first + nv + i`, pm_dlc.inc.c:242). The record does not hold ring slots for these draws. Hypothesis "ring slots vs static slots" is rejected for this scene.
- The OFFSET differs. The arena copy holds the window at a different base: window gidx 207 is arena word 24. The shift d (pm_gpu.c:2514) moves the 0x227 address by the arena base, not by a value change.
- The count is equal (nidx = 3 * sn) for the single-group draws.

Mapping rule (single-group draw, 46 of 52): the draw index list = `sd[0 .. 3*sn)` of the one group. The record needs only the arena base replaced by the pool base:
- Index values: identical to `g->sd` (and to the pool copy `g->pidx`, E3, which is a memcpy of sd, pm_native.c:229 to 236).
- Index start: `pool_off = (byte offset of g->pidx in the pool) + 6 * t`, where t is the first triangle of the batch segment in the group (t = 0 for a whole group).
- Index count: 0x228 = `3 * (end - t)` (same as dlc_static_group, pm_dlc.inc.c:300 onward).

## 2. Words that change, and where they come from

For a NatRec built from the DLC2 bake instead of the window copy:

| PICA word | Record today | NatRec from the bake | Source |
|---|---|---|---|
| 0x227 p0 (index address) | 0x80000000 \| (phys(arena + idx_off) - 0x18000000) | 0x80000000 \| (phys(pool + pool_off) - 0x18000000), i.e. `nat_gpu_addr(phys)` (pm_native.c:228) | pool base from E3 (`nat_pool_gpu_base`), `pool_off` from section 3 |
| 0x228 (index count) | nidx | 3 * sum of segment triangles | sd counts, pm_dlc.inc.c:228 to 260 |
| attribute buffer base (0x200 range) | gring | gring (unchanged) | BufInfo base gring, pm_gpu.c:1146 [C]; record indices are the same absolute static slots, section 1 [M] |
| vertex count | not a word of an indexed draw | nvtx stays in the gstat area | no change [C] |
| 0x2C0 index format, 0x22F trigger | unchanged | unchanged | nat_kind table, pm_native.c:53 [C] |
| tex0/tex1 (0x081, 0x091 blocks, addresses 0x085, 0x095) | block ids via nat_tex_id (E2) | same, from pm_tex_cur_slots | pm_gbi.c:3803 to 3805 [C] |

The rule for pool_off is only valid for an index run that is contiguous in the pool (section 3).

## 3. Multi-group draws (6 of 52) and the pool padding [M, C, I]

- The 6 draws with grp_eq -1 have ngrp 2, 3 or 4 and nidx 54, 90, 156, 186. Each is one 0x227 draw over several bake groups (grange merge, pm_gpu.c:2269 to 2290). The draw index list is the concatenation of the group segments in list order [I, checked by the sum rule; not printed per group].
- `nat_pool_put` (pm_native.c:229) rounds each group copy up to 32 bytes. Groups are not contiguous in the pool, so a merged draw cannot be one slice of the pool.
- Fix: pack the pool without padding (`bytes = n * 2u`). Consecutive static groups of one DL are then contiguous, and the merged draw is one `(pool_off, nidx)` pair. A gap (a skipped group, dlc_box_out, pm_dlc.inc.c:510) splits the draw. The split count is checked in the E4 gate.

## 4. The grange merge state, and why E4 needs its own copy [C, I]

Live state (pm_gpu.c, pm_gpu_grange at 2278 and grange_batch_open at 2269):
- `g_pal`, `pal_n`, `pal_g[0]` (= gm), `g_ct`, `g_use5`, `g_fx`, `g_uvso`, `g_vp`, `g_cmul`, `g_cadd`, `g_zmix`: the parameters of the OPEN GPU batch.
- `gni`, `gidx_n`: indices queued in that batch.
- `g_cullm`, `vp_sx..vp_ty` (rebuild_3d on a viewport change).
- Refusals in pm_gpu_grange: gv4_on, gpu_ok, use5 without gv5_on, r_zmark, r_decal && r_blend, pm_wide with the matrix test, gidx room. Flush at pm_gpu.c:938.

Why it cannot be read at emit time:
- The merge is decided when a group is queued, and the state describes the open batch of the live GPU queue. The NatRec builder runs in dlc_static_group before any queueing. The live state then belongs to the previous batch, not to the builder's draw.
- The parameters of a group come from `ep_p` (after gv_params_calc, pm_dlc.inc.c:284) with `ep_ct`, and they change per segment (dlc_seg, pm_dlc.inc.c:278). They are not kept per draw.

So the builder must keep the merge key per draw (a mirror), not read g_*. Store per NatRec draw: gm, ct, use5, fx (if use5), vp, uvso, cmul, cadd, zmix. This is the same comparison as grange_batch_open, copied, with the refusals of pm_gpu_grange as early returns.

## 5. Exact edits, in order (one commit each, nds1 branch)

Common gates as in map_gdl_walker.md section 6. Build: `python 3ds/tools/gen_map_restore.py` (use `/c/Python314/python`, no `python` on PATH), `PM_LANE=nds1 PM_ROW=<row> sh 3ds/build.sh`, then `git checkout -- 3ds/src/map_restore_table.c`. Census: `PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_mac01 -Row <row> -Build nds1 -Try`. Reference: 64 / 810 / 52 (batches / triangles / distinct). census_toad: 26 / 338 / 23.

**E4a. Pool packing (pm_native.c:229).**
- Change: `unsigned bytes = n * 2u;` (drop the 32-byte round). Comment: packed, so consecutive groups are contiguous (map_e4.md section 3).
- Gate: census_mac01 64 / 810 (no draw change; the pool is read only by E4b). Host test `sh 3ds/tools/nat_host_test.sh` must pass.

**E4b. Builder with a per-draw merge key (pm_dlc.inc.c, new, next to dlc_seg, line 278).**
- Struct (file static):
  `typedef struct { unsigned gm; int ct, use5; PmFx fx; float vp[4], uvso[4], cmul[4], cadd[4], zmix[2]; } NatKey;`
  `static NatKey nk_key; static int nk_open; static NatRec nk_draw[255]; static unsigned nk_nd, nk_end;` (nk_end = pool byte offset after the last segment).
- Set: in `nat_seg_add(const DlcGrp* g, unsigned t, unsigned end, unsigned gmi)`, called from the batch loop of dlc_static_group in place of `dlc_seg` for the builder only. Copy the key from `ep_p`, `ep_ct` and `gmi` when a new draw starts. A new draw starts when `!nk_open`, the key differs, or the segment's pool offset != nk_end.
- Read: key compare (memcmp per field) exactly as grange_batch_open (pm_gpu.c:2269). Refusals (return 0, group falls back to the walker): `r_zmark`, `r_decal && r_blend`, `pm_wide && !off_active` with the matrix test, and `pm_gpu_gidx_room` (nk_nd < 255).
- Output: nk_draw[0..nk_nd) with idx_off = pool byte offset of the first segment (E3 pidx byte offset + 6 * t), nidx = sum. mtx = dlc_gmi(), tex0/tex1 = nat_tex_id (E2), tmpl = the template key of the state. No draw is emitted.
- Gate: host test (new case in nat_host_test.c: two adjacent groups with equal key give one draw, a gap or a key change gives two). Log line `NATB dl <addr> groups <n> draws <nk_nd>`. Sum of draws over a frame equals the census batch count for the same DL (64 on census_mac01).

**E5. Use E4b in dlc_try (pm_dlc.inc.c:574) for groups with a captured template.**
- Change: at line 574, when `g->sd` and the template key of the group state is captured, call the E4b builder and `nat_emit` (pm_native.c:145) instead of `dlc_static_group`. Remove the walker branch for these groups in the same commit (rule 7). Template capture stays once per key (rec_done, pm_gbi.c:3800), as decision (a).
- Gate: census_mac01 64 / 810 / 52; census_toad 26 / 338 / 23. Shot `census_toad_shot` (until shot 1 frame 204): the Toad Town models in view. Region diff against `build3ds/azt/shots/census_toad_shot/0000_0_8.ppm` with `3ds/tools/region_diff.py`: 0 px on the walker-free models. A person reads the picture. The census gate does not pass without the picture.

**E6 (later).** Eye 1 in the same walk (P5), v2 draw (P6), delete the old path (P7). Out of scope here (design_producer_models.md section 10).

Gate per edit:
- E4a: census_mac01 64 / 810; host test.
- E4b: host test (merge and gap cases); NATB draw sum = 64 on census_mac01.
- E5: census_mac01 64 / 810 / 52; census_toad 26 / 338 / 23; shot diff on the viewed region of `build3ds/azt/shots/mac01t` frames 500 and 504 (baseline shots) and `census_toad_shot` 0 px.

## 6. Open points

- The 6 multi-group draws: the per-group segment list was not printed. The sum rule (nidx = 3 * sum of sn) is inferred, not measured.
- Whether a gap inside a merged draw happens in these scenes: not measured. E4a's contiguity check reports it.
- The attribute base is unchanged because the values are absolute static slots (section 1). This is measured on the three draws, not on all 52.
