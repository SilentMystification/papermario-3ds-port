# Post-PO3 path: CPU-side RSP and RDP work, what can move, ranked options

Written 2026-10-20. Read-only plan. No build. No Azahar run. Line numbers are from the working tree at 0ffb72520.
All numbers are Azahar guest ms (emulated ARM11 clock), not O3DS ms. Azahar has no cache model. The O3DS is slower (docs/plans/PO3.md:16, EST). Use the numbers for direction and ranking only.

## 0. Sources of the numbers

| Tag | Log | Switches | Use |
|---|---|---|---|
| GA | 3ds/logs/perf/po3/ga_noaudit.txt | phase phasecsv loglevel=debug fixedseed cpulimit=80 noaudit, pm_debug_has cache | per-phase means, full cycle, 8513 drawn frames (frames with w > 2000 ms dropped) |
| GP | 3ds/logs/perf/po3/gp4.txt | as GA plus gbiprof | 28 windows of 300 frames: draw_tri sections and the 8 most expensive op codes per window |
| CT | 3ds/logs/perf/po3/c_t_off.txt | PO3c run, gpuvtx off | gbi 19.53 ms per frame. This is the "19.4 ms" basis of the request |
| PT | docs/HISTORY.md, PT01 "Table 1" | phase, mac_00 and kmr_20 | older per-scene numbers (before PO1, PO2, AU1) |
| SR | 3ds/logs/perf/speed_ref_855.txt | recipe in its lines 1 to 2 | host-time A/B. 3ds/logs/perf/speed_recipe.txt does not exist at that path. 3ds/tools/perf_speed.py is not committed |

Timer cost: one tick read is 1.336 us in Azahar (`phase: on` line). gbiprof adds 1 tick read per op (pm_gbi.c:1839-1844) and 1 per lap in draw_tri (pm_gbi.c:1085). This doc subtracts that cost where it says "corrected".
One step = one drawn frame. The step runs inside pm_gfx_retrace of the drawn iteration (pm_main.c:287, 324). `pm_step_calls` counts steps (pm_phase.c:172).

## 1. Inventory of CPU-side RSP and RDP work per step

GA means per drawn frame. Phases are exclusive (a nested phase takes its time from the outer phase). gb = PH_GBI self time.
GA totals: wall 34.98, idle 1.90, fbegin 12.62, logic 0.43, dl build 0.59, gbi 12.29, gpu prep 0.71, gdraw 2.54, tex 0.87, audio join 1.20, rom 0.48, yay0 0.03, fend 0.29, gcache 0.00, other 0.31.
Counters (GA per frame): triangles out 341, vertex slots 1529, draws 135, texture misses 0.9, texture KB 7.2, ROM reads 10.1, audio ticks 2.1. GP window line: gbiops 5734, tri 1808, texcall 217, dbgcall 1941 (one demo window).

| # | Work | Where | Measured | Counter or switch |
|---|---|---|---|---|
| 1 | Display-list walk and op dispatch (is_host check, gbiprof and phasegbi branches, switch) | pm_gbi.c:1826-1850 | Not measured alone. 5734 ops per frame | `gbiops` (phase) |
| 2 | G_VTX: copy 16 B per vertex, S/T scale, vert_color, PO2 transform (16 mul-add) | pm_gbi.c:1874-1939 | GP section gvtx 1.02 ms. GP op 01: 1.38 ms at 120 per frame, corrected 1.22 ms | `gbiprof`, `phasegbi` (g_vtx), `vtxload` |
| 3 | Vertex lighting | vert_color pm_gbi.c:548 | Not measured alone. 0.1% of triangles are lit (PO3a) | po3a `lit` |
| 4 | Texgen | pm_gbi.c:1926 | Not measured alone. 0% of triangles (PO3a) | po3a `tgen` |
| 5 | G_MTX, G_POPMTX, viewport | load_mtx pm_gbi.c:1430 | 0.2 ms (request basis). GP op d8 (POPMTX) corrected 0.17 ms. Op da (G_MTX) is not in a top-8 list | `phasegbi` (g_mtx) |
| 6 | draw_tri section A: vert_live checks, po3a, off_screen_check, box_fold_sync, comb_audit | pm_gbi.c:1147-1173 | GP 3.93 ms, corrected 2.67 ms. comb_audit runs only at loglevel debug (pm_gbi.c:956). noaudit removes 0.75 ms of gbi (12.29 against 13.04) | `gbiprof` section A, `noaudit` |
| 7 | draw_tri: stat counts, ensure_tile, dual_bind | pm_gbi.c:1174-1188 | GP ens 1.31 and dual 1.76, corrected 0.05 and 0.50 ms | `gbiprof` |
| 8 | Trivial reject and cull from PO2 clip positions (PO3b2) | pm_gbi.c:1189-1222 | GP early 1.87, corrected 0.61 ms. 64% of triangles are rejected | `gbiprof`, `stats` (o) |
| 9 | Clipping: near clip clip_near_k at w 0.05, ClipV build | pm_gbi.c:685, 712 | Not measured alone. Inside GP section old | `stats` (c = cut triangles) |
| 10 | Triangle setup on the CPU path: xform_vtx, uv, colour overrides, G_ZS_PRIM, pack, put4 | pm_gbi.c:1239 to the end of draw_tri, pm_gpu.c:542 | GP old 2.45, corrected 2.0 ms (kept triangles only) | `gbiprof` section old |
| 11 | Raw Vtx path (gpuvtx): ring copy, index append, uniforms | gpu_emit pm_gbi.c:1104, pm_gpu.c:1084-1117 | Off by default. On: +0.5 to +1.7% host time (SR) | switch `gpuvtx` |
| 12 | Texture tile setup and cache lookup at G_SETTILESIZE | pm_gbi.c:2347-2507, flush at 2483, pm_tex_load at 2488 | GP op f2: 3.11 ms at 133 per frame, corrected 2.93 ms. It includes texture misses (row 13) | `gbiprof` op f2, `texcall` |
| 13 | Texture decode, tile swizzle, upload on a miss | pm_tex.c:418 on | GA tex 0.87 ms per frame. PT: max 42.8 ms in one frame at a map entry | phase `tex`, `texmiss`, `texKB`, `texmax` |
| 14 | Texture cache lookup on a hit: FNV of the palette per call, linear scan of slot_hi slots | pm_tex.c:402-417 | Not measured alone. Inside row 12 and row 17 | none |
| 15 | TMEM emulation: LOADBLOCK, LOADTILE, LOADTLUT, SETTILE, SETTIMG, ldrec | pm_gbi.c:2266-2346 | GP op f5 (SETTILE) corrected 0.26 ms, fd (SETTIMG) 0.22 ms, f0 (LOADTLUT) 0.03 ms. f3 and f4 are not in a top-8 list | `gbiprof` |
| 16 | Combiner: G_SETCOMBINE, decision cache dec[128], plan build | pm_gbi.c:2043, sync_combine 935, pm_comb.c | GP op fc: 1.01 ms at 105 per frame, corrected 0.87 ms | `gbiprof` op fc |
| 17 | Texture rectangles (2 flushes each), fills | pm_gbi.c:2146-2265, pm_gpu.c:1255-1330 | GP op e4: 2.95 ms at 85 per frame, corrected 2.84 ms (includes its draws) | `gbiprof` op e4 |
| 18 | Blender and other modes: SETOTHERMODE, sync_render, pm_gpu_set_render | pm_gbi.c:2024, 473 | GP op e3 corrected 0.36 ms. e2 is not in a top-8 list | `gbiprof` |
| 19 | GPU state build per batch: texenv stages, depth, blend, scissor, uniforms | flush pm_gpu.c:206-418 | GA gpu 0.71 ms | phase `gpu` |
| 20 | C3D_DrawArrays and C3D_DrawElements (citro3d emits dirty state here) | pm_gpu.c:456-508 | GA gdraw 2.54 ms at 135 draws (18.8 us per draw) | phase `gdraw`, `draw` |
| 21 | Data cache flush of the vertex pool, once per frame (PO1) | pm_gpu.c:1009-1013 | GA gcache 0.00 ms | phase `gcache` |
| 22 | FrameBegin wait (previous frame on the GPU, screen transfer) | pm_gpu.c:980-982 | GA fbegin 12.62 ms. Not CPU work. On the O3DS it is the PICA limit, if any | phase `fbegin` |
| 23 | FrameEnd submit | pm_gpu.c:1031-1033 | GA fend 0.29 ms | phase `fend` |
| 24 | Off-screen targets (V05, X02), off_snapshot | pm_gpu.c:162-204 | Not measured alone. Slots in use per frame are counted | `stats` (sl) |
| 25 | Frame grabs (B2): flush, C3D_FrameSplit, GX_TextureCopy, copy quad | pm_gpu.c:857-905, pm_gbi.c:143-152 | GA demo windows: 0 grabs. Count and split per frame are logged | b7 line (pm_gbi.c:1479), `stats` (sp, gr) |
| 26 | Previous-frame readback D03 (CPU detile of 320 x 240) | pm_main.c:213-230, pm_gpu.c:752-770 | Only on a load of an older frame buffer. Time per capture is logged | phase `fbread`, b7 "swirl capture" |
| 27 | F03 depth query (CPU read of 1 pixel of the last finished depth buffer) | pm_depth.c:10, pm_gpu.c:1331 | Time per query is logged | b7 "F03 query" |
| 28 | G_MODIFYVTX screen vertex (underwater) | pm_gbi.c:1941-1955 | Not measured. 0 screen vertices over the cycle (PO3a) | po3a `scr` |
| 29 | RSP audio microcode in C plus the audio engine | pm_rsp_audio.c, pm_audio.c, thread on core 1 (AU1d) | Core 0 join wait GA 1.20 ms. The core 1 time is not measured: `audiostat` "job" is the wall time from kick to done, it includes the time slices that cpulimit takes away | phase `audio`, `audiostat` |
| 30 | ROM read and Yay0 decode, texture cache invalidate per read | pm_os.c:293-336, pm_map.c:345-375 | GA rom 0.48, yay0 0.03 ms | phase `rom`, `yay0` |
| 31 | Switch look-ups in hot paths (pm_debug_has) | pm_gbi.c:480, 482, 497, 712, 940, 1323, 1346, 1354, 1371, 2173, 2243, pm_main.c:240, 302 | 0.595 us per call with a switch string (phase init line) x 1648 calls per frame (GA window mean) = 0.98 ms. Real play has no switch string and returns at once (pm_log.c:97) | `dbgcall` |
| 32 | Per-frame end work: counters, vert_ring reset | pm_gbi_end_frame pm_gbi.c:1467-1540 | Not measured | none |

