# BENCH notes: micro-benchmarks (S0, arch_review.md part 3)

Code: 3ds/src/pm_bench.c, 3ds/include/pm_bench.h. Hook: 2 lines in 3ds/src/pm_main.c (`#include "pm_bench.h"` and `if (pm_bench_run()) return;` as the first statement of `pm_frame_loop`). CMake: the file is in PORT_SRC and not in the -fno-short-enums list (it includes citro3d). No graphics, audio or options file is changed.

## Switches
- `bench` runs draw, drawpm, flush, tex, replay and fbwait.
- `bench=<list>` runs the named benches. Names: draw (B1 d0 to d5, r0, r1), drawpm (B1 d6), r2 (B1 r2, only by name), flush (B2), svcflush (B2 SVC, only by name, runs flush too), tex (B3 and the write speeds), replay (B4), fbwait (B7).
- `benchexit` leaves the frame loop after the benches. The game does not start. Without it the game starts after the benches.
- Every result line starts with `bench:` and has `min`, `avg`, `max` over `runs` runs, and the unit in the name of the field. `bench: begin` and `bench: end total_ms` bracket the run. Azahar run (host numbers): 27 s total.
- Numbers from Azahar are host numbers. Only the console numbers count.

## Safety
The benches use their own 64 x 64 render target (no screen output), own program instances of pm.v.pica and pm3.v.pica, own buffers and textures. They free all of it at the end. `drawpm` calls pm_gpu_begin, pm_gpu_vert_clip, pm_gpu_flush and pm_gpu_end: it resets the per-frame state of pm_gpu at its own begin, and the game resets it again at its first frame. The loop calls aptMainLoop between items: HOME works between items. A long item takes under 5 s.

## B1 draw ladder: `bench: draw <item> n <N> k <K>`
Field `cpu_us_per_draw`: CPU time of the N draws divided by N. Field `gpu_us_per_frame`: citro3d GPU time of the frame (about 0, scissor 8 x 8). N is 1, 10, 100, 500. K is 1 or 8 triangles. N = 1 holds the first-call cost (cache misses): use N = 500 for the rate.
- d0 C3D_DrawArrays only. d1 plus one texenv stage. d2 plus six stages. d3 plus C3D_TexBind of two textures in turn. d4 plus four C3D_FVUnifSet and one C3D_FVUnifMtx4x4. d5 plus C3D_BindProgram, C3D_SetAttrInfo and buffer info of two programs in turn.
- d6 (drawpm) the pm_gpu path: set_combine, three pm_gpu_vert_clip, pm_gpu_flush per draw. K is 1.
- r0 GPUCMD_AddRawCommands of the draw packet of citro3d (captured from one real draw, `words draw` 22 in Azahar). r1 the state block of six stages (`state` words) plus the draw packet. r2 a jump to a sub command buffer (GPUREG_CMDBUF_SIZE0, ADDR0 once, JUMP0 per draw). r2 is only by name, because a wrong sub buffer can hang the GPU. r2 checks only that the frame ends. It does not check a pixel.
- Decisions (arch_review.md 3.3): d6 over 40 us and r0 + r1 under 10 us: RAW1. d0 under 15 us and d6 over 40 us: fix flush_draw in place. d5 - d0 over 20 us: one DVLB. d5 - d0 under 5 us: no change.

## B2 flush cost: `bench: flush`
- `gsp`: GSPGPU_FlushDataCache, sizes 64 B to 1 MB, 50 calls (10 for 256 KB and 1 MB), `dirty 0` clean buffer, `dirty 1` the buffer is written before each call (not timed). `us_per_call`.
- `audio_proxy 1`: a busy thread on core 1 (ALU and memory loop, 64 KB) runs during the calls. It is a proxy of the mixer. It does not touch the real audio state, so the audio threads of the game are not measured. For the real audio effect read `gcache` and `tx` of the normal run.
- `fit gsp`: two-point fit. `fixed_us` is the call cost at size 0, `per_kb_us` the slope between 64 B and 1 MB. Decision: 64 B call over 0.4 ms: FL1. 64 B under 0.1 ms and 1 MB over 0.5 ms: the cost is per byte, flush only written bytes.
- `gx_enqueue`: GX_FlushCacheRegions, CPU time of the enqueue only (3 ms pause between calls).
- `svc`: svcFlushProcessDataCache (bench=svcflush only). `bench: svcflush start` is the last line if the process ends.

