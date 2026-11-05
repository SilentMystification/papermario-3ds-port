# map_e5: template source and key for map static groups, and the edits for E5

Branch nds1, worktree papermario-3ds-port-nds1, base HEAD 4d41802f2. Tags: [M] measured (Azahar, census_mac01, PM_ROW=ME5, 1 start, status line in 3ds/logs/perf/status_ME5.txt), [C file:line] read in source, [I] inferred, [U] not resolved.
Debug edit (ME5 snapshot in dlc_try) was reverted. Tree clean except 3ds/src/map_restore_table.c.bak.

## 0. Decision and what changes from agent_nds1 E5 blocker

Decision (a) stands: a learning hook in dlc_try for map static groups (blocker gap 1).
Change to the blocker design: the key is taken per draw (per dlc_seg batch, the same place nat_seg_add takes NatKey), not per group, and a draw is refused when its render mode at entry differs from exit. Section 2 gives the numbers.

## 1. What one live static batch emits [C, M]

A static group reaches the GPU only through dlc_seg (pm_dlc.inc.c:314) into pm_gpu_grange (pm_gpu.c:2278). The batch is drawn by flush_draw (pm_gpu.c:938) in one of these blocks, in this order:

| # | Words | Flush source | Depends on |
|---|---|---|---|
| a | VS uniforms: palette matrix per pal_n entry (u5_pal / u4_pal), uvso, cmul, cadd, zmix, fx (lflag, fogu), light table | g_pal, g_uvso, g_cmul, g_cadd, g_zmix, g_fx at flush (pm_gpu.c:952 to 985) | batch group params (ep_p, gv_params_calc) = entry state of the segment, not list exit |
| b | Program bind, attr info, projection uniform | bound, proj_last | previous batch (dirty shadows) |
| c | Texture unit 0 and 1 bind (pm_tex_bind, pm_tex_bind_aux) and TEV env | tile state at flush (pm_gpu.c:1053 region) | tile state set outside the list (dlc_compile refuses other ops, below) |
| d | Depth, stencil, alpha test, scissor, cull, blend | z_on, r_decal, r_blend, r_atest, scissor globals (pm_gpu.c:1100 to 1160) | render mode at flush (z_on from geometry mode, the rest from othermode) |
| e | DrawElements: 0x227 index address, 0x228 index count, 0x22F trigger | gidx + gi_start, gni (pm_gpu.c:1132 to 1137) | per draw only |

The template holds the words that do not change per draw (a, b, c, d). The per-draw words are the palette matrix (mtx), tex blocks (ids), 0x227 address, 0x228 count (nat_emit, pm_native.c:145 to 185). NAT_GRP_WORDS 38: 0x2C0 index 0 to 1, palette 2 to 19, tex0 20 to 25 (data 22 to 25), tex1 26 to 31 (data 28 to 31), index address 32 to 33, index count 34 to 35, trigger 36 to 37 (pm_native.h:12).

Which part depends on what [C]:
- Entry state of the segment: all of a (from ep_p and ep_ct), the z bit of d (geometry, set by dlc_set_geom at pm_dlc.inc.c:380 before dlc_seg), the gm of a.
- Render mode at list exit: only the othermode bits (r_decal, r_blend, r_atest). A static list cannot set them. dlc_compile (pm_dlc.inc.c:145 to 210) refuses any op except VTX, TRI1, TRI2, GEOMETRYMODE, NOOP and sync ops (DLR_OP). So SETOTHERMODE, SETCOMBINE, SETTIMG, SETTILE stay outside every static list. [C]
- Tile state for c: set before the list runs. Not changed by the list. [C]

## 2. Key numbers (census_mac01, frame 200, 65 static groups on the static path) [M]

Probe: at each static group taken on the static path, the render mode (pm_gpu_rec_rmode), the six C3D ctx words (pm_gpu_grp_get), the flush generation and the tex slot pair (pm_tex_cur_slots), read before and after dlc_static_group. Run result: census summary 64 / 810 / 53 (PASS, 53 is the distinct count of this build).