Request breakdown of 19.4 ms (CT basis): other ops plus dispatch 7.2, draw_tri 6.4, C3D draw flush 2.5, G_VTX 1.0, G_MTX 0.2. The GP corrected op list explains most of the 7.2 ms: f2 2.93, e4 2.84, fc 0.87, e3 0.36, f5 0.26, fd 0.22, d8 0.17. Two GP op entries are artefacts: df (G_ENDDL, 16.0 ms) holds the time from the end of one list to the first op of the next list, and 06 (G_TRI2, 15.1 ms) holds the draw_tri lap reads.

Basis note: CT (19.53 ms gbi) and GA (13.04 ms gbi with the audit, 12.29 ms without) use different switch sets. 3ds/logs/perf/speed_ref_855.txt line 15 says: do not compare the two sets. GP minus its tick reads gives about 13.1 ms (26.45 ms gb, minus 5734 op reads x 1.336 us = 7.66 ms, minus about 944 x 4.5 lap reads x 1.336 us = 5.7 ms). So GP corrected matches GA. Section 1b uses the GA basis.

## 1b. Split of the GBI cost: parse, translation, flushes

Scope: the full CPU path of the display list at loglevel debug, GA basis: gbi self 13.04 + gpu prep 0.71 + gdraw 2.54 + tex 0.87 = 17.16 ms per frame. At loglevel warn (real play, no audit, no switch string) it is about 15.4 ms.
Tags: M = measured (GP corrected or GA phase). E = read-only estimate from the code and the counts. GP op times are wall time from one op to the next. So a draw that an op causes is charged to that op.

### Dispatch loop (pm_gbi.c:1826-1850), read result

- No function pointer table. One `switch (op)` (a jump table). draw_tri and load_mtx are direct calls. This part is cheap.
- Per op, before the switch: `guard++`, the end test, `is_host((u32)pc)` (pm_gbi.c:258-267: 4 global loads and up to 6 compares), the load of w0 and w1, `n_ops++`, `gp_on()` (load and branch), `pm_ph_fine` (load and branch). E: 15 to 25 instructions per op, x 5734 ops = 0.3 to 0.6 ms. Avoidable part: is_host on every op. pc changes only at list entry, G_DL and G_ENDDL, so one check there is enough. E: 0.1 to 0.2 ms.
- The avoidable cost is in the handlers, not in the loop. No handler has a fast path for a repeated value:
  - G_SETCOMBINE (2043-2095): decodes 20 fields and calls sync_combine each time, also when w0 and w1 are the same as before.
  - G_SETPRIMCOLOR (2096), G_SETENVCOLOR (2124), G_TEXTURE (2018), G_SETCONVERT, G_SETKEYR, G_SETKEYGB, the cycle-type part of G_SETOTHERMODE_H (2040), G_SETTILESIZE of tile 1 and box_fold_sync (464): each calls sync_combine. sync_combine (935-952) runs sync_legacy, one uncached pm_debug_has("nocomb") (940), decide (881: FNV of 6 words, then a linear scan of up to 128 entries if the last hit differs), pm_comb_colours and pm_gpu_set_plan (a memcmp of the whole PmPlan, pm_gpu.c:1158).
  - sync_render (473-525) runs for every kept triangle on the CPU path (pm_gbi.c:1341): 3 pm_debug_has calls (nomix, nofog, rmoff), img_role and 6 pm_gpu_set_* calls. Its inputs (omode_l, omode_h, blendc, fogc, cimg) change only at a few ops.
  - draw_tri section A runs off_screen_check (img_role, pm_gbi.c:119) and box_fold_sync for every triangle. box_fold_sync calls tile0_white (432-446), a texel scan of tile 0, on every triangle while a box window combiner is set (box_prod and td[1].valid).
  - dual_bind (1015-1023) hashes 11 words for every triangle of a TEXEL1 plan.
  - G_SETTILE of tile 0 (2290-2293) and G_SETTILESIZE (2483) call pm_gpu_flush with no compare. A gDPLoadTextureBlock macro gives both ops, so a reload of the same texture breaks the batch two times.
  - G_SETTILESIZE (2488) calls pm_tex_load each time: an FNV over the whole palette (pm_tex.c:403-406, 512 bytes for CI8) and a linear scan of the live slots (pm_tex.c:407-417), also for the texture that is bound now.

