# Resource topology of the O3DS port: actual, gaps, target

Written 2026-10-21 by TOPO (row PT17). Read-only plan. No build, no Azahar start, no console access.
Sources:
- HW 902: 3ds/logs/perf/hw_pull2/hw_o3ds_log.txt (O3DS, build 902, full cycle, Push 1) and docs/plans/hw_rerank.md part 1.3 (per-phase table of that log).
- HW 926: build3ds/hw_live_log_180.txt and 3ds/logs/perf/hw_926_P3_push4.out (O3DS, build 926, 143 lines: boot and logos only). The Push 3 A/B run of build 926 was played by hand (PT18), so no world-scene number of build 926 exists.
- AZ: 3ds/logs/perf/merge_au2.txt (Azahar, AU2 merge build, full cycle), docs/plans/AU2_design.md part 8.
- Plans: docs/plans/hw_rerank.md, hw_findings.md, hw_batch.md, GV1.md, post_po3_path.md, docs/MEMORY_BUDGET.md.
- Code at HEAD 9aa64c887: 3ds/src/pm_main.c, pm_audio.c, pm_au2.c, pm_log.c, pm_flash.c, pm_mem.c, pm_os.c, pm_gpu.c, pm_tex.c.

Tags: M = measured on the O3DS. A = measured in Azahar. R = Azahar value times the HW ratio of hw_rerank.md 1.3. E = estimate from code and counts. G = guess.
All ms are per drawn step. One step is 33.3 ms at 30 steps per second. "Play basis" = the HW test numbers minus 3.3 ms of test overhead (hw_rerank.md 1.1).

## 0. The vision and the short answer

Vision of the user: core 0 = game logic. Core 1 = audio logic. GPU = drawing logic. VRAM = texture cache. RAM = game assets + port + N64 RAM.

Short answer:
- Core 0 is not "game logic". Game logic and display-list build are 4.3 ms (8 percent) of 53.0 ms cpu per world step (M). The CPU side of the RSP and RDP emulation is 46.2 ms (87 percent) (M). This is the main gap.
- Core 1 is "audio" in the main. The audio job uses 55 to 64 percent of the app share (M, build 902). It has 8 to 14 ms per step free (E). The rest of the GBI path does not fit there.
- The GPU does the rasterization, the texture combine, the blend and the far, x and y clip. It does not do the vertex transform, the cull or the near clip. It idles 75 to 85 percent of a step (E from M).
- VRAM holds the render targets, the off-screen slots and the grab copy. It holds no texture. Peak use is 1774 KB of 6144 KB (M).
- RAM holds the resident ROM (27,106 KB), the image (19,592 KB) and the linear pool (14,208 KB) (M). Two large blocks are almost unused: the GPU command buffer (4096 KB, peak 3 percent) and the heap reserve (sbrk-left 4124 to 4440 KB) (M).

## 1. Actual topology today

### 1.1 Threads, cores and priorities (code)

Lower number = higher priority. The main thread priority is read at run time (default 0x30).

| Thread | Core | Priority | Created at | Work |
|---|---|---|---|---|
| main | 0 | 0x30 | libctru | Starts the game thread, then waits in threadJoin (pm_main.c:530 to 535). No CPU use |
| game | 0 | same as main | pm_main.c:532 | Game logic, display-list build, GBI interpreter (pm_gbi.c), combiner (pm_comb.c), citro3d state and draws (pm_gpu.c), texture decode and upload (pm_tex.c), ROM reads and Yay0 decode (pm_os.c, pm_map.c), audio kick and join, FS1 frame skip |
| audio control (au_thread, nuAuMgr) | 1 | same as main | pm_audio.c:160 (core 0 fallback 163) | Sequencers, voices, SFX queue, AU2 command emit. The game joins it at 4 sites |
| audio render (AU2) | 1 | main + 1 | pm_au2.c:152 | Mixer: ADPCM, resample, envelope, reverb, the C RSP audio microcode (pm_rsp_audio.c), PCM post to ndsp. The game never waits for it |
| log writer | 1 | 0x3F | pm_log.c:496 | Ring to log.txt, fsync once per second, socket accept every 250 ms and send (live log TCP 17492) |
| flash save | 1 | 0x3F | pm_flash.c:82 | One write of the dirty span of save.flash per save (HW2) |
| libctru service threads (GSP event, ndsp, APT) | E: process default core 0 | E: above the game | libctru | Event dispatch. Not in this tree, not checked. Cost not measured (G: under 0.5 ms per step) |
| System modules (GSP, DSP, FS, HID, NWM Wi-Fi) | 1 (O3DS system core) | system | firmware | GSPGPU_FlushDataCache IPC, DSP_FlushDataCache IPC, SD access, Wi-Fi |