## B3 texture stages and write speed
- `bench: tex <w>x<w>`: per texture, batches of 50, 4 batches: `init` C3D_TexInit, `decode`, `flush` C3D_TexFlush, `free` C3D_TexDelete (us). The decode is a proxy (tx1_build is static in pm_tex.c): RGBA5551 copy or CI8 with a 256 entry TLUT, written in 8 x 8 tile order. Real decode time: read `tx1 f` in the normal run. Decision: flush over 0.6 ms of the 1.21 ms per miss: FL1. Init or free over 0.6 ms: slot pool first.
- `bench: wr <target> <kind> flush <0|1>`: bytes per us writing 64 KB. Targets: `linear_cached` (linearAlloc), `vram_uncached` (vramAlloc, written by the CPU, no flush needed). Kinds: word (volatile u32), byte (volatile u8), memcpy. `flush 1` adds one GSPGPU_FlushDataCache of the 64 KB in the time.

## B4 replay: `bench: replay <linear|vram> draws 100 tris_per_draw <T>`
One mesh of T triangles (16 byte Vtx, index buffer, one memory block), 100 replays per frame with pm3.v.pica and a new matrix uniform per draw, 5 frames. `cpu_us_per_draw`: CPU per replay. `gpu_ms`: citro3d GPU time of the frame (tris_per_frame = 100 x T). `frame_wall_ms`: wall time of one frame including the GPU wait. VRAM buffers are written by the CPU. Decision: cpu per draw under 15 us and gpu under 10 ms for 100,000 triangles (T = 1000): DLC-G. cpu over 30 us: RAW1 first.

## B5 audio (not in this row)
`auprof` is in pm_rsp_audio.c (audio files, AUD-DSP agent). The ndsp cost bench is the `dsptest` of AUD-DSP (A2). BENCH adds no code for B5.

## B6 dlsrc counter (for the graphics agent: add it to pm_gbi.c, it is not a bench)
Counter in the display-list interpreter, active with switch `dlsrc` (zero cost without it). One line per 300 drawn steps and per scene: `dlsrc <mode> <map> ops m <n> s <n> o <n> tri m <n> s <n> o <n> vtx m <n> s <n> o <n> dl <n> stable <n>`.
- Region of a command, by the address of the display-list word (pc): m = main list (gDisplayContext buffers), s = static data (map shape gMapShapeData, .data, .rodata, entity and animator data), o = other (heap, .bss). The region of a vertex load is the region of the Vtx pointer. Classify with address range tests (3 compares), not with a search.
- `ops` counts every F3DEX command executed. `tri` counts triangle commands (G_TRI1, G_TRI2, G_QUAD). `vtx` counts vertices loaded by G_VTX.
- `dl` is the number of distinct G_DL targets per frame (a small open-address set, reset per frame).
- `stable`: for every static-region G_DL target, keep a 32-bit hash (for example FNV-1a over the list bytes up to G_ENDDL) in a table keyed by the target address. A target is stable when its hash equals the hash taken 30 drawn frames before. Print the percent of static targets that are stable in the window.
- Decision (arch_review.md 3.3): static share of triangle commands at 70 percent or more in world scenes and stable share at 95 percent or more: DLC-G. Static share of state ops at 40 percent or more: DLC-S. Static triangles under 50 percent: no DLC.

## B7 frame wait A/B: `bench: fbwait <syncdraw|nosync_flags0> cpu_work_ms <W>`
Synthetic loop of 1 s per run, 3 runs, with one tiny draw and W ms of CPU work after C3D_FrameBegin (the game step). `syncdraw` is C3D_FrameBegin(C3D_FRAME_SYNCDRAW) (the game now). `nosync_flags0` is C3D_FrameBegin(0): it waits only for the GPU of the last frame. Read `frames_per_s` per W. The loop has no screen transfer and no swap, so `nosync_flags0` is an upper bound (at W = 0 it runs free). Compare the two at W of 12, 20 and 30 ms: the gain of nosync is the vblank alignment part of `fb`. Decision (3.3): drawn per second +2 or more in world scenes: FB1.
For the real A/B in the game (statehash check, `pace:` lines) the graphics agent adds a switch `fbwait` at the `C3D_FrameBegin(C3D_FRAME_SYNCDRAW)` line in pm_gpu_begin: with the switch use `C3D_FrameBegin(0)`, and keep the explicit wait for the GPU of the last frame only. BENCH does not edit pm_gpu.c.

## Lane C
Lane C is a git worktree on branch lane-c (PM_LANE=laneC). 3ds/build.sh now adds the suffix `-<PM_LANE>` to the build number (before: only `-laneB`). run_azahar.sh and its lock are the same for all lanes. Build: `PM_LANE=laneC sh 3ds/build.sh`. Output: build3ds/lane-laneC/pm_3ds.3dsx.