### Split table (ms per frame, GA basis, loglevel debug)

| Item | ms | Tag | (a) parse, dispatch, repeated checks | (b) N64 to PICA translation | (c) batch flush and draw |
|---|---|---|---|---|---|
| Op loop before the switch, 5734 ops | 0.3 to 0.6 | E | all | | |
| pm_debug_has with a switch string, 1648 calls (inside the rows below, mostly sync_render and sync_combine) | 0.98 | M | all (test runs only) | | |
| comb_audit (loglevel debug only) | 0.75 | M | all (test runs only) | | |
| draw_tri section A without the audit: vert_live, po3a branch, off_screen_check, box_fold_sync | 1.9 | M | all | | |
| dual_bind | 0.5 | M | all (key rehash per triangle) | | |
| Early reject and cull (RSP clip and cull rules) | 0.61 | M | | all (geometry) | |
| CPU triangle setup (xform_vtx, uv, colour overrides, pack, put4) and sync_render | 2.0 | M | sync_render 0.4 E without its switch calls | 1.6 E (Vtx to PICA float vertex) | |
| G_VTX (copy, S/T scale, vert_color, PO2 transform) | 1.0 | M | | all (geometry) | |
| G_MTX, G_POPMTX | 0.2 to 0.4 | M (d8 0.17), E (da) | | all | |
| G_SETTILESIZE f2 | 2.93 | M | lookup of the bound texture 0.4 to 0.8 E | decode and upload of misses 0.87 M, tile window logic and first lookup 0.6 to 1.0 E | about 30 to 40 draws x 24.1 us = 0.7 to 1.0 E |
| G_TEXRECT e4 | 2.84 | M | | handler, uv and quad build 0.6 to 0.8 E | 85 draws x 24.1 us = 2.05 E |
| G_SETCOMBINE fc and sync_combine | 0.87 | M | repeated pairs, share not measured | combiner decode and plan | plan-change flushes inside |
| G_SETOTHERMODE e3, G_SETTILE f5, G_SETTIMG fd | 0.84 | M | | render mode and TMEM state | the SETTILE flush is inside f5 |
| Ops outside the GP top-8 lists (fa, fb, e2, da, f3, f4, db, dc, d9, d7) and the end-of-frame batch | about 2.0 | E (remainder) | not split | not split | not split |

Draw cost (M): gpu prep 0.71 + gdraw 2.54 = 3.25 ms for 135 draws = 24.1 us per draw. A draw is charged to the op that causes its flush. So the 3.25 ms sits inside the f2, e4, f5, fc rows and in the remainder. It is not added again.

Flush counts: not measured. The counter C_FLUSH exists (3ds/include/pm_port.h:54, name `flush` in pm_phase.c:19) but no code adds to it, so the `phase count` line always shows `flush 0/0`. PT01 (older build, mac_00) counted 784 flush calls and 120 draws per frame: most flush calls find an empty batch and cost little. The cost is the batch break, not the call. Which state changes set the same value again: not measured. The setters in pm_gpu.c:1119-1214 already compare and return on an equal value. G_SETTILE tile 0, G_SETTILESIZE, the texrect and fill paths (pm_gpu.c:1258, 1269, 1297, 1315) and G_SETOTHERMODE_H filter change do not compare (the filter change compares).

### Answer: translation cost against avoidable overhead

Of about 17.2 ms per frame (GA basis, loglevel debug):
- Work that the N64 data needs for the 3DS (translation and RSP emulation): about 6.5 to 8.5 ms. Geometry 3.4 to 3.6 ms (G_VTX 1.0, reject and cull 0.61, triangle setup 1.6, matrices 0.2 to 0.4). Texture decode and upload 0.87 ms (M). Tile, combiner, render mode and TMEM state 1.5 to 2.5 ms (E). The draw calls that a minimal batch count still needs, about 1.0 to 1.5 ms (E).
- Avoidable overhead in our own code: about 5.0 to 7.5 ms. Test-run only: 1.7 ms (pm_debug_has 0.98, comb_audit 0.75), real play does not pay it. Repeated per-triangle checks: 2.5 to 2.8 ms (section A 1.9, dual_bind 0.5, sync_render 0.4). Lookup of the texture that is bound already: 0.4 to 0.8 ms. Batch breaks that a compare or a merge can remove (same texture reload, texrect per draw): 1.0 to 2.0 ms. Dispatch is_host per op: 0.1 to 0.2 ms.
- Not attributed: about 2.0 ms (ops outside the top-8 lists).
So in real play (warn) about 3.3 to 5.8 ms of the about 15.4 ms is avoidable overhead (EST). The rest is translation work. The texrect and same-texture flush share depends on counters that do not exist yet (row PG0, item 4).

## 2. Can each item move to the GPU or to core 1

Facts that apply to all rows:
- The N64 RCP runs in parallel with the N64 CPU. The game keeps two display contexts (src/main_loop.c:15, toggled at 185 and 283). So a display list and the matrices in its context stay valid for one more step.
- The port runs the list at once and in step: nuGfxTaskStart calls pm_gbi_run (pm_nusys.c:74-91). nuGfxTaskAllEndWait is empty (pm_nusys.c:19).
- The PICA has no compute stage. Only vertex work and per-pixel work can move to it.
- citro3d is not thread safe. All C3D_* and GX_* calls must stay on one thread.
- Core 1 runs audio (AU1d) and the log writer. Default share: cpulimit 30 (pm_log.c:443-446), so 10 ms per 33.3 ms step. Tests use cpulimit=80 (26.7 ms per step). AU1e (O3DS test of 30, 50, 80) is open.

| # | GPU | Core 1 | Verdict |
|---|---|---|---|
| 1 dispatch | No (control flow) | Only as a whole interpreter (option O8) | Neither alone |
| 2 G_VTX | Done as PO3b raw path, no gain in Azahar | With the interpreter only | Keep on CPU |
| 3 lighting | Yes, shader fixed light (PO3d). Gain near 0 (0.1% lit) | No | Not worth it |
| 4 texgen | Yes in the shader. 0% of triangles | No | Not worth it |
| 5 matrices | Already a uniform on the raw path | No | Keep |
| 6, 7 section A, bind | No (state logic) | No | Make cheaper on the CPU (O3) |
| 8 reject and cull | Yes (PICA clipper and C3D_CullFace, PO3e). Risk R2, R3 of PO3.md. The CPU reject saves vertex and draw work, so a move adds GPU load | No | Keep on CPU |
| 9 near clip | Yes (GPU clipper). Pixel change near the camera (V18, V27 near-plane history) | No | Keep on CPU |
| 10 triangle setup | Done for plain triangles (PO3b, PO3c), no gain | With the interpreter only | Keep the CPU path as default |
| 12, 14 tile and cache lookup | No | Shares the cache with core 0 invalidations (pm_os.c:335, pm_map.c:375, pm_main.c:191) | Make cheaper (O1) |
| 13 decode and upload | Partly: GX_DisplayTransfer can swizzle a linear RGBA8 image. Format conversion (CI, IA, I4) stays on the CPU | Yes as a prefetch at map load. A miss in a frame needs the result in the same frame, so a core 1 decode then needs a wait | Prefetch later (O9) or bake (T1) |
| 15 TMEM | No | No | Keep |
| 16 combiner | Already PICA texenv. The decision is CPU | No | Make cheaper (O5) |
| 17 texrects | Already GPU quads | No | Batch them (O2) |
| 19, 20 state and draws | No | Only with all of citro3d on core 1 (O8 variant) | Cut the count and the dirty state (O2, O4) |
| 24, 25 off-screen, grabs | Already GPU (render to texture, GX_TextureCopy in the command queue) | No | Keep |
| 26 D03 readback | Yes in part: render the target rotated into an off-screen RGB5A1 target and GX_DisplayTransfer to linear memory. Only pause and battle-entry frames | No | Low priority |
| 27 F03 depth | Reads the last finished frame already (one-frame delay as on the N64) | No | Keep |
| 29 audio | No (PICA has no compute). ndsp mixing on the DSP changes the PCM hash (AU1 plan) | Done (AU1d) | Keep |
| 31 switches | n/a | n/a | Cache in static ints (O6) |