Core 1 rule: the app gets core 1 only after APT_SetAppCpuTimeLimit. The game value is 80 (pm_log.c:547 to 560). HOME and sleep set 30. HW: `cpu limit: set 80 (start) result 00000000` (M, 902 and 926). Core 2 and 3 do not exist on the O3DS.

### 1.2 Core 0 per world step (HW 902, M)

| Part | ms | Share of cpu | Notes |
|---|---|---|---|
| Game logic (lo) | 2.55 | 4.8 % | step_game_loop |
| Display-list build (dl) | 1.72 | 3.2 % | gfx_draw_frame |
| GBI interpreter self (gb) | 36.03 | 68.0 % | 4026 ops, 8.9 us per op. Contains about 2.2 ms comb_audit and 0.9 ms tick reads of the test switches (E). Whole-cycle mean 31.59 ms (hw_seg.py) |
| citro3d state (gp) | 3.03 | 5.7 % | |
| citro3d draw (gd) | 3.19 | 6.0 % | 98 to 121 draws, 63 us per draw with gp |
| Cache flush (gcache) | 0.86 | 1.6 % | One GSPGPU_FlushDataCache IPC per frame |
| FrameEnd (fe) | 0.83 | 1.6 % | |
| Texture decode and upload (tx) | 2.23 | 4.2 % | Steady. Spikes 82.1 and 113.3 ms (steps 332, 337) |
| ROM read and Yay0 (rom, yay0) | 0.23 | 0.4 % | Spike 40.6 + 12.0 ms (step 331) |
| Audio join wait (au) | 1.00 | 1.9 % | Waits for core 1 |
| Other (ot) | 0.86 | 1.6 % | Counters, pc line |
| cpu total (wall - idle - fb) | 53.01 | 100 % | Play basis 49.7 ms |
| fbegin wait (fb) | 9.83 | | vblank alignment, not work |

Sum of the CPU side of the RSP and RDP emulation: gb + gp + gd + gcache + fe + tx = 46.2 ms (M). Game logic plus list build: 4.3 ms (M).
The log writer is not on core 0. The pc line format (0.1 to 0.3 ms, E) is on core 0 in test runs only.

Build 926 changes that touch core 0 (not measured on HW in world scenes): TX1 (tex 1.0 to 1.5 ms less steady, spikes to 15 to 35 ms, R), DR1 (gp + gd 2 to 4.5 ms less, E), KICK1 (0 ms cpu, the GPU starts earlier), AU2 (join wait 630 us to 14 us in Azahar, A; HW 1.00 ms to about 0.05 ms, E), FS1 (no ms change, the game skips a draw when late). Estimate for build 926: 42.7 to 45.7 ms cpu per world step, play basis (E).
HW 926 logos only (M): 29.9 steps per second, 0 late steps of 217, audio join avg 3.0 to 3.9 ms (p95 10.5 ms) in the logos. The logos have a very short cpu part (13.7 ms in 902), so the join comes early. This is the same effect as the 902 logos (8.29 ms). It is not a world number.

### 1.3 Core 1 per step (app share 80 percent = 26.7 ms)

| Part | ms per step | Share of 26.7 | Tag | Notes |
|---|---|---|---|---|
| Audio job (control + mixer), build 902 | 14.6 to 17.0 | 55 to 64 % | M | `audio cost` 7.3 to 8.5 ms per 60 Hz tick, 2 ticks per step |
| AU2 control part, build 926 | 0.5 to 1.8 | 2 to 7 % | R | Azahar 0.23 to 0.77 ms per step, ratio 2.2 (HW 7.3 / AZ 3.3 ms per tick) |
| AU2 render part, build 926 | 7.7 to 15 | 29 to 56 % | R, E | Azahar render busy 1.74 ms per job (max 4.76), 2 jobs per step, ratio 2.2. Upper end = the 902 job time. AU2 moves the mixer off the join path. It does not cut its cost |
| Log writer and socket | 0 (warn) to 2 (debug, phasecsv) | 0 to 7 % | G | 902 dropped 1207 lines: the writer starved at 0x3F under the audio job |
| Flash save | 0 in play, 1 write per save | 0 % | M (HW2 code) | Burst only |
| Free | 8 to 14 | 30 to 52 % | E | hw_rerank.md 2.4 gives 8 to 11 for build 902 |

