# Hardware re-rank of the speed options (O3DS, build 902)

Written 2026-10-20 by RERANK. Read-only plan. No build, no Azahar start, no console access.
Sources:
- HW: 3ds/logs/perf/hw_pull2/hw_o3ds_log.txt (build 902, O3DS, switches `audiostat pacestat logstat phase phasecsv loglevel=debug fixedseed statehash=10`, 8273 pc lines of 9006 steps).
- AZ: 3ds/logs/perf/merge_gate.txt (build 902, Azahar, same switches plus `cpulimit=80`, 8545 pc lines, 0 lines dropped). This is the same build as HW, so it replaces baseline_azahar_840.txt (build 840), speed_ref_860.txt (build 860) and po3a_azahar_845.txt (build 845) as the ratio basis. The older files give the counters only.
- Older Azahar A/B logs: 3ds/logs/perf/po3/dipb_gv_off.txt and dipb_gv_on.txt (build 870, gpuvtx off and on), po3/pg0d.txt (build 861, gbiprof flush causes), po3/gp3.txt (build 859, phasegbi).
- Scripts: 3ds/tools/hw_seg.py, and a scratch per-scene script (mode plus map from the `game mode` and `load map area` lines, mean of every pc column). The scratch script is not committed. Its method: a step belongs to the last mode line and the last map line before it. Mode 2 after a map load counts as the intro story (on HW the mode 16 line of the second intro is lost in the dropped lines).

Tags: M = measured on the O3DS. R = Azahar value times the hardware ratio of part 1. E = estimate from code and counts. G = guess.
All ms are per drawn step. One step is 33.3 ms at 30 steps per second.

## 0. Three findings that change the old ranking

1. Azahar phase ms are emulated ARM11 ticks, not host time. Proof: 3ds/logs/perf/full/cur_fast.txt and cur_normal.txt (build 840, fast and normal speed) give the same per-step values (pc 500: gb 84 and 84, pc 2500: gb 279 and 279, tx 114 and 113). Azahar adds about one tick per guest instruction. It has no cache model, no VFP latency and near-zero cost for system calls (gcache is 0.00 ms in AZ and 0.86 ms on HW). So an Azahar phase ms is an instruction count. The hardware/Azahar ratio of part 1 is the cost per instruction of that code on the O3DS. The labels "host time" in baseline_azahar_840.txt and docs/plans/hw_findings.md line 167 are wrong for the phase columns. Only the run total (speed_recipe.txt) is host time.
2. The GPU is not the limit. It is idle for most of each step (section 2.1). The CPU path is the limit: the cpu column (wall - idle - fbegin) is 53.0 ms in world steps.
3. The N64 game has its own frame skip (src/main.c:136, `if (gfxTaskNum < 3)` runs step_game_loop and skips gfx_draw_frame). The port always passes 0 (3ds/src/pm_nusys.c:111 `gfx_func(0)`), so the game never skips a draw and runs at 15.7 steps per second (52 percent game speed). Option FS1 (section 2.10) uses the N64 path. It does not cut ms, but it brings the game speed back near 30 steps per second.

## 1. Hardware cost model

### 1.1 What the log times and what it does not

Timed per step (pc line, 3ds/src/pm_phase.c:158): wall, idle, logic (lo), display-list build (dl), gbi self time (gb), gpu state setup in the flush (gp), texture (tx), audio join wait (au), rom, yay0, fbegin wait (fb), fend, other (ot), gcache, gdraw (gd), and the counters triout, vtxslot, draw, texmiss, texKB, romn, romB, atick, flush, dbgcall, gbiops, ajob. Phases are exclusive (pm_phase.c:52 to 67).