Readbacks and effects that need a result in the same frame or the next step:
- Frame grab B2 (water ripples, crystal ball, bulb glow, motion_blur_flame): same frame, inside the GPU queue. No CPU wait. Safe on any thread that owns citro3d.
- Off-screen slot read in the same frame (V05 pause pages, X02 darkness, FX2a flame): same frame, GPU only.
- F03 depth: the game reads DepthCopyBuffer at src/model.c:4476 in the display-list build of the next step. pm_depth_query writes it while the list runs.
- D03 previous frame: pm_cfb_capture_prev (pm_main.c:213) reads the finished PICA colour buffer with the CPU. The game reads frame buffer memory with the CPU at src/background_gfx.c:343 (gfx_transfer_frame_to_depth, pause background) one step later, and builds lists from the older buffer at src/screen_render_util.c:44-63 and src/state_battle.c:43.
- Texture cache invalidation from core 0 work (ROM read pm_os.c:335, Yay0 pm_map.c:375, z-buffer pm_main.c:191).

## 3. Per-step budget: what exists, what is missing

Existing:
- `phase` (pm_phase.c:87-143): every 2 s one `phase W` line with wall and cpu (wall - idle - fbegin) avg, p95 and max, and the counts "wall over34" and "cpu over33" for the window.
- `phasecsv` (pm_phase.c:157-163): one `pc` line per drawn frame. 3ds/tools/phase_report.py:106-107 prints "frames with cpu work over 33.3 ms: N (P%)" over the whole log. Full cycle GA: 518 of 8513 frames (6.1%) have cpu over 33.3 ms. Wall is over 33.4 ms in 33.3% of the frames (fast run, wall includes the Azahar GPU wait). CT: 2867 of 8465 (33.9%) cpu over 33.3 ms.
- `pacestat` (pm_main.c:250-262): per second, steps, drawn frames, vblanks and the vblank interval histogram 1:2:3:4+. On a paced run (not `fast`, real frame limit) the intervals 3 and 4+ are the steps over 33.3 ms. The histogram array pace_iv is cumulative, but the line prints only the change of the last second.
- 3ds/tools/perf_speed.py (not committed): guest fps per segment from the `pc` w column.

Missing:
1. A cycle total of late steps on a paced run. Smallest change: pm_main.c:257-259, add `total 1:%u 2:%u 3:%u 4+:%u` with pace_iv[0] to pace_iv[3] to the `pace:` line (switch `pacestat`). One format string and 4 arguments. The last `pace:` line of a hardware run then gives the share of steps over 33.3 ms for the whole cycle.
2. GPU busy time per frame. fbegin mixes the PICA time, the screen transfer and the vblank. If the O3DS is GPU bound, no CPU option helps. Smallest change: pm_gpu.c, under switch `phase`: read pm_ticks at C3D_FrameEnd (line 1032) and in a gspSetEventCallback(GSPGPU_EVENT_P3D, cb, NULL, false) callback, sum the difference, add a counter C_GPUBUSY to cname in pm_phase.c:19 and print it at the end of the `pc` line (after atick, pm_phase.c:158). The regex of phase_report.py:19 and perf_speed.py:7 is not end-anchored, so a new last field is safe. About 15 lines.
3. Core 1 time of the audio job per step (needed for O8). Smallest change: pm_audio.c, record the kick-to-done ticks of the last job (au_j_t0 at line 179, the done point at line 164) in a global, and print it as a second new last field of the `pc` line (switch `phasecsv`). About 6 lines. It is wall time on core 1, so it is an upper bound of the CPU time.
4. Flush count and cause, same-value state sets, sync_combine and sync_render call counts (needed for O1, O2, O4, O5 and for the section 1b split). Smallest change: see row PG0 item 4 (switch gbiprof, pm_gpu.c:207 and the flush call sites, pm_gbi.c report at 1519-1534). About 45 lines.

Row PG0 in section 6 adds items 1 to 4.

## 4. Avoidable overhead in the whole per-step path

Scope: core 0 per step (game logic, display-list build, port layer, GBI, GPU, texture, audio join) and core 1.
GA per-step split of core 0 cpu (20.46 ms = wall 34.98 - idle 1.90 - fbegin 12.62): gbi path 17.16 (section 1b), audio join 1.20, ROM 0.48, dl build 0.59, logic 0.43, other 0.31, fend 0.29.
So in Azahar the GBI path is 84% of core 0 cpu. Game logic and the dl build are 1.02 ms together. Avoidable work in game logic can save at most that in Azahar terms.
Tags: M = measured, E = read-only estimate from the code and the counts, G = guess (no count, or O3DS cost model only).