System share: 20 percent of core 1 (6.7 ms per step) stays with the system modules. The audio probe (902) gave avg/min 1.03 to 1.12, so the audio thread got 90 to 97 percent of a core during the probe (M).

### 1.4 GPU (PICA200)

| Work | On the GPU today | Where |
|---|---|---|
| Vertex transform | No on the CPU path (identity game matrix, CPU xform_vtx). The raw path `gpuvtx` (default off) transforms on the PICA but keeps the CPU transform for the reject | pm_gbi.c G_VTX, pm.v.pica, pm3.v.pica |
| Trivial reject, cull | No. CPU reject (73 percent of triangles end there, M) and CPU cull, C3D_CullFace NONE | pm_gbi.c draw_tri |
| Clip | Far, x and y planes on the PICA clipper. Near plane on the CPU (clip_near) | pm_gbi.c:731 |
| Lighting, texgen, fog | No. CPU path. Use: 0.1 % lit, 0 % texgen, 0 fog (A, po3a) | |
| Rasterization, texture, combiner, blend, depth, stencil | Yes | pm_gpu.c, pm_comb.c |
| Texture rectangles and fills | Yes, as quads. DR1 merges equal-state quads | pm_gpu.c |
| Off-screen targets, grabs (B2) | Yes, render to texture and GX_TextureCopy | pm_gpu.c:222, 1110 |
| Previous-frame readback D03 (pause background, battle swirl) | No. CPU detile of 320 x 240 | pm_gpu.c, A20 of post_po3_path.md |
| Texture swizzle | No. CPU (TX1 one pass) | pm_tex.c |

Load: 98 to 121 draws, 360 drawn triangles, 1396 vertex slots of 40 B, 400 x 240 target per world step (M counters). PICA render plus transfer about 5 to 9 ms per frame (E from the 28.5 ms on-time knee, hw_rerank.md 2.1). Idle 75 to 85 percent of a 33.3 ms step, over 90 percent of a 55 to 65 ms world step (E).
Start point: citro3d holds the GX queue until C3D_FrameEnd (pm_gpu.c:1252). KICK1 (build 926) adds one C3D_FrameSplit when 55 percent of the draw count of the last frame is queued, if that count is 48 or more (pm_gpu.c:337 to 349, 1238). The HW gain of KICK1 is not measured (PT18).

### 1.5 Memory regions