Not timed or not usable in this log:
- g_vtx, g_tri, g_mtx, g_op: 0 in every step. They need the switch `phasegbi` (pm_phase.c:34). So the split of gb by op class on HW does not exist.
- fbread: timed in the `phase W` lines (pm_phase.c:18), 0.0 in every window. No `pc` field.
- Core 1 work: the audio job CPU time, the log writer, and the system modules (GSP, FS, DSP run on core 1, the O3DS system core). The log has only the join wait (au), the `audio cost` and `audio wait` lines and the ajob counter (core 1 wall time).
- PICA busy time and the screen transfer: no counter (PG0 item 2 was not done, citro3d owns the P3D event). fbegin mixes the GPU wait, the transfer and the vblank alignment.
- The ROM and Yay0 split inside logic at map entry (ro is nested in lo there).
- About 730 pc lines (8 percent) are missing. The log writer on core 1 dropped 1207 lines (queue full, 120 `lines dropped` lines, the first at #1376).

Overhead of the test switches inside the HW numbers (E):
- comb_audit runs at loglevel debug (3ds/src/pm_gbi.c:1009, 1263). Azahar: 0.75 ms of gbi (post_po3_path.md row A17). HW: about 6 percent of gb, so 2.2 ms in world steps (R).
- Tick reads of the phase timer: 0.933 us each on HW (log line #5), about 900 per world step (draw, tex and gbi pushes and pops), so 0.8 to 0.9 ms (E).
- pc line format on the game thread: 0.1 to 0.3 ms (E).
- Total: about 3.3 ms per world step. A play build (loglevel warn, no phase) is about 49.7 ms cpu in world steps instead of 53.0 ms.

### 1.2 Counters: hardware against Azahar

The per-scene counters are equal within 1 percent in all 15 matched scenes. Examples (HW/AZ per step): world a1m4 triout 519/519, vtxslot 1974/1974, draw 121/121, gbiops 5390/5390. World a6m1 467/467, 1810/1810, 111/111, 4273/4266. Battle a18m1 963/966, 3259/3266, 126/126. Title 52/52, 721/728, 100/101, 1105/1112. Logos 5/5, 114/114, 17/17, 190/190. texmiss and texKB are equal per scene (a4m5 2/2 and 41/41). Steps 331 to 339 (intro NPC spawn) have equal vtxslot, draw, texmiss, texKB and romn on both targets (for example step 337: 2352, 185, 74, 552, 0).
So both targets do the same work per scene. The ratio of part 1.3 compares the same work. The statehash differs (HW f4000 1d03f66e, AZ bcd4e3ea) because the user skipped the first intro on HW. The demo scene order differs for that reason, so the comparison is per map, not per segment number.

### 1.3 O3DS ms per step per phase, and the hardware/Azahar ratio

World = 11 matched maps (a11m3, a2m2, a7m9, a17m4, a18m1, a1m4, a6m1, a4m5, a14m5, a6m5, a4m20), 2411 HW steps, 2381 AZ steps. Battle = 4 matched maps (a17m4, a18m1, a6m1, a14m5), 803 and 860 steps. Intro = intro story in area 5 map 5 (2578 HW steps from f4853 to f8044, 3153 AZ steps). Title = mode 2 without a map (1492 and 824 steps). Menus: file select (mode 14, 236 HW steps, AZ has no file select) and logos (312 and 312 steps).

| Phase | World HW | World AZ | ratio | Battle HW | ratio | Intro HW | ratio | Title HW | ratio | File select HW | Logos HW | ratio |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| cpu (wall - idle - fb) | 53.01 | 16.87 | 3.14 | 57.96 | 2.91 | 59.09 | 3.60 | 30.30 | 3.25 | 41.17 | 13.65 | 7.18 |
| fb (wait) | 9.83 | 15.05 | 0.65 | 9.65 | 0.71 | 9.58 | 0.67 | 9.80 | 0.59 | 8.58 | 16.15 | 0.97 |
| logic | 2.55 | 0.89 | 2.88 | 0.96 | 3.43 | 0.66 | 2.94 | 0.12 | n/a | 0.12 | 0.11 | n/a |
| dl build | 1.72 | 0.56 | 3.07 | 1.89 | 2.57 | 1.83 | 2.60 | 0.34 | 3.42 | 0.76 | 0.12 | n/a |
| gbi self (gb) | 36.03 | 10.27 | 3.51 | 36.42 | 3.42 | 40.97 | 4.37 | 14.37 | 4.83 | 25.11 | 1.36 | 4.49 |
| gpu state (gp) | 3.03 | 0.79 | 3.85 | 3.35 | 3.85 | 5.31 | 3.65 | 3.01 | 3.45 | 5.81 | 0.41 | 3.80 |
| gcache | 0.86 | 0.00 | n/a | 0.88 | n/a | 0.89 | n/a | 0.84 | n/a | 0.94 | 0.86 | n/a |
| gdraw (gd) | 3.19 | 1.49 | 2.14 | 3.46 | 2.13 | 5.16 | 2.12 | 2.99 | 2.05 | 5.83 | 0.43 | 1.87 |
| tex | 2.23 | 1.27 | 1.76 | 3.27 | 1.58 | 1.02 | 2.05 | 0.37 | 1.75 | 0.15 | 0.10 | 1.34 |
| audio join (au) | 1.00 | 0.38 | (2.64) | 0.77 | (3.64) | 0.93 | (1.56) | 6.20 | (2.38) | 0.52 | 8.29 | (88) |
| rom | 0.18 | 0.06 | 3.15 | 0.13 | n/a | 0.05 | n/a | 0.01 | n/a | 0.00 | 0.01 | n/a |
| yay0 | 0.05 | 0.04 | n/a | 0.06 | n/a | 0.02 | n/a | 0.03 | n/a | 0.00 | 0.01 | n/a |
| fend | 0.83 | 0.29 | 2.87 | 0.82 | 2.78 | 0.75 | 2.53 | 0.70 | 2.34 | 0.69 | 0.58 | 1.95 |
| other | 0.86 | 0.41 | 2.11 | 0.87 | 2.13 | 1.00 | 2.74 | 0.83 | 2.95 | 0.81 | 0.83 | 2.57 |

Notes:
- n/a: the Azahar value is under 0.05 ms, so the ratio has no meaning.
- The au ratio is in brackets because it does not compare the same thing. Azahar runs the audio thread on core 0 (merge_gate.txt #13006 `audio thread core 0`), the O3DS runs it on core 1 (HW #14203 `audio thread core 1`).
- fb is a wait, not work. Its ratio under 1 only says that HW steps are late.
- Map change (mode 5, 130 HW steps): cpu 43.91 (ratio 3.18), gb 30.95 (3.48). Demo end (modes 17 and 9): cpu 18.79, au 13.96.
- Whole cycle (hw_seg.py): gbi 31.59 ms per step on HW, 9.17 ms in AZ 902 (ratio 3.44). Window pc 1700 to 1800: 25.67 against 12.22 (the window is a different scene on each target, not comparable).

Spike steps (intro NPC spawn, equal counters on both targets):

| Step | HW wall | AZ wall | HW tex | AZ tex | tex ratio | HW gb | AZ gb | gb ratio | other |
|---|---|---|---|---|---|---|---|---|---|
| 331 | 125.9 | 98.5 | 0 | 0 | | 0 | 0 | | ro 40.6/28.6 (1.42), yay0 12.0/9.4 (1.28), au 6.0/0.7 |
| 332 | 197.9 | 119.3 | 82.1 | 61.6 | 1.33 | 51.9 | 17.1 | 3.04 | yay0 18.4/15.1, au 20.2/2.2 |
| 336 | 103.9 | 47.7 | 4.2 | 2.9 | 1.45 | 68.0 | 22.6 | 3.01 | |
| 337 | 200.2 | 109.0 | 113.3 | 84.6 | 1.34 | 58.1 | 15.8 | 3.68 | 74 misses, 552 texKB |

Cost classes on the O3DS (from the two tables):
- Streaming integer code (texture expand and swizzle, Yay0, ROM memcpy): 1.3 to 1.8 times the Azahar instruction count. Azahar predicts it well.
- Display-list interpreter (gb): 3.4 to 3.5 in 3D scenes, 4.4 to 4.8 in the 2D-heavy intro story and title (many texture rectangles: 130 to 156 G_TEXRECT per frame in the title windows of pg0d.txt, 36 to 67 in world windows).
- citro3d state setup (gp): 3.5 to 3.9. citro3d draw call (gd): 1.9 to 2.1.
- Game logic and dl build: 2.6 to 3.4.
- System calls: gcache is a fixed 0.73 to 0.94 ms per step for 17 to 186 draws (one GSPGPU_FlushDataCache per frame, pm_gpu.c:1127). It is an IPC to the GSP module on core 1. Azahar gives 0. A tick read is 0.933 us on HW and 1.336 us in Azahar (`phase: on` lines), so Azahar charges a system call more than the O3DS in this case.
- Per draw: gp + gd = 6.22 ms for 98 draws in world steps = 63 us per draw on HW (24 us in AZ). File select: 11.64 ms for 186 draws = 63 us.
- Per gbi op: 36.03 ms for 4026 ops = 8.9 us per op in world steps (AZ 2.7 us).

## 2. Re-rank with the hardware ratios

### 2.1 Real PICA load

- fbegin on HW is the vblank alignment, not a GPU wait. Steps with cpu 18 to 21 ms have fb 11.4 to 13.3 ms and land on 33.3 ms in 94 to 97 percent of cases. The sum cpu + fb stays at about 33 ms. A GPU that is slower than the CPU would make fb longer than the vblank remainder. It does not.
- On-time share by cpu ms (HW, wall at most 34.5 ms): cpu 25 to 28 ms 82 to 86 percent, cpu 29 ms 39 percent, cpu 30 ms 31 percent, cpu 31 ms 12 percent, cpu 32 ms 7 percent. The knee is at 28.5 ms of cpu, not at 33.3 ms.
- Cause of the knee: citro3d holds the GX queue in a frame and runs it at C3D_FrameEnd (pm_gpu.c:971 comment, C3D_FrameEnd at pm_gpu.c:1148). So the PICA starts only after the CPU finished the whole list. The GPU time of frame N overlaps only with the logic and dl build of step N+1 (about 4 ms on HW). The tail after that is about 4.8 ms (33.3 - 28.5). So the PICA render plus transfer is about 5 to 9 ms per frame (E). The PICA is idle for 75 to 85 percent of a 33.3 ms step, and for more than 90 percent of a 55 to 65 ms world step.
- Work on the PICA per world step: 98 to 121 draws, 360 drawn triangles, 1396 vertex slots of 40 B, 400x240 targets. This is far below the PICA rate (E).
- Conclusion: the GPU has room. Any work moved from the ARM11 to the PICA saves CPU time at about the gb ratio (3.5) and costs GPU time that the PICA has. GPU clipping, culling, vertex transform, batching, strips and index buffers all pay on HW. The one GPU cost that matters is the serial tail. Option KICK1 hides it.

### 2.2 Per-triangle CPU path against the raw path on the ARM11

CPU path per drawn triangle (3ds/src/pm_gbi.c:1196 to 1460): vertex index checks, clip-bit reject (vert_cb, line 1237), cull with 8 float multiplies (1242 to 1253), statistics, ensure_tile, dual_bind, 3 calls of xform_vtx into ClipV, near clip, colour and alpha overrides, sync_render, then 3 vertices of 40 B (10 floats) into the linear vertex pool. 73 percent of the triangle commands end at the reject (1633 tri commands, 446 drawn in the window of HW #4850). Per world step the CPU path writes about 43 KB of vertices (360 x 120 B). The ARM11 has a 16 KB data cache per core and the O3DS has no L2 cache. VFP11 has a long latency on dependent float chains. Azahar charges neither.
Raw path (switch gpuvtx, PO3b and PO3c): a 16 B Vtx copy per loaded vertex at G_VTX (about 22 KB per step), 3 u16 indices per triangle, a matrix per draw. It keeps the CPU transform at G_VTX (clip bits for the reject), the CPU reject and cull and all per-triangle state checks.
Azahar result (build 870, dipb_gv_off.txt against dipb_gv_on.txt, world 2622 steps): gb 25.05 and 25.07, gd 1.98 and 2.44. So the raw path removes as many instructions as it adds, and its draw call costs 0.46 ms more.
Hardware expectation: the removed part is float code at a high cost per instruction (gb class, 3.5). The added part is copy and index code at a low cost per instruction (streaming class, 1.3 to 1.8). Net gain of the existing gpuvtx switch on HW: -1 to +3 ms per world step (G). It costs no code to measure: one A/B window in Push 3 (section 4).
The large gain needs the full path (GV1): no CPU transform, no CPU reject and cull for plain batches, PICA clipping and C3D_CullFace. Then the CPU per triangle is the op decode and 3 indices.

### 2.3 Texture decode and upload

- Steady state: tex 2.23 ms per world step (1 to 2 misses, 14 texKB). Spikes: 82.1 ms (step 332) and 113.3 ms (step 337, 74 misses, 552 KB). HW rate: 552 KB in 113.3 ms = 205 ns per output byte. Azahar: 84.6 ms = 41 instructions per output byte. The decode is instruction bound, so Azahar predicts it (ratio 1.33).
- Cause in the code: expand() writes a linear image to scratch (pm_tex.c:153), then a byte loop computes the Morton index for every texel (tiled16, pm_tex.c:141 to 147, loops at 687 to 725), then C3D_TexLoadImage copies the result again (pm_tex.c:730).
- Fix TX1 (new): write 8x8 tiles straight from the source rows with a 64-entry Morton table, 16-bit and 32-bit stores, and decode into the texture buffer with no scratch copy. E: 3 to 6 times faster.
- GX_DisplayTransfer swizzle (part of O13): the GX queue is held inside a frame (pm_gpu.c:971), so a GX tiling job must run outside the frame or with a split. More complex than TX1 and it saves only the swizzle. Hold.
- VRAM tier (O13): saves GPU fetch time only. The GPU is not the limit. Hold.
- ETC1: lossy, fails the equality rule (post_po3_path.md 4b). Rejected.
- PG2 texture bake loader: Azahar showed no gain and map entry cpu 2 to 3.6 ms higher (file reads). On HW the SD read is slower again, and after TX1 the decode is cheap. The verdict HOLDS: closed.
- Prefetch at map load (O9) and async decode on core 1: after TX1 the spike is about 20 to 35 ms. A spread over the fade-in steps or a decode on core 1 (8 to 11 ms free per step, section 2.4) can take the rest. After TX1.

### 2.4 O8 (interpreter on core 1) with the audio thread on core 1

- Core 1 on the O3DS is the system core. The app gets cpulimit 80 percent = 26.7 ms per step (`cpu limit: set 80 (start)`, HW #1 and #2). GSP, FS and DSP services also run there.
- Audio: `audio cost` 7.3 to 8.5 ms per 60 Hz tick (HW #14162 to #14362), so 14.6 to 17.0 ms per step = 55 to 64 percent of the app share. Job wall time 29.5 ms per step at the title (HW #14203). Free share: about 8 to 11 ms per step, less the log writer in test runs.
- The whole GBI path on HW is gb + gp + gd + gc + tx + fe = 46.2 ms per world step. It does not fit in 8 to 11 ms. O8 is REJECTED on the O3DS (more firmly than the Azahar verdict). Variant O8b (citro3d only, gp + gd + gc + fe = 7.9 ms) would take all of the free share and starve the audio. Rejected.
- What fits on core 1: small async jobs with no citro3d call, for example texture decode for a prefetch (after TX1) or Yay0 decode of the next map sprites in the fade. On the N3DS (core 2 free) O8 is a valid option. Not measured.
- Audio join wait grows when core 0 gets faster: title steps (cpu 30.3 ms) wait 6.20 ms, logos (cpu 13.7 ms) wait 8.29 ms, world steps (cpu 53 ms) wait 1.00 ms. The kick-to-join window is 14.4 ms and the job is 29.5 ms (HW #14203). At 30 steps per second a world step will wait about 5 to 7 ms (E) unless AU2 decouples the audio. AU2 is part of the minimum set (section 3).

### 2.5 Ranked list (O3DS ms per world step, play-build basis 49.7 ms cpu)

Done before build 902 and inside the HW numbers: PO1, PO2, PO3b and PO3c (gpuvtx, default off), PG0 (O6), PG1 (O1), DIP1, DIP2 (O5), PG3a (O4), PG3b (O11), PG3c (A15), PG4, AU1f, HW3.

| Rank | ID | Option | O3DS ms saved per world step | Basis | Size (lines) | Risk | Gates | Azahar verdict and HW verdict |
|---|---|---|---|---|---|---|---|---|
| 1 | GV1 | Full GPU geometry path for plain batches: transform on the PICA, PICA clip, C3D_CullFace from the geometry mode, no CPU transform, reject and cull, index-only CPU work per triangle. Covers PO3d, PO3e, O7 and the remaining MA1 N1 divides | 8 to 14 | R: TRI plus VTX share of gb (PG4 ranking 42 and 10 percent, lap inflation corrected by 0.6 to 0.8) times gb 36.03, minus about 2 ms that stays (op decode, index, copy) | 400 to 700, plan first | High: near plane (V18, V27), cull rule V02, off-screen slot start (X02, V05, keep off_screen_check per op), matrix palette (PO3.md R1), depth range | C with new triout and draws (they change by design), golden 10 scenes, H, Azahar gb lower by 2.5 to 4 ms | REVERSED. The Azahar "no gain" of PO3b and PO3c came from a partial path that kept the CPU transform, reject and state checks. A full path also lowers the Azahar instruction count |
| 2 | DR1 | Draw cost: one BufInfo per frame and draws by offset, uniform shadow compare, texture bind compare, merge of texture rectangles and fills with equal state (O2) | 2 to 4.5 | M: 63 us per draw on HW. E: per-draw cost to 35 to 45 us, 10 to 30 merged draws of 26 to 64 texrect flushes per world frame (pg0d.txt) | 80 to 120 | Medium: draw order, B2 grab rows (R13c), fill_z | C (draws lower, others equal), H, golden title, fileselect, pause1, b7 grab counts equal | REVERSED for O2 (Azahar 0.5 to 1.2 ms, HW 2 to 4.5 ms) |
| 3 | KICK1 | Early GPU kick: C3D_FrameSplit after the world pass or every N draws, with a flush of the vertex range written since the last split | 0 ms of cpu, but the on-time knee moves from 28.5 to about 31 to 32 ms (2 to 4 ms more budget) | M: knee. G: shift | 30 to 50 | Medium: grab and off-screen order, extra flush per split (gcache is about 0.86 ms per call if the cost is per call) | golden, b7 grabs equal, HW on-time histogram | NEW. Azahar cannot show it (host GPU) |
| 4 | AU2 | Async audio: engine step in the join, render on core 1 from a command list, PCM ring | Avoids a rise of 5 to 7 ms of join wait at 30 steps per second | M: title and logos waits | plan, then 300 to 500 | High: engine state shared with game code | audiohash equal, underrun 0 | HOLDS, now in the minimum set |
| 5 | TX1 | Texture expand and tile in one pass (Morton table, 16-bit stores, no scratch copy, no C3D_TexLoadImage copy), old path under a check switch | 1.0 to 1.5 steady. Spikes: step 332 82 to 15 to 25 ms, step 337 113 to 20 to 35 ms | M: tex ms and texKB, 205 ns per byte | 120 to 180 | Low: byte-equal check | memcmp 0 mismatches, C, golden toad yard battle | NEW. Azahar predicts it (ratio 1.33) |
| 6 | O3r | Rest of the per-triangle and per-op state work: ensure_tile, dual_bind, sync_render on dirty flags, G_SETTILESIZE bound-texture lookup, G_SETCOMBINE decode | 1 to 2.5 | R: Azahar rest 0.3 to 0.7 ms times 3.5 | 40 to 80 | Low | C, H, golden pause1 yard battle | HOLDS, larger on HW. GV1 removes a part |
| 7 | GC1 | gcache: test the per-call cost of GSPGPU_FlushDataCache (0.86 ms per step for any draw count). If it is fixed IPC latency, flush by svcFlushProcessDataCache or once per two frames of ring use | 0 to 0.7 | M cost, G gain | 10 to 20 | Low to medium: SVC access of a 3dsx | HW only (Azahar gives 0) | NEW |
| 8 | T2 / DLC1 | Static geometry as pre-built PICA buffers and command lists (T2), or a run-time cache of translated command output keyed by display-list content and state (DLC1) | 4 to 10 on top of GV1 | G: the op dispatch that stays after GV1 (8.9 us per op on HW) for static map models | 1500 or more | High | golden 10 scenes, H | REVERSED in priority: it is the only family left that closes the rest of the gap. Plan after the GV1 HW numbers |
| 9 | O12 | u8 colour attribute, 28 B vertex on the CPU path | 0.3 to 0.7 (0.1 to 0.2 after GV1) | R | 40 plus shader | Medium | C, golden 10 scenes | HOLDS, low. GV1 takes most triangles off the CPU path |
| 10 | O9 | Texture prefetch at map load or async decode on core 1 | spikes only, 10 to 25 ms in spike steps after TX1 | E | 80 to 150 | Medium: cache budget, core 1 budget | C, texmax, golden toad yard | HOLDS, after TX1 |
| 11 | O13 | VRAM texture tier, GX swizzle | 0 cpu. GPU tail 0.5 to 1.5 ms less (only without KICK1) | G | 60 to 100 | Medium: VRAM budget | C, golden, mem line | HOLDS. The GPU is not the limit |
| 12 | MA1 logic | D8 to D16 | 0 to 0.5 | R: logic 2.55 ms on HW (ratio 2.88), math is a part | per site | statehash, parked questions | H | HOLDS, parked |
| 13 | PO3d | Lighting in the shader | about 0 (0.1 percent lit) | M (po3a) | 50 | Low | golden | HOLDS. Bundle into GV1 only if it is free |
| 14 | O8 | Pipelined interpreter on core 1 | negative on the O3DS | M: core 1 budget (2.4) | 800 to 1200 | High | none | HOLDS (rejected). N3DS only |
| 15 | PG2 | Texture bake loader | negative (SD reads) | M (Azahar), E (HW) | done, default off | none | none | HOLDS (closed) |
| 16 | FS1 | N64 frame skip: pass gfxTaskNum 3 to gfxRetrace_Callback when the last drawn step was late, so the game runs step_game_loop without gfx_draw_frame (src/main.c:132 to 140) | 0 ms. Game speed 15.7 to about 28 to 30 steps per second at 15 to 17 drawn frames per second | E: lo + dl 4.3 ms per step, the drawn step 50 ms | 40 to 80 | Medium: draw-coupled state, F06 interpolation records, audio kick per drawn step (pm_main.c:236 to 247), D03 and F03 one-frame readbacks | statehash equal between a forced skip pattern and no skip (Azahar), golden | NEW. Needs a user decision (drawn frame rate against game speed) |

Azahar "no gain" conclusions:
- REVERSED: PO3d and PO3e as part of GV1 (full path), O2 (texrect batching, inside DR1), T2 (priority), O7 (gpuvtx as the base of GV1).
- HOLD: O12 (small, superseded), O13 (GPU not the limit), O8 (core 1 budget), MA1 logic sites (logic is 2.55 ms on HW), PG2 loader (SD read cost).
- NEW only on HW: KICK1, GC1, TX1 priority, AU2 as a gate, FS1.
- MA1 render divides: VFP divide is about 19 cycles on the ARM11 and 1 tick in Azahar. PG3b removed the large sites. N1 (vert_uv /tw /th, about 2046 per step, pm_gbi.c:600) is about 0.15 ms (E). Bundle into GV1.

## 3. The target: 30 steps per second

Requirement on HW: the cpu column must stay under the knee (28.5 ms today, about 31 to 32 ms with KICK1) in most steps, not only in the mean.
Play-build basis per world step: mean 49.7 ms, p50 51.7 ms, p95 68.7 ms (HW p50 55.0, p95 71.9, minus 3.3 ms of test overhead).

| Set | Cut (ms) | World mean after | World p95 after (scaled) | 30 steps per second? |
|---|---|---|---|---|
| GV1 + DR1 + O3r + TX1 (steady) + GC1 + MA1 logic | 12.0 to 23.7 | 26.0 to 37.7 | 35.9 to 52.2 | Mean only at the optimistic end. p95 no |
| Same without AU2 | add back 5 to 7 | 31.0 to 44.7 | 41 to 59 | No |
| Same plus KICK1 (knee 31 to 32) | same | 26.0 to 37.7 against a 31.5 knee | 35.9 to 52.2 | Mean in light and average world scenes. p95 no |
| Plus T2 / DLC1 | 16.0 to 33.7 | 16.0 to 33.7 | 22.1 to 46.6 | Mean in most scenes. p95 only at the optimistic end |

The p95 column scales the p95 by the same factor as the mean (the cuts are per triangle and per draw, so they scale with the load). Heavy scenes stay above the knee in all rows: world a1m4 62.5 ms, a6m1 61.6 ms, a6m5 61.5 ms, battle a6m1 77.0 ms (tex 8.9 ms) on HW today. Light scenes reach 30 with the first row: a14m5 world 31.3 ms, a2m2 35.8 ms, title 30.3 ms.

Answer: 30 steps per second in every scene is not reachable on the O3DS with the listed options. A mean of 30 in most world scenes needs the full set (GV1, DR1, KICK1, AU2, TX1, O3r and T2 or DLC1). The realistic result of the first five rows is 20 to 25 drawn steps per second in world scenes (a mix of 2 and 3 vblank intervals) and 30 in light scenes. The minimum set for a 30 mean in average world scenes is GV1 + DR1 + KICK1 + AU2, with TX1 for the spikes. FS1 is the only option that brings the game speed near 30 steps per second soon, at a lower drawn frame rate. The N3DS (804 MHz, L2 cache, a free core 2) can reach 30 with less. Not measured.

## 4. Phased plan: first 5 rows and one push

Row order: HWM1 makes every later HW number cheap and complete. TX1 and DR1 (with KICK1) are medium size with a clear gate. GV1 is the largest gain and needs a plan and the HW split of Push 3. FS1 needs the user. AU2 is an open row (3ds/SLICES.md). Its plan can run in parallel (opus).

```
[ ] HWM1 | (sonnet) HW measurement lines that survive the log queue and an in-run A/B. (1) Switch phsum: one `phs` line per scene (mode and map) and per A/B state, at each scene change and every 300 drawn steps, with steps, the sums of every pc phase, cpu p50 and p95, counters, and an on-time histogram of cpu 24 to 35 ms (on-time and late counts per 1 ms). (2) Switch abtog=<state,state,...>:<n>: rotate the named switch sets every n drawn steps by the switch generation (PMDL), skip the first 2 steps of a window in the sums, write `abtog <k> <switches> f<frame>` at each change. Allowed states: base, gpuvtx, nodr1, nokick, notx1, phasegbi (pm_ph_fine). (3) Switch phasecsv=<from>:<to>: pc lines only for that step range. (4) 3ds/tools/phs_report.py: per scene and state tables, ratio to an Azahar phs log. Plan: docs/plans/hw_rerank.md section 4 | one Azahar full cycle with phsum abtog=base,gpuvtx:30 phasecsv=300:400: phs sums equal the pc-line sums of steps 300 to 400 within 1 percent, both states show in every world scene, 0 lines dropped, counters and statehash equal to merge_gate.txt with abtog off. HW metric (Push 3): every scene has a phs line, 0 phs lines dropped
[ ] TX1 | (sonnet) Texture expand and tile in one pass in pm_tex.c (expand 153, tile loops 687 to 725, upload 730): 64-entry Morton table, 8x8 tile writes with 16-bit and 32-bit stores from the source rows, decode into the texture buffer with no scratch pass and no C3D_TexLoadImage copy where the format allows. Keep the old path under switch notx1. Switch tx1check: run both and memcmp, log the mismatch count. Plan: docs/plans/hw_rerank.md section 2.3 | Azahar: tx1check full cycle 0 mismatches, texmiss texKB 0 frames differ, golden toad yard battle 0 differences, tex ms of steps 332 and 337 lower by 3 times or more (AZ 61.6 and 84.6). HW metric (Push 3, pc lines 300 to 400): tex of step 337 under 35 ms (Push 1: 113.3), step 332 under 25 ms (82.1)
[ ] DR1 | (sonnet) Draw cost and early GPU kick in pm_gpu.c. (1) One BufInfo for the vertex pool per frame, draws by first-vertex offset (BufInfo at 553 to 555, 578 to 580, 607 to 608). (2) Shadow compare for the uniforms of flush (319 to 340) and for C3D_TexBind. (3) Merge texture rectangles and fills with equal state into one draw (O2, pm_gpu.c tex_rect and fill_rect). (4) KICK1: C3D_FrameSplit after the world pass and every 64 draws, with GSPGPU_FlushDataCache of the range written since the last split. Switches nodr1 and nokick for A/B. Plan: docs/plans/hw_rerank.md sections 2.1 and 2.5 | Azahar: draws per step lower, gp+gd lower, triout vtxslot texmiss texKB 0 frames differ, statehash equal, golden title fileselect pause1 battle pass, b7 grab and split counts equal. HW metric (Push 3, phs base against nodr1 and nokick): gp+gd per draw under 45 us (Push 1: 63), on-time share at cpu 29 to 31 ms above 60 percent (Push 1: 12 to 39)
[ ] GV1 | (opus plan, then sonnet) Full GPU geometry path for plain batches: no CPU transform, reject and cull, PICA clip and C3D_CullFace, index-only CPU work per triangle, CPU path kept for lit, texgen, screen vertices, mixed matrix ids, depth images and debug switches. Plan input: Push 3 phs lines of the states gpuvtx and phasegbi (HW g_vtx, g_tri, g_op split). Plan: docs/plans/hw_rerank.md sections 2.2 and 2.5, PO3.md R1 to R7 | plan file exists with the slices. Code: Azahar gb lower by 2.5 ms or more per world step, golden 10 scenes within TOL, V18 pan and V02 yard cliff shots equal, statehash equal. HW metric (next push, phs): gb lower by 8 ms or more per world step
[ ] FS1 | (sonnet) USER DECISION FIRST. N64 frame skip: when the last drawn step was late (wall over 2 vblanks), pass gfxTaskNum 3 to gfx_func (pm_nusys.c:111) so gfxRetrace_Callback (src/main.c:136) runs step_game_loop without gfx_draw_frame, as the N64 does with 3 pending tasks. Skip FrameBegin and FrameEnd for that iteration and keep the audio kick per step. Switch fs1off for A/B, switch fs1force=<n> (skip every n-th draw) for Azahar. Plan: docs/plans/hw_rerank.md section 2.5 rank 16 | Azahar: fs1force=2 full cycle statehash equal to the no-skip run (if not equal, list the draw-coupled state and stop), audiohash equal, golden 10 scenes pass with fs1 off. HW metric (pace lines): steps per second 28 or more in world scenes, drawn frames per second reported
```

The one push for these rows (Push 3, after HWM1, TX1 and DR1 are on main). It replaces a separate push per row. It uses the pulled log.txt only (docs/plans/hw_batch.md section 2 method).

```
export O=<ip> B=<build>; sh 3ds/push_3ds.sh --no-build --ip $O -t 420 audiostat pacestat logstat phase phsum noaudit loglevel=debug fixedseed statehash=10 phasecsv=300:400 abtog=base,gpuvtx,nodr1,nokick,phasegbi:30
python3 -u 3ds/tools/ftppull.py $O o build3ds; cp build3ds/hw_o3ds_log.txt 3ds/logs/perf/hw_${B}_P3_pull.txt
python3 3ds/tools/phs_report.py 3ds/logs/perf/hw_${B}_P3_pull.txt > 3ds/logs/perf/hw_${B}_P3_table.txt
```

User hands: none. Do not press a button during the run. Push 1 lost the first intro because of a button press, so its statehash and scene order differ from Azahar. `noaudit` removes the 2.2 ms comb_audit cost of loglevel debug. `phasecsv=300:400` keeps the pc lines of the spawn steps only, so the line rate falls from about 30 per second to about 1 per second.

Log lines per row in the pulled log:
- HWM1: `phs <mode> <map> st <k> n <steps> | lo dl gb gp gc gd tx au ro y fb fe ot gv gt gm go (sums) | cpu p50 p95 | ok24..35 late24..35 | triout vtxslot draw texmiss texKB flush gbiops ajob`, `abtog <k> <switches> f<frame>`, `pc` lines 300 to 400.
- TX1: `tx1 f<frame> miss <n> KB <n> us <n> ns/B <n>` every 300 drawn steps (and `tx1check mismatch <n>` when the check switch is on). Read with: `grep -E "^#[0-9]+ pc 33[0-9] " <pull>` and `grep "tx1 f" <pull>`.
- DR1 and KICK1: `dr1 f<frame> merged <n> splits <n> uniform skips <n>` every 300 drawn steps. Read the phs lines of states base, nodr1 and nokick.
- GV1 input: phs lines of state gpuvtx (gb, gd against base) and state phasegbi (gv, gt, gm, go). The phasegbi state inflates gb by its tick reads (0.933 us each). phs_report.py subtracts gbiops x 0.933 us x 2.
- FS1 (later push, after the user decision): `pace:` lines (steps against drawn per second), `fs1 skipped <n> of <n>` every 300 steps.
- Always: `audio wait`, `audio cost avg`, `audio core probe`, `statehash`, `texstat`, `cpu limit`, `mem` lines (already present).

## 5. Azahar as a proxy

What Azahar predicts:
- Work counts: triout, vtxslot, draw, texmiss, texKB, gbiops, romn, flush are equal per scene within 1 percent (section 1.2). Any change that lowers a count lowers the HW time in proportion to the unit cost of section 1.3.
- Instruction counts: an Azahar phase ms is a guest instruction count (section 0). A change that removes instructions in one cost class saves about ratio x Azahar ms on HW. This holds well for streaming code (tex, yay0, rom spikes: 1.3 to 1.5) and for the gbi class in 3D scenes (3.4 to 3.5, cross-check: world ratios predict battle gb within 2.7 percent and map change within 0.8 percent).

What Azahar does not predict:
- A change that moves work between cost classes with no change of the instruction count (gpuvtx: Azahar 0, HW -1 to +3 ms, G).
- Memory and cache effects: working sets above 16 KB, writes to linear memory, the 40 B vertex stream. Azahar has no cache model.
- VFP latency and divides (19 cycles on HW, 1 tick in Azahar).
- System calls: gcache (0 against 0.86 ms), tick reads (1.336 against 0.933 us), SD and FS calls (PG2 loader, save), GSP IPC.
- Core 1: Azahar runs the audio thread on core 0. The audio join wait and the core 1 budget are HW only.
- GPU time: the PICA tail and the vblank knee (KICK1) are HW only.
- The 2D-heavy scenes: the world ratio underestimates the title gb by 27 percent and the intro story gb by 20 percent (texture rectangles cost more per instruction on HW).
Cross-check of a single-ratio proxy (world ratios applied to the AZ phases of other scene groups, measured HW au added): battle cpu 54.3 against 58.0 measured (-6 percent), title 26.2 against 30.3 (-14 percent), intro story 50.9 against 59.1 (-14 percent). Good for a rank, not for a 30 steps per second verdict.

Recommended proxy, in two steps:
1. Now, no push: add a column `o3ds_est_ms` to 3ds/tools/perf_az_full.py and hw_seg.py (and phs_report.py): 3.5 gb + 3.85 gp + 2.1 gd + 1.75 tx + 2.9 (lo + dl) + 2.8 fe + 2.1 ot + 0.86 (gcache per drawn step) + 1.4 (rom + yay0). Use gb weight 4.6 in scenes with more than 100 G_TEXRECT per step (title, intro story). Report au apart (HW only). A row passes the Azahar gate if o3ds_est_ms falls by the expected amount. Error: 6 to 14 percent on cpu (cross-check above).
2. After Push 3: a unit-cost model from counts. Push 3 gives per-op-class HW times (phasegbi state) and per-draw, per-texKB, per-vertex costs. Fit O3DS ms = a x gbiops + b x tri + c x vtxload + d x draw + e x texKB + f x romB + const per scene group, with the HW phs lines as the target. Then every Azahar run gives an HW estimate from counts alone (counts are exact in Azahar). Refit after a row that changes the per-op code (GV1, T2). A row that changes only counts (DR1 merges, TX1 has its own ns per byte) needs no refit.
Do not use the Azahar run total (host time) for HW decisions. It measures the host JIT and the host GPU.