| ID | Overhead | file:line | ms per step | Tag | Fix |
|---|---|---|---|---|---|
| A1 | Audio join wait at the default core 1 share (cpulimit 30) | pm_log.c:443-446 (default 30), join pm_audio.c:191-198 | 8.68 at 30 against 0.74 at 80 | M (SR, speed_ref_855.txt line 4) | Default cpulimit 80. Needs AU1e (O3DS test by the user) |
| A2 | Unconditional flush and texture lookup at G_SETTILE tile 0 and G_SETTILESIZE | pm_gbi.c:2290-2293, 2483, 2488 | 1.0 to 2.0 | E (op f2 2.93 M) | Compare the tile key, skip when equal (O1) |
| A3 | Palette FNV per lookup and linear slot scan | pm_tex.c:402-417 | inside A2 | E | Hash at G_LOADTLUT, bucket index (O1) |
| A4 | Per-triangle state checks before the reject: off_screen_check, box_fold_sync (tile0_white texel scan for box windows), po3a branch, stat block | pm_gbi.c:1162-1185, 432-465 | 1.9 | M (section A corrected, without audit) | Memoize per state change, move the stat block after the reject (O3) |
| A5 | sync_render for every kept triangle, 3 switch look-ups in it | pm_gbi.c:1341, 473-525 | 0.4 (warn), 1.0 (test runs) | E | Run on a dirty flag set by G_SETOTHERMODE, G_SETCIMG, G_SETBLENDCOLOR, G_SETFOGCOLOR (O3) |
| A6 | dual_bind rehash of 11 words per triangle of a TEXEL1 plan | pm_gbi.c:1015-1023 | 0.5 | M (dual corrected) | Dirty flag set by tile 1 ops (O3) |
| A7 | sync_combine on every PRIM, ENV, TEXTURE, KEY, CONVERT op and every SETCOMBINE, with no compare | pm_gbi.c:2022, 2094, 2100, 2105, 2110, 2116, 2127, 935-952 | 0.2 to 0.5 | E (op fc 0.87 M, other ops not measured) | Compare the inputs that the plan uses, skip when equal (O5) |
| A8 | One draw per texture rectangle (flush before and after) | pm_gpu.c:1294-1319, 1255-1272 | 0.5 to 1.2 | E (85 texrects, 24.1 us per draw M) | Batch rectangles with equal state (O2) |
| A9 | Full GPU state rebuild per batch: TexEnvInit of every used stage, depth map, blend, alpha test, scissor | pm_gpu.c:244-418 | 0.3 to 0.8 | E (gpu 0.71 M plus part of gdraw 2.54 M) | Dirty-state compare, cached TEV block per plan (O4) |
| A10 | Colour float divides in put4, 4 per emitted vertex | pm_gpu.c:564-567 (MA1 P7) | about 0 in Azahar, 0.4 on the O3DS | G (1529 vertex slots x 4 x 19 cycles VFP divide) | 256-entry float table, same values (O11), or u8 colour attribute (O12) |
| A11 | 40 B float vertex for the CPU path (x, y, z, w, u, v, r, g, b, a) | pm_gpu.c:542-567, attr_cpu | not measured | G | u8 x4 colour attribute (28 B), scale in the shader (O12) |
| A12 | Texture miss path: CPU expand to scratch, then a CPU swizzle per texel with tiled16, then C3D_TexLoadImage copies again | pm_tex.c:429, 461-505 | part of tex 0.87 | M (tex phase), split not measured | GX_DisplayTransfer swizzle or pre-converted data (O13, T1) |
| A13 | One linearAlloc per texture miss and C3D_TexDelete per eviction | pm_tex.c:449, 247-250 | not measured | G | Slot pool by size class |
| A14 | Textures live in linear FCRAM. VRAM holds only targets and grabs | pm_tex.c:449 C3D_TexInit, pm_gpu.c:196, 802, 873 | not measured (GPU side) | G | Hot textures in VRAM (O13) |
| A15 | is_host check on every op | pm_gbi.c:1828, 258-267 | 0.1 to 0.2 | E | Check at list entry and at G_DL only |
| A16 | pm_debug_has in hot paths with a switch string | pm_gbi.c:480, 482, 497, 712, 940, 1323, 1346, 1354, 1371, 2173, 2243, pm_main.c:240, 302, 352, 353, 366, 372, pm_gpu.c:997 | 0.98 in test runs, about 0 in real play | M | Static ints (O6) |
| A17 | comb_audit at loglevel debug | pm_gbi.c:956, 1172 | 0.75 in test runs, 0 in real play | M | Done for real play (0ffb72520). Test runs can add `noaudit` |
| A18 | Texture cache invalidate scans every live slot on each ROM read (10 reads per frame) and each Yay0 decode | pm_os.c:335, pm_map.c:375, pm_tex.c:295-323 | under 0.1 | E | Skip when the range is outside the min and max cached address |
| A19 | Per-frame end work: stats snprintf every 30 frames without the switch, vert_ring memset | pm_gbi.c:1496-1504, 1536 | under 0.05 | E | Put the snprintf under the stats switch |
| A20 | D03 CPU detile of 320 x 240 with a Morton index per pixel | pm_gpu.c:759-769 | only in capture frames | M (b7 swirl capture time) | GPU path (O10) |
| A21 | G_VTX calls vert_color for every vertex, also unlit (0.1% lit) | pm_gbi.c:1925, 548 | under 0.1 | E | Inline the unlit test |
| A22 | Game logic and dl build math (MA1 D sites) | src/npc.c, collision.c, cam_math.c and others | at most 1.02 (all of logic and dl build) | M (upper bound) | See section 4d |
| A23 | Log calls in play paths | pm_log.c:266-267 | about 0 | E (vlog returns before any format work when the level is off) | None |

No per-frame heap allocation was found in pm_gbi.c, pm_gpu.c or pm_tex.c hot paths (only gm_tab once at pm_gbi.c:1907, debug shot rows at pm_gpu.c:731, 941).

## 4b. One-time conversion of assets to 3DS-native form

The user accepts an offline or first-run conversion. Equality rule: the converted data must give the same picture. ETC1 is lossy, so it fails the rule. RGBA5551, RGBA4444, LA4, LA8 are exact only where the N64 format fits them.

| Item | What it removes | Share of the measured ms | What stays dynamic | Equality gate | Size on SD, memory | Effort |
|---|---|---|---|---|---|---|
| T1. Textures pre-decoded and pre-swizzled to the PICA tile order (the exact bytes that pm_tex.c uploads today) | expand and swizzle on a miss (pm_tex.c:429, 461-504) | of tex 0.87 ms per step (M): the decode and swizzle share is not measured, G: 50 to 80%. Peak frames at map entry (PT max 42.8 ms) fall most. The lookup and the upload copy stay. SD read on the O3DS can cost more than the decode (G) | CPU-written rasters and palettes (flash palettes actor_rendering.c, npc.c, V16b), imgfx, frame buffer and z-buffer images, off-screen slots, shaded and dual textures (dkey, lut path) | Bytes equal: run 2 decodes and compares (memcmp) every baked texture once. texmiss, texKB per frame equal. Golden 0 differences (same bytes) | G: 10 to 40 MB on SD (CI4 becomes 4 times larger as RGBA5551, power-of-two padding). No new RAM if loaded into the same 4 MB cache. A per-map pack in RAM costs up to the map working set (peak 1,209 KB, docs/MEMORY_BUDGET.md H03) | First-run bake in the game (no host decoder to keep equal): 200 to 300 lines |
| T1b. Smaller exact formats: I4 and IA8 as LA4 instead of LA8 | half of the texture bytes and upload time for these formats | not measured (texKB per format not counted) | none | golden 0 differences only if the PICA expands 4 bit as the RDP does (4 bit to 8 bit by replication on both, G) | saves VRAM and linear memory | 30 lines, no offline step needed |
| T2. Static map geometry pre-converted: Vtx to a PICA vertex buffer, display lists to pre-built PICA command lists per model and render state, index buffers | per-triangle CPU work for static models: G_VTX, reject, cull, triangle setup, state translation | G: 2 to 4 ms of the 17.16 ms (static map models are a share of 944 triangles per frame, the share is not counted). The raw Vtx path (PO3b) is already the native 16 B layout and gave no gain in Azahar (SR) | the 16 files that write Vtx at run time (docs/plans/H06a.md Cause 1), entity and NPC models, sprites, effects, HUD, texture pan (SETTILESIZE scroll per frame), fog, model visibility and transform groups changed by scripts, translucent sort by distance (model.c), readback effects (B2 grabs, X02, F03, D03) | golden 10 scenes, statehash equal (logic unchanged). The counters triout and draws change by design (the GPU clips), so C cannot be equal. Near-plane pictures change (PO3.md R2, V18, V27) | G: 5 to 20 MB on SD for all maps. RAM: one map set of vertex and command buffers, G: 0.5 to 2 MB linear | 1500 or more lines, tool plus hooks in model.c. High risk |
| T3. Combiner and render state precompiled to TEV and state blocks keyed by the N64 state | the per-batch texenv rebuild (A9) and the 24-sample plan check on a new combo (decide, pm_gbi.c:899-930) | 0.3 to 0.8 ms (A9, E). The new-combo check is not measured (dec misses are rare after warm-up, E) | colour constants (prim, env, keys) change per draw, so the block holds the stages and the constants are set apart | C equal, golden battle intro pause1 | none on SD (a run-time cache gives the same result), under 10 KB RAM | 80 lines. An offline step gives no extra gain over a run-time cache |
| T4. CI textures expanded once | the CI expand on a miss | already done once per (img, tlut, palette hash) key (pm_tex.c:403-416). Re-expansion happens only when the palette changes | flash palettes per frame | n/a | n/a | none |