O3DS commit limit: 65536 KB (M, 926 line #19 `mem commit limit 65536KB used-by-image 19592KB offered 45944KB`). Azahar reports 98304 KB (A, merge_au2.txt #18) and gives the port a 51200 KB heap against 31608 KB on the O3DS. So an Azahar run does not prove a memory fit on the O3DS.

| Region | Size | Use (peak) | Holds | Tag |
|---|---|---|---|---|
| Image (.text, .rodata, .data, .bss) | 19,592 KB | all | Code of 271 maps, engine, battle, linked map data, the decomp heaps and buffers in .bss (the game's "N64 RAM" is these statics, not one 4 MB image), log ring | M (926) |
| Socket buffer at 0x08000000 | 128 KB | all | socInit (live log) | M |
| Heap (newlib, in the linear block above the pool) | 31,608 KB | 27,411 to 27,556 KB used, free-in-arena 2 to 72 KB, sbrk-left 4,124 to 4,440 KB | Resident packed ROM 27,106 KB (read from SD once at boot, pm_os.c:232 to 250), malloc of game and port (about 300 to 450 KB). The 4 MB "leave" (pm_os.c:235) is room for malloc | M |
| Linear pool | 14,208 KB | 11,614 KB (902), free min 2,594 KB | See below | M |
| - GPU command buffer | 4,096 KB | 2 to 3 percent (about 123 KB) | C3D_Init(0x400000), pm_gpu.c:794 | M (`command buffer 3%`) |
| - CPU vertex pool | 1,280 KB | 3,600 to 5,196 of 32,768 slots (141 to 203 KB) | pm_gpu.c:846 | M |
| - Raw Vtx ring, index ring | 256 + 128 KB | gpuvtx only | pm_gpu.c:840 to 841 | M (code) |
| - Texture scratch | 512 KB | | pm_tex.c:384 | M (code) |
| - Screen frame buffers (gfxInitDefault) | about 1,012 KB | all | Top 400 x 240 x 3 x 2, bottom 320 x 240 x 3 x 2. libctru default places them in linear FCRAM | E |
| - Audio PCM slots, AU2 ring | about 64 + 40 KB | | pm_audio.c:58 | E |
| - Texture cache | budget 4,096 KB | peak 4,096 KB, live 1,045 KB at f9000 | One linearAlloc per slot (C3D_TexInit, pm_tex.c:692), 512 slots | M |
| - Margin | 2,584 KB | | Fragmentation reserve | M |
| VRAM | 6,144 KB | 1,774 KB peak (free min 4,370 KB) | Main target 400 x 240 RGBA8 + D24S8 (750 KB), off-screen slots 4 full + 4 of 64 x 64 + 2 8-bit, FX2a snapshot copies, grab texture 256 x 512 RGBA8 (512 KB) | M (902 `vram free min 4370KB`). Design worst case 3,310 to 3,822 KB plus snapshots (E, MEMORY_BUDGET.md) |
| ROM file on SD | 40 MB file | read once at boot | No run-time SD read, except the `romfull` fallback (stdio FILE*, its own buffer) and the PG2 bake loader (off) | M (code) |

Texture cache, HW 902 full cycle (M, sum of all texstat lines): hits 1,001,122, misses 7,397, hit rate 99.27 percent. 4,704 evictions of 37,343 KB. 1,228 of them (10,998 KB, 29 percent of the evicted bytes) were never hit. Live slots max 512: the slot limit binds, not only the byte budget. Uploaded up to 5,156 KB per 300 frames (f1500). Azahar (A, merge_au2.txt): 99.31 percent, 4,946 evictions. TC1 in Azahar: 44 percent of evicted bytes never hit, full in 4 scenes (f2700, f5100, f7200, f7500).

## 2. Gap analysis against the vision

### (a) Core 0 = game logic only

Gap: 46.2 ms of 53.0 ms is the CPU side of the RSP and RDP (M). Game logic is 4.3 ms. Where each part can go:

| Part (HW 902 ms) | To the GPU | To core 1 (8 to 14 ms free) | To a precomputed form | Verdict |
|---|---|---|---|---|
| G_VTX transform, reject, cull, triangle setup (TRI plus VTX share of gb, about 52 percent of 36.03 = 18.7, R) | Yes: GV1 (PICA transform with a matrix palette, box reject, C3D_CullFace, PICA clip). 8 to 14 ms saved (R, GV1.md) | No: tied to the per-op state | T2 (static geometry as PICA buffers) or DLC1 (cache of translated output) after GV1: 4 to 10 ms more (G) | GV1 first |
| Per-triangle state checks (section A, dual_bind, sync_render): 2.5 to 2.8 ms in Azahar, x 3.5 | No (state logic) | No | GV1b state epoch removes most of it | GV1b, O3r rest |
| Op decode and dispatch, TMEM, combiner decode | No | No (citro3d and the state must stay on one thread; O8 full interpreter is 46.2 ms, it does not fit) | DLC1 skips the decode of repeated static lists | DLC1 after GV1 |
| citro3d state and draws (gp + gd 6.2 ms, 63 us per draw) | Already the GPU's own work. CPU cost is citro3d command building | No (citro3d is not thread safe; O8b = 7.9 ms takes all of core 1) | Command-list reuse (part of T2 or DLC1) | DR1 done, GV1c palette keeps the draw count |
| gcache 0.86 ms | n/a | n/a | n/a | GC1: measure per-call cost |
| Texture decode 2.23 ms steady, spikes 82 to 113 ms | Swizzle only (GX_DisplayTransfer), format conversion stays CPU. Hold | Yes for a prefetch at map load or a decode of known next textures (O9). Not for a miss that the same frame needs | Bake (T1, PG2): closed, SD read cost | TX1 done, O9 for the spikes |
| ROM read and Yay0 0.23 ms steady, 52.6 ms in step 331 | No | Yes: Yay0 decode of the next map sprites in the fade-out (HW6, O9). The ROM read is a memcpy of the resident ROM, not worth a move | n/a | HW6 then O9 |
| Audio join 1.00 ms | n/a | Done (AU2) | n/a | Done |

Result: core 0 can come close to "logic plus list walk" only with GV1 plus DLC1 or T2. GV1 alone leaves about 32 to 38 ms of cpu (E), of which the logic is 4.3.

### (b) Core 1 = audio logic

Today core 1 holds the audio control, the AU2 mixer, the log writer, the socket server and the flash save. In a play build (loglevel warn, no live log) the writer and the socket are near 0 and the flash thread runs only at a save. So core 1 is in fact the audio core in play (E).
- Is the share enough: yes for the audio. 12 to 17 ms of 26.7 ms (E). Underruns: Azahar AU2 98 of 16797 posts (A). The O3DS underrun count with AU2 is not measured. PT14 (crackle near 30 FPS) needs the `audio2` and `audiohash` lines of a hands-off HW run.
- What to move off core 1: nothing in play. In test runs the log writer competes with the mixer at 0x3F. It lost 1207 lines in 902. The 1024-slot queue of HWM1 fixed the loss in Azahar (0 dropped). Keep `noaudit` and `phasecsv=<from>:<to>` in HW runs.
- What to move onto core 1 (8 to 14 ms free): texture prefetch decode (O9), Yay0 decode of the next map (HW6), never the GBI path or citro3d.
- Risk: a core 1 job that takes the render thread time causes underruns. Rule: any new core 1 job runs at 0x3F (below the render thread) and stops at 6 ms per step (E budget: the 8 ms lower end of the free share minus a 2 ms margin).

### (c) GPU = drawing logic

Gap: geometry (transform, reject, cull, near clip) and the D03 readback stay on the CPU. The PICA has room (75 to 85 percent idle, E).
- Geometry: GV1a to GV1c move the transform, the cull and the far/x/y clip of plain batches (85.8 percent of the triangles, A) to the PICA. Near-plane loads, lit loads (0.1 %), texgen (0 %), screen vertices (0), wide UI and debug switches stay on the CPU path (GV1.md 1.5).
- Fog and lighting: no gain (0 and 0.1 percent use). GV1e only if a counter shows fog over 2 percent.
- Framebuffer effects: grabs (B2: water, crystal ball, flame blur) and off-screen slots already run on the GPU. The D03 readback (pause background, battle swirl and blur of PT12) is a CPU detile of 320 x 240 per capture (A20). O10 moves it to a GPU render and GX_DisplayTransfer. Gain: capture frames only. The HW capture time is not measured: read the b7 `swirl capture` line first (row B7).
- Texture swizzle on the GPU (GX_DisplayTransfer): holds. TX1 made the CPU swizzle cheap, and a GX job inside a held frame needs a split.

### (d) VRAM = texture cache

Gap: 0 textures in VRAM. VRAM peak use 1774 KB (M), so 4370 KB are free at the minimum on HW. Design worst case 3822 KB plus FX2a snapshots leaves 2.2 to 2.8 MB (E).
What a VRAM tier (O13) would hold: the textures hit in the last N frames. HW f9000: 107 slots used in the last 300 frames, live 175 slots of 1045 KB (M). So a 1.5 MB VRAM tier holds the hot set of a typical scene.
Cost of the copy: one GX_TextureCopy per promoted texture. The copy is a GX command in the same queue as the draws. citro3d holds the queue in a frame, so the copy runs in queue order before the first draw that uses it (E). The CPU still decodes into linear FCRAM and flushes it (C3D_TexFlush, pm_tex.c:766). Per step about 14 KB of new texture data (M texKB), spikes 552 KB (step 337). The DMA time is small against the step (G: under 1 ms for 552 KB).
PICA fetch from VRAM against FCRAM: VRAM is the GPU's own memory. An FCRAM fetch shares the bus with both ARM11 cores, the DSP and the GX DMA (E). The PICA has a small texture cache. The texel bytes per frame are not counted. Gain: GPU time 0.5 to 1.5 ms (G, hw_rerank.md rank 11) and less bus contention while KICK1 overlaps the GPU with core 0 (G, not measurable without a GPU busy counter).
Effect on the 4096 KB FCRAM cache: a tier that copies keeps every texture in FCRAM too, so it frees nothing. A tier that moves (VRAM owns the hot set, the FCRAM copy is freed after the copy) frees up to 1.5 MB of FCRAM. But CPU-written textures (palette flash, imgfx, frame buffer and z-buffer images) must stay in FCRAM, and an eviction from VRAM needs a new decode.
The binding limits on HW are elsewhere: the 512 slot limit (live 512, M) and the one-shot textures (29 percent of evicted bytes never hit, M). A cache policy fix (TC2, new) is cheaper and acts on the misses, which cost CPU time (tex spikes, PT16). The VRAM tier acts only on GPU time, which is not the limit.
Verdict: O13 holds until a GPU busy number (pcx `gpu` field) shows the GPU tail or the bus as the limit after GV1. TC2 goes first.

### (e) RAM = assets + port + N64 RAM

O3DS app region: 65536 KB commit (M). Split (M, build 926 boot): image 19,592 KB, socket 128 KB, linear pool 14,208 KB, heap 31,608 KB. The 98304 KB figure is Azahar's (A), not the O3DS.
- Game assets: the resident packed ROM, 27,106 KB in the heap. 2,790 KB of it were never read in the M02 trace (D7, E). No run-time SD reads.
- Port: linear pool 14,208 KB and the port part of the image.
- N64 RAM: the decomp heaps and buffers are .bss in the image (MEMORY_BUDGET.md: N64 base RAM 4 MB inside .bss 6,943 KB). There is no separate N64 RAM block. The 4 MB "leave" of pm_os.c:235 is free malloc room.
What can be released (M sizes):
- GPU command buffer: 4,096 KB allocated, peak 3 percent (about 123 KB). GV1 adds about 62 KB per frame (GV1.md 2). A 512 KB buffer keeps a 4 times margin. Release: 3,584 KB of linear (row MEM1, new).
- CPU vertex pool: peak 5,196 of 32,768 slots. GV1f shrinks it to 8,192 slots: 960 KB.
- Heap reserve: sbrk-left 4,124 to 4,440 KB never used in a full cycle (M). Keep it: all 421 maps (S30b, +1,988 KB E) and malloc peaks of maps that the cycle does not visit need it.
- Never-read ROM ranges (D7): up to 2,130 KB, needs a romtrace of all maps, battle and pause. Low priority.
What to do with the released linear memory: raise the texture cache budget (TC2) or keep it as margin. Streaming the ROM from SD is not needed and costs SD time (PG2 lesson).

## 3. Target topology

### 3.1 Target per resource (world step, play basis)

| Resource | Today (902) | After 926 rows | After GV1a to GV1c | After GV1d to GV1f, O3r, TC2, GC1 | After DLC1 or T2 |
|---|---|---|---|---|---|
| Core 0 cpu ms | 49.7 (M) | 42.7 to 45.7 (E) | 31 to 40 (E) | 28 to 37 (E) | 18 to 33 (G) |
| - of which game logic + list build | 4.3 (M) | 4.3 | 4.3 | 4.3 | 4.3 |
| Core 1 ms of 26.7 | 14.6 to 17.0 audio (M) | 12 to 17 (E) | same | same, plus O9 prefetch up to 6 in load steps (E) | same |
| GPU ms of 33.3 | 5 to 9 (E) | 5 to 9, starts at 55 % of the draws (E) | 6 to 11 (G: the transform of about 1681 vertices per frame moves to the PICA) | 6 to 11 (G) | 6 to 11 (G) |
| VRAM KB of 6144 | 1,774 peak (M) | 1,774 | 1,774 | 1,774 (O13 held) | 1,774 |
| Linear KB of 14,208 | 11,614 peak (M) | same | same | 7,070 + texture budget (MEM1 -3,584, GV1f -960) (E) | + 500 to 2,000 for T2 buffers (G) |
| Heap KB of 31,608 | 27,556 peak (M) | same | same | same | same |
| Drawn steps per second, world | 15.7 (M) | 20 to 23 (E) | 25 to 30 in light and average scenes (E) | same, heavy scenes 20 to 25 (E) | 30 mean in most scenes (G) |

Honest estimate: the vision "core 0 = logic" is not reachable on the O3DS. GV1 makes core 0 about 60 to 70 percent emulation work instead of 87 percent. Only DLC1 or T2 brings it near 40 to 50 percent (G). 30 steps per second in every scene is not reachable with any listed row (hw_rerank.md 3). FS1 keeps the game speed near 30 steps per second at a lower drawn rate.

### 3.2 Rows in order of gain per effort

| # | Row | New or existing | Gain | Size | Done check |
|---|---|---|---|---|---|
| 1 | P3R: repeat Push 3 of build 926 hands-off (abtog=base,gpuvtx,nodr1,nokick,notx1,phasegbi:30) | existing (PT18), needs the user | Data: HW numbers of TX1, DR1, KICK1, AU2, the GV1 inputs, the GPU busy field (pcx), audio2 underruns | 0 lines | phs lines for every scene and state, 0 lines dropped, `pcx` gpu field present, `audio2` underrun count logged |
| 2 | MEM1: GPU command buffer 4096 KB to 512 KB (C3D_Init, pm_gpu.c:794), log the peak in the mem line | new | +3,584 KB linear (M basis) | 3 to 5 | Azahar full cycle: `command buffer` max under 50 percent, 0 `GPU LIMIT NEAR` lines, golden title yard battle pass, mem line linear free min higher by 3,584 KB |
| 3 | GV1a, GV1b, GV1c, then HW push 4 | existing (docs/plans/GV1.md) | 6 to 11.5 ms core 0 (R) | 370 to 510 | GV1.md slice gates; HW: gb lower by 6 ms or more per world step |
| 4 | TC2: texture cache policy. SLOTS 512 to 1024, admission on the second use for textures over 16 KB (segmented LRU), budget raised by the MEM1 bytes | new | fewer misses in the 4 full scenes and the 512-slot scenes. Spike ms lower (G: 5 to 30 ms in load steps) | 40 to 70 | Azahar full cycle: evictions lower by 50 percent or more in f2700, f5100, f7200, f7500, texmiss lower, never-hit evicted KB lower, golden toad yard battle 0 differences, linear free min 1 MB or more |
| 5 | GC1: measure the per-call cost of GSPGPU_FlushDataCache. It runs once per frame, once per KICK1 split and once per texture upload (C3D_TexFlush, pm_tex.c:766: 74 calls in step 337). If the cost is per call, flush the texture range once per frame | existing (hw_rerank.md rank 7), scope widened | 0 to 0.7 ms steady, spike part not known (G) | 10 to 30 | HW pcx or phs: gcache per call reported, tex ms of step 337 lower |
| 6 | GV1d, GV1f, O3r rest | existing | 0.3 to 1 + 1 to 2.5 ms (R), -960 KB linear | 140 to 250 | GV1.md and hw_rerank.md gates |
| 7 | HW6 then O9: Yay0 and NPC sprite load spread or on core 1, texture prefetch decode on core 1 at 0x3F, at most 6 ms per step | existing | spike steps 331 to 337: 20 to 60 ms less (E) | 80 to 150 | step 331 wall under 60 ms (Azahar), statehash equal, audio2 underrun count not higher |
| 8 | O10: D03 readback on the GPU, after the b7 swirl capture time is known (row B7) | existing | capture frames only (PT12 battle entry) | 60 | golden pause1 to pause4, b7 capture time lower |
| 9 | DLC1 or T2 | existing | 4 to 10 ms (G) | 1500 or more | plan after push 4 |
| 10 | O13: VRAM tier | existing, hold | 0 cpu, GPU 0.5 to 1.5 ms (G) | 60 to 100 | only if the pcx gpu field shows GPU time over 15 ms per frame after GV1 |
| 11 | FB1: screen frame buffers in VRAM (gfxInit with VRAM buffers), about 1,012 KB from linear to VRAM | new, low | -1,012 KB linear, LCD scanout reads leave FCRAM (G: bus only) | 5 to 10 | golden 3 scenes, D03 capture still correct (it reads the PICA buffer, not the screen buffer), VRAM free min above 2 MB |

Top 3 by gain per effort: P3R (data, 0 code), MEM1 (3.5 MB for 5 lines), GV1a to GV1c (the only large core 0 cut).

## 4. Resource hazards

1. One memory bus. FCRAM serves core 0, core 1, the PICA (vertex fetch, texture fetch, command list), the DSP (PCM slots) and GX DMA. Before KICK1 the PICA ran only while core 0 waited. With KICK1 the PICA reads the vertex pool and the textures while core 0 runs the GBI path and core 1 runs the mixer. The contention cost is not counted (Azahar has no bus model). Signs on HW: gb per op higher with KICK1 than with `nokick` in the P3R run. Writes per world step: about 43 KB of vertices (E), 14 KB of texture data (M). Fixed readers: LCD scanout of the linear frame buffers (E: about 30 MB per second for both screens), display transfer from the VRAM target to the linear frame buffer each frame.
2. Cache flushes. The ARM11 data cache is 16 KB per core, no L2 on the O3DS. Every GPU-read range must be written back: vertex pool, gring, gidx (pm_gpu.c:190 to 199) at each split and at FrameEnd, and each texture at upload (C3D_TexFlush). Each GSPGPU_FlushDataCache is an IPC to the GSP module on core 1, so it also takes a little of the core 1 system share. gcache is 0.86 ms per step for one call (M). A missed flush is a HW-only fault (GV0a, GV0b were two). A new GPU-read buffer in any row needs a flush before the next split.
3. DSP, mixer and core 1. The DSP itself is a separate processor. The mixer runs in C on core 1 (render thread). The DSP service, DSP_FlushDataCache (pm_audio.c:112) and the ndsp interrupt path also use core 1 system time (E). A long system call on core 1 (SD write of the log or the save, Wi-Fi) delays the mixer and can underrun. AU2 has 8 jobs of ring, at most 4 steps of buffer.
4. Thread priorities. Core 1: control (main prio) above render (main + 1) above log writer and flash (0x3F). The writer starves under audio load (902: 1207 lines lost). A save waits behind the mixer (the HOME and sleep join covers safety, not latency). Core 0: the libctru service threads (GSP event, ndsp, APT) run above the game thread (E, not checked). Any new thread must use libctru natives and a priority below the render thread on core 1.
5. APT limit. 80 percent is the maximum the API allows. HOME and sleep set 30, so a suspended app gets 8.9 ms per step on core 1. The kernel enforces the limit in time slices, so a core 1 job can stop for a slice inside a step (probe avg/min 1.03 to 1.12, M). Core 1 work must not be on the critical path of a step: AU2 removed it from the join.
6. System services on core 1. HOME menu, Wi-Fi (NWM), FS and the GSP run in the 20 percent system share. The live log adds Wi-Fi traffic. Two old crash dumps are in nwm (Wi-Fi) and six in rehid (hw_findings.md B). HW performance runs should use the pulled log, not the live log.
7. Azahar as a memory proxy. Azahar gives a 51,200 KB heap and a 98,304 KB commit against 31,608 KB and 65,536 KB on the O3DS. A row that adds heap or linear use must check the HW boot `mem` lines or the margin by arithmetic.
8. Fragmentation of the linear pool. Each texture slot is its own linearAlloc and C3D_TexDelete at eviction. 4,704 evictions per cycle (M) churn the pool. 0 texture failures so far (M), margin 2,584 KB. TC2 with more slots raises the churn. MEM1 gives the margin back.

## 5. Not measured

- HW world-scene numbers of build 926 (TX1, DR1, KICK1, AU2, FS1). P3R gives them.
- GPU busy time per frame on HW (pcx `gpu` field exists from HWM1, no hands-off HW log yet).
- Core 1 CPU time of the AU2 render thread on HW, and the HW underrun count.
- Cost per call of GSPGPU_FlushDataCache.
- Bus contention between the PICA and the cores.
- libctru service thread cores and priorities (library source not in this tree).
- The texel bytes the PICA fetches per frame.