| Quantity | Groups where entry != exit |
|---|---|
| geometry word (geom) | 0 of 65 |
| render mode z, decal, blend, atest | 8 of 65 (4 times 1001 to 1000, 3 times 1000 to 1001, 1 time 1001 to 1010) |
| ctx words 0 to 5 (group key words of the record, rec_g0 layout, pm_gbi.c:3630) | 0 of 65 |
| flush generation (a flush of an open raw batch) | 1 of 65 |
| tex slot pair (pm_tex_cur_slots) | 34 of 65 |

Reading:
- The record key of rec_done (pm_gbi.c:3800 to 3806) takes the group words at capture start and the render mode at exit. For map groups the words are the same at entry and exit (0 of 65), so the key at entry gives the key that rec_done would give, except for the render mode part. 8 of 65 groups get a different key with the render mode at entry. So "if they never differ the problem disappears" is false for 8 groups. [M]
- The cause of the 8 render mode changes is not found. No static-path code writes r_atest, r_blend or r_decal (only pm_gpu_set_render, pm_gpu.c:2360, called from pm_gbi.c:679, 684, 706, which are interpreter ops). A grab path (pm_gbi.c:1220, 2616 grab_cur) also writes the state through pm_gpu_grab. Not checked. [U]
- The tex slot pair changes in 34 of 65 groups. The flush generation changes in 1 of 65. So the slot change is not a flush of a raw batch. Cause not found. [U]
- 65 static groups on the path give 64 census batches. One group merged into the open batch (grange, gr == 2). [M, I]

Consequence for the design:
- Key from the ctx words and the geometry at the group's entry (both stable through the group). [M]
- Refuse a draw (walk live) when its render mode at entry differs from exit. 8 of 65 are refused. Safe, no diagnosis needed. [M]
- Do not refuse on the tex slot pair (34 of 65 would refuse). Take the tex source from the batch (section 4). [M]

## 3. Learning rule (new template for a map group key) [C, I]

The record path already learns templates: rec_try (pm_gbi.c:3643) starts a capture with pm_gpu_rec_begin and rec_g0_set, rec_done (pm_gbi.c:3746) ends it with pm_gpu_rec_end, then nat_tmpl_add (pm_native.c:37) and nat_tmpl_strip (pm_native.c:112). Those functions expect one RecKey slot. For map groups:

1. In dlc_try (pm_dlc.inc.c:548), for a group g with g->sd and no template for its key, do the same capture as rec_try: rec_g0_set() (ctx words at group entry), pm_gpu_rec_begin(), run the group through dlc_static_group (live), pm_gpu_rec_end(&w, &n), then the same steps as rec_done from the render mode read on. Use a RecKey-free variant that takes the key directly (key = ctx words 6 to 10, geom at entry, render mode at entry, use_t1).
2. Refuse the capture when the window has state after the first draw. nat_tmpl_strip returns -3 for that (pm_native.c:112). Then the group keeps the walk.
3. Store the template id in the group (new field DlcGrp.tid, pm_dlc.inc.c DlcGrp struct) for the group's batch key, or keep a small table keyed by key (64 entries, nat_tmpl already).
4. Later groups with the same key take nat_emit (pm_native.c:145) with NatRec built from the NatKey per draw (nat_seg_add, pm_dlc.inc.c:288, E4b mirror).

Note: the batch words of a live group are read from the GPU command buffer by pm_gpu_rec_end. That needs the group to draw in raw mode with rec_cap set. Check that pm_gpu_rec_end works for a non-model window (rb_buf, rb_kick checks, pm_gpu.c:2489). [I]

## 4. Texture pair, live path versus record path (gap 4) [C, M]