Conclusion for 4b: T1 is the only conversion with a small, testable experiment and an exact gate. T2 has the largest gain guess but the highest effort and risk, and the PO3b result shows that the native vertex layout alone gives nothing in Azahar. T2 waits for O3DS numbers that show the GBI path is the limit.

## 4c. PICA200 and 3DS strengths: used or ignored (from the code)

| Strength | State in the port | Where |
|---|---|---|
| Few large draws with few state changes | Partly. 135 draws per frame (M). Each texrect and each fill is its own draw. SETTILE and SETTILESIZE break batches with no compare | pm_gpu.c:1258-1319, pm_gbi.c:2290, 2483 |
| Sort draws by state, keep N64 order where depth or blend needs it | Not used. N64 order is kept for all draws | pm_gpu.c flush call sites |
| Vertex buffers in linear memory, native attribute format | CPU path: 40 B floats with colour divides. Raw path (gpuvtx, off): native Vtx 16 B | pm_gpu.c:542-567, 456-461 |
| Index buffers, strips | Index buffer only on the raw path (C3D_DrawElements). CPU path: 3 vertices per triangle, no reuse (a vertex is sent about 1.8 times, PO3.md:16). No strips | pm_gpu.c:461, 486 |
| Command buffer reuse | Not used. citro3d builds the list each frame | pm_gpu.c:981, 1032 |
| Native texture formats | RGBA5551, LA8, RGBA8 are used. LA4, L4, A4, RGBA4444, ETC1 are not used | pm_tex.c gpu_fmt |
| VRAM placement | Targets, off-screen slots, snapshots and the grab copy are in VRAM. Textures are in linear FCRAM | pm_tex.c:449, pm_gpu.c:196, 802, 873 |
| GX DMA for copies | Used for grabs and snapshots (GX_TextureCopy). Not used for texture swizzle or for the D03 readback | pm_gpu.c:203, 894, pm_tex.c:461-505, pm_gpu.c:759 |
| One cache flush per frame | Used (PO1) | pm_gpu.c:1009-1013 |
| Early depth test | Off | pm_gpu.c:416 |
| Stencil | Used for the B3b depth mark only | pm_gpu.c:397-401, 495-497 |
| GPU clip and cull | Not used on the CPU path (CPU reject, C3D_CullFace NONE) | pm_gpu.c:444, pm_gbi.c:1199-1215 |
| Second core | Audio and the log writer on core 1 | pm_audio.c:138, pm_log.c:442-447 |

Early-z, VRAM textures and GPU cull help only when the PICA is the limit. That is not known (missing counter 2 in section 3).

## 4d. MA1 math sites (docs/plans/MA1_list.md)

Cost model (G, not measured on the O3DS): VFP11 single divide and sqrt about 19 cycles each (blocking), multiply 1 cycle issue. ARM11 has no integer divide: __aeabi_idiv about 30 to 60 cycles (MA1-R note). 1 us = 268 cycles. Azahar does not model divide cost (MA1_list.md section 1), so no math site can show a gain in Azahar.
Status: done P13, P14, P15, P18, P20 (MA1-R: counters, statehash and audiohash equal, +0.1% host time, no change). Skipped P16 (guRotateRPYF already uses the sin_rad and cos_rad tables, so Q-MA1-P16 in 3ds/QUESTIONS.md:77 has no site left) and P17 (VFP has vsqrt, a bit trick changes the float bits).

| Site | Path | Calls per step | Basis of the count | ms per step (O3DS cost model) | Hot path of the profile | Note |
|---|---|---|---|---|---|---|
| P1 vert_color /127.f x3 | G_VTX | about 2 (0.1% of vertices lit) | po3a lit | about 0 | G_VTX, lit only | Table gives the same values |
| P2 x/w per corner | draw_tri | 0 | read: after PO2 the CPU path sends clip positions (pm_gpu_vert_clip), no x/w divide remains | 0 | no longer present | Close |
| N1 (new) vert_uv /tw, /th | draw_tri, pm_gbi.c:600-601 | about 2046 (341 kept triangles x 3 x 2) | GA triout | 0.15 | draw_tri | Reciprocal per bind changes the last float bit (render only, golden) |
| P3 /255.f boost colour | draw_tri, pm_gbi.c:1411 | under 1 (7495 boost cases over the cycle) | po3a boost | about 0 | draw_tri | none |
| P4, P5 clip_near mod and divide | draw_tri | near-crossing triangles only | `stats` c, not summed | under 0.05 | draw_tri | none |
| P6 unpack8 /255.f x4 | pm_gbi.c:1565, decide and colour paths | tens | E | under 0.05 | dispatch (state ops) | Table gives the same values |
| P7 put4 /255.f x4 | pm_gpu.c:564-567 | 6116 (1529 vertex slots x 4) | GA vtxslot | 0.43 | triangle emit and 2D | Table gives the same values (O11) |
| P8, P9, P10 box and tile texel paths | pm_gbi.c box pattern | rare | not counted | under 0.05 | texture decode (rare path) | none |
| P11 texrect /tw, /th x4 | G_TEXRECT | 340 (85 x 4) | GP op e4 count | 0.02 | dispatch | none |
| P12 fill z /32767.f | G_FILLRECT | few | E | about 0 | dispatch | none |
| P19 asset table mod | ROM read, load only | load frames only | MA1 list | 0 per step | no | none |
| P21 resample mod | audio | compiler mask | MA1 list | 0 | audio | none |
| D1 model raster divides | load only | 0 per step | MA1 list | 0 | no | none |
| D2 model.c 1/outW per bounding box corner | dl build, src/model.c:2850, 4415, 4525 | G: 800 (100 models x 8 corners) | not counted | 0.06 | dl build | Feeds model cull and sort, render |
| D3 projected tex coords | dl build | few models | MA1 list | under 0.05 | dl build | none |
| D4 to D7 audio | core 1 | D5: one double divide per pull call, G: 48 per step | MA1 list | under 0.01 | audio (core 1) | Audio output changes with D5, audiohash gate |
| D8 to D12 game logic (NPC, atan2, collision, camera, battle) | logic | not counted | MA1 list | inside logic 0.43 M (Azahar, all logic) | logic only | statehash gate. Q-MA1-D8, D9, D10, D11-D12 stay PARKED (3ds/QUESTIONS.md:73-76) |
| D13 to D15 effects | logic and dl build | per particle | not counted | inside logic plus dl build 1.02 M | logic and dl build | Effect positions can feed logic: statehash gate |
| D16 libultra s64 | depends on caller | not counted | MA1 list | not known | not known | Read the callers first |

