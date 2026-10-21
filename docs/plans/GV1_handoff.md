# GV1 handoff (full GPU geometry path). Written 2026-10-21. Commits bbfab7084 (a), 96b2d857f (b), 5ee5c3ec3 (c), sign fix and this file after.

## Design as built (switch gv1, default off; nogv1, gv1check, gv1box, gv1raw, gv1flip; abtog state gv1)
- G_VTX (pm_gbi.c, case G_VTX): an unlit load copies the 16 byte Vtx to the ring (pm_gpu_gvtx_load) and makes per vertex only the clip bits (vert_cb, bit 64 = w <= 0.05, near). No colour, no S T. vert_gvf tells what is valid. gv_xf() makes vert_p, vert_st, vert_c from the ring copy on demand.
- Triangle (draw_tri then gv_tri): reject by vert_cb AND (mask 0x2f). Fallback to the old CPU path for: near (bit 64), screen vertex (G_MODIFYVTX), lit or texgen, fog, depth image role (zmark), TEXEL1 plan, debug switches, mixed texture scale (tss), a refusal of pm_gpu_gtrip (decal2, wide UI, ring full), lit loads and a full ring (at load time). Counters in the `gv1` log line (every 300 frames).
- Cull: G_CULL_FRONT keeps N64 cross > 0, G_CULL_BACK keeps cross < 0. The PICA culls in window space. Mode = GPU_CULL_BACK_CCW or GPU_CULL_FRONT_CCW from the sign of the 3x3 determinant (x y w) of the post matrix (pm_gpu.c rebuild_3d, gv_wind). VERIFIED sign: ccw = (gv_wind > 0) == gv1flip, so the default is the flipped one. Golden yard, toad: 0 differences. gv1flip gives the first (wrong) sign back.
- State epoch (GV1b): gv_epoch goes up at every op except G_VTX, G_TRI1, G_TRI2, G_MTX, G_POPMTX, G_DL, G_ENDDL, G_MODIFYVTX (pm_gbi.c dispatcher), at pm_gbi_run start and at end of frame. The first triangle of an epoch runs the state prep (comb_audit, ensure_tile, dual_bind, sync_render, fallback reason, cull type, draw parameters per texture scale). Later triangles: reject check plus pm_gpu_gappend (3 index stores).
- Palette (GV1c): shader 3ds/shaders/pm4.v.pica, 21 matrices (84 uniforms). The palette index k is in the flag field (bytes 6 and 7) of the ring copy. Entry = post x game, composed in the flush. pm_gpu_gtrip adds matrices, writes the index, copies a vertex whose index must change or whose ring slot was already flushed. G_MTX calls pm_gpu_note_mvp (no flush unless viewport or ortho flag changed).
- A CPU triangle now closes an open raw batch first (pm_gpu_vert_clip, tri3d).

## Commands
- Options: Azahar options.ini has warp=osr. Set warp=0 for the dip run, restore after (backup first).
- Dip window: `PM_ROW=<row> sh 3ds/run_azahar.sh -F -t 600 -u "pc 1850 " -o <dir>/log.txt -d "fixedseed phase phasecsv=1650:1850 noaudit loglevel=debug gv1 shots shotwin=1740:2"` (12 s). Average pc 1702 to 1850 with a small script over the `pc` lines (gb gp gd, triout, draw = last columns).
- Work split: add `gbiprof`, read `gbiprof f1799 per frame` (laps early, emit, gvtx). Do not use it for ms (timer reads inflate).
- Golden: `PM_ROW=<row> python3 3ds/tools/golden.py --extra gv1 toad yard` then `battle pause1`. Without --extra the default must stay 0 differences (not run).
- Check: add gv1check to count gvcull, gvstale, gvtriout in the `gv1` line (Azahar only).

## Numbers (dip window, ms per step, noaudit)
| Variant | gb | gp | gd | triout | draws | weighted (3.5 gb, 3.7 gp, 1.9 gd) |
|---|---|---|---|---|---|---|
| base | 12.7 | 1.2 | 2.0 | 290 | 150.6 | 52.7 |
| GV1a box reject | 13.6 | 1.3 | 2.7 | 568 | 156.6 | |
| GV1b epoch | 11.5 | 1.3 | 2.7 | 568 | 156.6 | 50.2 |
| GV1c palette + box | 12.4 | 1.3 | 2.7 | 568 | 156.6 | 53.3 |
| GV1c per-vertex bits | 11.2 | 1.2 | 2.6 | 389 | 152.6 | 48.6 |
| gv1raw | 14.6 | 1.6 | 3.6 | 1075 | 192.6 | |
Golden with gv1: toad 0.00, yard 0.00, battle 2.56 percent (limit 3.0), pause1 0.06. Dip shots 0 px.

## Why gv1raw has +42 draws, and the batch breaks (pm_gpu.c)
A draw ends when flush() runs. Conditions: (1) any state setter that changes a value (pm_gpu_set_render, set_combine, set_mode, set_replace, scissor, set_tex1, plan, ...): one per material run, the main source. (2) pm_gpu_gtrip: a change of uvso, viewport, cmul, cadd, zmix or cull type (pm_gpu.c:1488), palette full (1490), CPU vertices pending (1487). (3) pm_gpu_set_mvp when viewport or ortho flag changes (1417). (4) a CPU fallback triangle (vert_clip flush). (5) texture bind change at flush. The base path drops runs whose triangles are all rejected, so their state change never happens as a flush. With no reject, every off-screen run keeps its state change and its draw. A vertex reload is NOT a break (ring indices are absolute). A matrix change is no break now.
## Ideas to make one epoch one draw
- Do not flush at state change: queue per-run state (a small struct) and draw lazily. Merge only runs with equal state (compare the struct, not the setter calls). Different state still needs its own draw.
- Cheap run reject without a transform: per-load bounds (min and max of ob, 6 compares per vertex) tested against the 6 planes with a matrix row bound (linear in the box, no 8 corners): reject the whole load. A run is empty when all its loads are rejected: no state prep, no draw. Box corners cost as much as per-vertex bits, but the row-bound test is 3 compares per plane.
- uvso, cmul, cadd, zmix per vertex or per index range (a uniform array indexed by the flag field, like the palette) removes the breaks of (2) when only these change.
- Reuse the draw: DrawElements has no per-range state, so state changes (texture, combiner) cannot be merged. This is the floor: one draw per distinct state, the base path has the same.

## Open items
Guard band for huge triangles (PICA clip of x and y at the frustum, not tested). Wide mode (gtrip refuses ortho matrices, not tested). decal2 and V16b (refused, CPU path). V18, V19 flicker scenes. Default-off golden. The command buffer percent with more draws. Hardware run (gate: gb falls 6 ms or more, Azahar estimate is about 4 ms weighted in the dip window).

## Budget state
GV1a: 4 starts used of the reset 4. GV1b: 3 of 8. GV1c: 4 of 4 (after reset). The orchestrator resets per row.