- Record path: nat_tex_id (pm_native.c:200) takes the blocks from the template words 22 to 25 and 28 to 31 (rec_done, pm_gbi.c:3803 to 3805). The template holds the blocks of the learning batch. nat_emit reads nat_tex[r->tex0] (pm_native.c:173 to 174).
- Live map path: the tex slot of each batch is pm_tex_cur_slots at flush (pm_gpu.c:1053). In 34 of 65 groups the pair changes inside the group (section 2).
- Change needed: the NatRec for a map draw takes tex0 and tex1 from the slot pair at that draw's queue time, as the record does, and refuses the draw when a template block differs from nat_tex[] for that slot. The one-line change: in nat_seg_add (pm_dlc.inc.c:288) store the slot pair from pm_tex_cur_slots(&a, &b) in NatKey, and compare it with the template slot pair. [I, not measured]
- Whether the template block words equal the words of the slot at queue time: not measured. The gate in E5 measures it (census tex multiset).

## 5. Exact edits (each one agent, about 30 tool calls)

Common: build `/c/Python314/python 3ds/tools/gen_map_restore.py`, `PM_LANE=nds1 PM_ROW=<row> sh 3ds/build.sh`, then `git checkout -- 3ds/src/map_restore_table.c`. Census: `PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_mac01 -Row <row> -Build nds1 -Try`. Reference now: census_mac01 64 / 810 / 53 (measured in this design, PASS). census_toad 26 / 338 / 23.

E5-1. Key at entry in the builder. File pm_dlc.inc.c, nat_seg_add (line 288). Add to NatKey the tex slot pair (pm_tex_cur_slots) and the render mode at entry (pm_gpu_rec_rmode). Add a refusal flag nk_refuse set when the render mode at entry differs from the render mode after dlc_seg. Remove nothing. Gate: census_mac01 64 / 810 (no draw change); nat_host_test.sh passes; log line NATB draws unchanged.

E5-2. Learning capture for a group key. File pm_dlc.inc.c, new static function dlc_learn (near dlc_static_group, line 337), called from dlc_try (line 613) for a group with g->sd and no template. Uses pm_gpu_rec_begin, rec_g0_set, pm_gpu_rec_end, nat_tmpl_add, nat_tmpl_strip. Store the template id per group (new field in DlcGrp, file pm_dlc.inc.c struct, and the key). Gate: census_mac01 64 / 810 (the learn capture draws live, no change to draws); log count of learned templates (LRN line); nat_host_test.sh.

E5-3. Template emit for map groups. File pm_dlc.inc.c, dlc_static_group (line 337): when the group has a template and every draw passes E5-1 checks, build NatRec by nat_seg_add and call nat_emit (pm_native.c:145) and pm_gpu_rec_play (pm_gpu.c, ~2617) instead of the batch loop. Keep the walker for refused draws (not a stub: it is the fallback for the 8 refused groups of section 2). Gate: census_mac01 64 / 810 / 53; census_toad 26 / 338 / 23; tex+tri multiset equal to the walk (census tuples, cen_put); host tests.

E5-4. Shot gate. No code. Run viewed-region diff of build3ds/azt/shots/mac01t frames 500 and 504 (baseline) against the E5-3 build with 3ds/tools/region_diff.py. Expected 0 px on the viewed region. The census gate alone does not pass. A person reads the picture. Gate: 0 px differ, or the difference is explained by a named draw.

E5-5. Stereo. Run yard_stereo_r (stereo scene) and compare the right eye with the baseline. nat_eye_copy (pm_native.c:189) uses nat_mtx[eye]. Gate: the right eye matches the walk on the same frame.

E5-6. Delete the old path only when E5-3 gate passes on all groups with a template. Not in this E5 (the refused groups keep the walker, section 2: 8 of 65).

Host tests: `sh 3ds/tools/nat_host_test.sh` and `sh 3ds/tools/ndt_host_test.sh`. Add a case for the E5-1 refusal (render mode entry != exit) in nat_host_test.c.

## 6. Open points

- Cause of 8 render mode changes inside static groups (section 2). Diagnosis for an investigation agent, not for the code agent.
- Cause of 34 tex slot changes inside static groups (section 2). Same.
- Whether pm_gpu_rec_end works for a map group window (E5-2). Not tested.
- Whether the template block words of a map group equal the slot words at queue time. Not measured.
- Azahar starts used on this worktree for E5: 1 (ME5). Budget reset not needed (ME5 row, 1 of 8).