Verdict per group:
- Render math (P1 to P12, N1, D2, D3): SMALL. All open sites together are about 0.6 to 0.7 ms per step on the O3DS cost model (G), 0 in Azahar. Only P7 is large enough to count. P7, P6, P1 have an exact table form, so do them inside O11 with no gate risk.
- Logic math (D8 to D16): NOT WORTH IT now. All game logic is 0.43 ms and the dl build is 0.59 ms per step in Azahar (M). The math is a part of that. Each edit needs the statehash gate and a user answer to a parked Q-MA1 question. Revisit only if B7 shows a large logic phase on the O3DS.
- Audio math (P20 done, P21, D4 to D7): NOT WORTH IT. The per-sample loops have no divide. The audio cost is the mixer itself (MA1_list.md section 5).

## 5. Ranked list (gain per effort, one list for all kinds)

Gain is Azahar guest ms per step unless the tag says O3DS. "Size" is changed lines (E). All gates run with PM_ROW set.

Common gates:
- C: full cycle `phase phasecsv fixedseed statehash=10 cpulimit=80`, the counters triout, vtxslot, draws, texmiss, texKB per frame against the reference build (same method as 3ds/logs/perf/baseline_azahar_840.txt "determinism checks": 0 frames with a different value). romn and romB are excluded.
- H: statehash=10 lines equal to the reference. audiohash final line equal (pm_audio.c:17).
- S: host time of the full cycle (SR recipe), a gain under 1.5% counts as no change.
- G: golden.py with 1 to 3 named scenes.

| Rank | ID | Option | Gain | Tag | Size | Risk | Gates | Status |
|---|---|---|---|---|---|---|---|---|
| 1 | O0 | Default cpulimit 80 (pm_log.c:445) | 7.9 (join wait 8.68 to 0.74) | M | 1 | Low in Azahar. O3DS system load unknown | AU1e on the O3DS, H | Needs the user (AU1e) |
| 2 | O6 | Hot-path switch flags (A16) | 1.0 in test runs, 0 in play | M | 25 | Low | C, H | Bundle in PG0 |
| 3 | O1 | SETTILE and SETTILESIZE compare, palette hash at LOADTLUT, slot index (A2, A3) | 1.0 to 2.0 | E | 60 to 90 | Medium: V12c ldrec, FX3f mask period, V05 origin, V06 palette hash, H06a class 2 content keys | C (texmiss, texKB equal, draws equal or lower), H, G pause1 pause4 battle, then toad yard intro, S | PG1 |
| 4 | O3 | draw_tri trim: memoize img_role and off_screen_check, sync_render and dual_bind on dirty flags, stat block after the reject (A4 to A6). Keep off_screen_check and box_fold_sync before the reject (an off-screen slot must start even when all its triangles are rejected) | 1.0 to 1.6 | E (inputs M) | 40 | Low: V05, X02 begin order | C, H, G pause1 yard battle, S | Next row after PG2 |
| 5 | O5 | sync_combine compare (A7) | 0.2 to 0.5 | E | 25 | Low | C, H, G battle intro toad | Bundle with O1 in PG1 |
| 6 | O11 | Exact float tables for /255.f and /127.f (P7, P6, P1, A10) | 0 in Azahar, 0.4 to 0.5 O3DS | G | 20 | None (same values) | C, H, golden 0 differences | Bundle with O3 |
| 7 | O2 | Texrect and fill batching (A8) | 0.5 to 1.2 | E | 40 | Medium: draw order, B2 grab rows (R13c), fill_z | C (draws lower, others equal), H, G title fileselect pause2, then battle pause3 intro, b7 grab counts equal, S | After the PG0 flush counts |
| 8 | O4 / T3 | Dirty-state compare and cached TEV block per plan (A9) | 0.3 to 0.8 | E | 80 | Low to medium: TexEnvInit order, env_hi, off-screen resets | C, H, G battle pause1 intro, S | With O2 |
| 9 | T1 | Texture bake (first-run conversion experiment) | 0.4 to 0.7 average, peaks at map entry | G | 200 to 300 | Low for the picture (byte-equal gate). SD read time on the O3DS unknown | byte compare, C, golden 0 differences, S, phase_report over34 count in map-entry scenes | PG2 |
| 10 | T1b | I4 and IA8 as LA4 | not measured (texKB per format) | G | 30 | Low to medium (4 bit expansion rule) | golden all 10 scenes 0 differences | After T1 |
| 11 | O12 | u8 colour attribute on the CPU path, 28 B vertex (A10, A11) | 0.1 to 0.4 plus less GPU fetch | G | 40 plus shader | Medium (pm.v.pica, write each output once) | C, golden 10 scenes | Later |
| 12 | O13 | GX_DisplayTransfer swizzle on a miss, hot textures in VRAM (A12, A14) | part of tex 0.87, GPU side unknown | G | 60 to 100 | Medium: VRAM budget 2,798 to 3,822 KB in use (docs/MEMORY_BUDGET.md:37) | C, golden, `mem` line | After counter 2 shows GPU load |
| 13 | O9 | Texture prefetch at map load | 0 to 0.9, peaks | E | 80 | Medium: cache full (H03) | C, `texmax`, G toad yard | Later |
| 14 | A15 | is_host once per list and G_DL | 0.1 to 0.2 | E | 10 | Low (S30 bad dl guard must stay) | C | Bundle with any GBI row |
| 15 | O10 | GPU path for the D03 readback | capture frames only | M (b7) | 60 | Medium | G pause1 to pause4 | Later |
| 16 | T2 | Static geometry to pre-built PICA buffers and command lists | 2 to 4 | G | 1500 or more | High | golden 10 scenes, H | After B7 |
| 17 | O8 | Pipelined core 1 interpreter | at most 1.5, negative at cpulimit 30 | E | 800 to 1200 | High | see analysis | After B7 and AU1e |
| 18 | O7 | gpuvtx default, PO3d, PO3e | 0 or negative | M (SR) | 100 to 300 | Medium | PO3f | After the O3DS run |
| 19 | MA1 logic | D8 to D16 | inside 1.02 | M (upper bound) | per site | needs statehash | H | Parked (Q-MA1) |

Options 2 to 8 together (excluding O0): about 4 to 7 ms of the 17.16 ms GBI path in Azahar terms (E, the gains overlap: O1 and O2 both cut draws).

### O8 analysis: pipelined core 1 interpreter

What the change is: nuGfxTaskStart (pm_nusys.c:74) puts the list into a queue. A core 1 thread owns citro3d and runs pm_gbi_run, pm_gpu_begin, pm_gpu_end. Core 0 goes on with step N+1. Core 0 joins before it reuses the display context of list N (the step after next) or at a sync point.

N64 RAM state it must copy or freeze:
- Safe by the N64 design: the display context (src/main_loop.c:15) with its Gfx and Mtx, for one step.
- Must be frozen or joined: Vtx that game code writes at run time outside the display context (16 files, docs/plans/H06a.md "Cause 1"), CI palettes that the game writes (actor_rendering.c and npc.c flash palettes, V16b), the port model vertex copy (src/model.c:4145, mdl_local_gfx_copy_vertices), DepthCopyBuffer[16] (written by core 1, read by src/model.c:4476).
- Port globals that the interpreter reads: nuGfxCfb_ptr, nuGfxZBuffer, pm_wide, pm_interp (F06 records and pm_interp_repl), rm_frame, pm_frame_n, segbase. Each needs a copy per queued list.
- Port state with two writers: the pm_tex cache (invalidations from core 0), the phase timer (one stack, pm_phase.c:53), the PO3a and stats counters.
Most of the run-time Vtx and palette writes happen in the display-list build (gfx_draw_frame), not in step_game_loop. A join before the dl build of step N+1 removes most races. Then the overlap is only logic plus ROM: 0.43 + 0.48 ms in GA.

Effects that force a sync (core 0 waits for core 1 and the GPU):
- src/background_gfx.c:343 gfx_transfer_frame_to_depth (CPU read of nuGfxCfb[0] and [1]).
- Any texture load of an older frame buffer (pm_cfb_capture_prev, pm_gbi.c:2271).
- A map change (the game calls nuGfxTaskAllEndWait at src/main.c:113 and src/os/nusys/nugfxfuncset.c:7, now empty in the port).
- A ROM read or Yay0 decode into memory that list N reads (the invalidation must reach core 1 in order).

Core 1 share: GA core 1 work after the move = gbi 12.29 + gpu 0.71 + tex 0.87 + gdraw 2.54 + fend 0.29 = 16.7 ms. Audio adds its job on the same core (Azahar: 8.68 ms core 0 join at cpulimit 30, PT audio 8.0 ms per frame in mac_00 when it ran on core 0). At cpulimit 80 the app gets 26.7 ms of core 1 per step: 16.7 + 8 = 24.7 ms fits only just. At cpulimit 30 (10 ms) it does not fit. With audio moved back to core 0: core 0 = 0.43 + 0.59 + 0.48 + 0.31 + about 8 = 9.8 ms, core 1 = 16.7 ms of CPU at an 80% share = 20.9 ms of wall time. Today the cpu column is 20.46 ms. So in Azahar terms the pipeline gives no gain. It pays only if the O3DS shows a large logic plus dl-build time (B7). Do not start O8 before B7 and AU1e.

Variant O8b: only citro3d on core 1 (core 0 records draw commands, core 1 replays them). Moves gpu plus gdraw plus fend = 3.5 ms. Same core 1 share problem, same thread rules. Not ranked.

## 6. First 3 rows (queue format of 3ds/SLICES.md)

Order: PG0 (counters and switch flags, they make every later number honest), PG1 (O1 with O5, the largest measured CPU item), PG2 (T1, the conversion experiment). O0 needs the user (AU1e) and is not a row here. Then O3 with O11 and A15, then O2 with O4. O7, O8, T2 wait for the O3DS (B7, AU1e, PO3f). Optimization code needs S28 first (3ds/AGENT_RULES.md) unless the user lifts that rule for these rows as for PO3b.

```
[ ] PG0 | (sonnet) Budget counters, flush and same-value counters, hot-path switch flags. (1) pm_main.c:257 pace line: add total 1:%u 2:%u 3:%u 4+:%u from pace_iv (switch pacestat). (2) pm_gpu.c: GPU busy ticks per frame (FrameEnd at line 1032 to the P3D event of gspSetEventCallback), a new counter in pm_phase.c cname (line 19) and a new last field of the pc line (line 158). (3) pm_audio.c: kick-to-done ticks of the last job (au_j_t0 line 179, done line 164) as the next pc field. (4) Under switch gbiprof: PH_ADD(C_FLUSH, 1) at pm_gpu.c:207 (the counter exists in pm_port.h:54 and is never added), a cause code set before each flush() call in pm_gpu.c:1121-1322 and pm_gpu_flush 1198, same-value counts at pm_gbi.c G_SETCOMBINE 2043, G_SETPRIMCOLOR 2096, G_SETENVCOLOR 2124, G_SETTILE tile 0 2290, G_SETTILESIZE 2483, call counts of sync_combine 935 and sync_render 473, printed as `gbiprof flush` and `gbiprof same` lines in the report at pm_gbi.c:1519-1534. (5) O6: static ints for pm_debug_has at pm_gbi.c:480, 482, 497, 712, 940, 1323, 1346, 1354, 1371, 2173, 2243, pm_main.c:240, 302, pm_gpu.c:997. Plan: docs/plans/post_po3_path.md sections 3, 1b, 4 | one full cycle with phase phasecsv pacestat fixedseed statehash=10 cpulimit=80 and one with gbiprof added: the new fields and lines print, phase_report.py and perf_speed.py still parse, dbgcall per frame under 100, non-empty flushes per frame equal the draw counter within 1 percent, triout vtxslot draws texmiss texKB 0 frames differ against 0ffb72520, statehash equal
[ ] PG1 | (sonnet) O1 and O5: compare the tile key at G_SETTILE tile 0 and G_SETTILESIZE and skip the flush and pm_tex_load when it and a cache epoch (pm_tex_invalidate, G_LOADTLUT, G_LOADBLOCK, G_LOADTILE) are unchanged, palette FNV once per G_LOADTLUT, hashed slot index in pm_tex.c, sync_combine skipped when its inputs are unchanged. Use the PG0 same-value counts to confirm the hit rate first. Plan: docs/plans/post_po3_path.md O1, O5 | full cycle: texmiss and texKB 0 frames differ, draws per frame equal or lower, statehash and audiohash equal, golden pause1 pause4 battle pass, then toad yard intro pass, gbiprof op f2 under 1.5 ms and op fc under 0.5 ms per frame (GP basis 3.11 and 1.01), host time lower by more than 1.5 percent (SR recipe)
[ ] PG2 | (sonnet) T1 conversion experiment, texture bake. Switch texbake: on each texture miss of a plain texture (no lut, no dkey, not an off-screen or frame buffer image, not native fb memory) write the tiled upload bytes with a key (FNV-1a of the source texels and palette bytes plus fmt, siz, w, h, stride) to sdmc/3ds/PaperMario/texbake/ (one pack file plus an index). Switch texbaked: on a miss, look the key up and upload the stored bytes, else decode as today. Switch texbakecheck: decode as well and memcmp, log the mismatch count. Plan: docs/plans/post_po3_path.md section 4b T1 | run 1 texbake full cycle writes the pack and logs its size, run 2 texbaked texbakecheck full cycle logs 0 mismatches and the hit rate, texmiss and texKB 0 frames differ, golden toad yard battle 0 differences with texbaked, tex phase ms per frame and the over34 count of map-entry scenes (phase_report.py) reported against run 1, host time by the SR recipe reported (a gain or no change, both are a result)
```

Done check basis: gbiprof numbers are GP-style (gbiprof on, ticks included). Compare against GP: section A 3.93 ms, op f2 3.11 ms, op fc 1.01 ms.

## 7. Not measured

- Every number on the O3DS (B7, AU1e, PO3f are open).
- PICA busy time per frame (PG0 item 2).
- Core 1 CPU time of the audio job (PG0 item 3 gives an upper bound only).
- The split of draw_tri section A, the dispatch cost per op, the hit-path cost of the texture cache lookup, the split of gdraw into citro3d state emit and draw call.
- Flush count and cause, same-value state sets, sync_combine and sync_render call counts (PG0 item 4).
- Op codes outside the top 8 of each gbiprof window: da (G_MTX), e2, f3, f4, db, dc, fa and others (about 2.0 ms remainder).
- Texture misses split into map-entry frames and steady frames, the decode against swizzle against upload split of the tex phase, texKB per format.
- The share of static map models in the triangle count (T2).
- Call counts of the MA1 logic sites D8 to D16 and of D2.
