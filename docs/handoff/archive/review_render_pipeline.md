# Review: does the render pipeline fit the 3DS?

Branch `dlc-compiler-c1c2`. Read-only review. Tags: [M] measured (log or table named), [C] read from code (file:line), [H] public 3DS hardware fact (not measured here), [I] inference.

## 1. Short answer
- The pipeline is a per-frame HLE interpreter: every frame the ARM11 walks every N64 display-list op, keeps the RSP and RDP state, and emits citro3d calls. The GPU then idles. The cost is on the CPU walk, not on the PICA. [M][C]
- (a) Task split and hops: NOT supported. 2 tasks per frame (bg + main), hops/ops 0.12 in WORLD, under the 0.15 threshold of the plan. [M]
- (b) RDP state model against the CPU/GPU split: PARTLY supported. The RDP model makes many small CPU steps and closes batches at N64 tile granularity. But the GPU is not the limit. [M][I]
- (c) Both from display-list work: SUPPORTED. The shared root is the walk of the full list on one core each frame. The display-list cache that would remove it does not engage in the world (`pdl 0 pdlm 0`). [M]

## 2. Measured numbers used
| Source | Value | Tag |
|---|---|---|
| HANDOFF.md, O3DS yard build 1141 | wall 53 ms, walk 27 ms, about 19 fps, `tile` ~65, `comb` ~60 | [M] console |
| `build3ds/hw_live_log_170.txt` | 0 lines. No console split exists for the per-class fields | [M] |
| console_ready.txt (source is the Azahar attract_s6 run, not the console) | WORLD: W 9670 us, C 2003, X 857, G 325, ops 3412, closes 268, tasks 2.0, hops 401.7 | [M] Azahar |
| attract_s6_run.txt f4500/f4501 (WORLD) | `taskpl` bg 2390 us + main 14603 us, ops 5772, push 358 ret 358, spmax 5, pdl 0, pdlm 0. `rsplit` dlc 15 | [M] Azahar |
| same, `flsplit f4500` | tex 305 closes, 228 empty, 2303 us. cc 16, rect 34, tri 25, oth 33 | [M] Azahar |
| same, `thsplit f4500` | hit 91 / 365 us, miss 1 / 335 us, dcache flush 1 / 62 us | [M] Azahar |
| same, `gbisplit f4500` | tex-op streak 6129 us, tri streak 1172 us, sync 799 us | [M] Azahar |
| same, `gpusplit f4500` | gproc 19964 us, gdraw 332 us, draws 110, kick 1 | [M] Azahar |
- Azahar GPU time (gdraw is 332 us in almost every frame) does not show real PICA time. It shows direction only. [M][I]
- The streak timer reads `pm_ticks` (about 1.3 us in Azahar) at each streak edge. The 6.1 ms tex streak contains timer cost. [C pm_gbi.c:42-60][I]

## 3. PICA200 and the 3DS against what the port does
| Hardware property [H] | What the port does per N64 command | Fit |
|---|---|---|
| CPU builds a GPU command list in linear FCRAM. GSP submits it. Each submit and each `GSPGPU_FlushDataCache` is an IPC call | One command list per frame plus one `C3D_FrameSplit` at about 55 % of last frame's draws (KICK1). Vertex ring and index ring flushed in one span before each split [C pm_gpu.c:255-275, 435-456] | Fits |
| The GPU can draw while the CPU builds, only after a submit | One split per frame (`kick 1/3`) [M]. The GPU gets the first part late and the rest at FrameEnd | Partial. More splits help only when the GPU is busy [I] |
| 6 TEV stages, fixed function, one combiner buffer | N64 combiner decoded to a plan, mapped to up to 6 stages, memo of plans (`decide`, `scm_*`) [C pm_gbi.c:956-1090, pm_gpu.c:470-620]. Combiner rebuild 0.43 ms Azahar [M HANDOFF] | Fits |
| Uniforms: 96 vec4 float registers. A matrix palette in the vertex shader holds about 21 matrices | GV path: raw N64 Vtx into a GPU ring, matrices as a palette, transform on the GPU [C pm_gpu.c:1789-1833]. CPU path stays for decals, z-mark, wide UI [C pm_gpu.c:1793-1799] | Fits. It moves transform off the ARM11 |
| Uniform state is per draw. Any change ends the draw | `uvso` (texture origin/scale) is a per-batch uniform. A different origin closes the batch [C pm_gpu.c:747, 1803]. File-select flip: 231 draws, 158 closes from SETTILESIZE [M SLICES PT86] | Does not fit. Per-texture data in uniforms forces small batches |
| Textures: power of two 8 to 1024, 8x8 Morton tiles, no palette formats, L8/L4/LA4/RGBA5551/RGBA8 | CI expands to RGBA5551 keyed by palette hash. I/IA go native L8/L4/LA4. Morton write on a miss. Pre-baked converted textures (`tb_mode`) [C pm_tex.c:289-297, 897-1013] | Fits |
| VRAM 6 MB (fast GPU reads). FCRAM linear is slower for the GPU. Texture to VRAM goes by DMA | 1536 KB texture slab in VRAM, staged upload with `C3D_TexLoadImage`, merged flush spans [C pm_tex.c:413-514]. Miss cost 335 us per frame, hits 365 us [M] | Fits. Not the bottleneck |
| Batches want large runs of one state. Draw count of a few hundred per frame is normal for PICA | 110 draws per frame, 305 TEX-class closes of which 228 empty [M]. A close happens on every tile-0 SETTILE that differs, every palette change of a bound tile, every scissor change [C pm_gbi.c:3064, 3114; pm_gpu.c:2096] | Partial. The draw count is fine. The CPU cost per close (about 30 us Azahar per non-empty close [M/I]) is not |
| Screens 400x240 top, 320x240 bottom, stored rotated (240x400, 240x320) | N64 320x240 drawn to the top target, `pm_wide` stretches 3D by 1.25 and keeps UI 1:1 [C pm_gpu.c:137-139, 1796] | Fits |
| O3DS: ARM11 268 MHz, 16 KB L1, no L2. App core 0 full, core 1 (syscore) under the APT time limit. N3DS: 804 MHz, L2, a free core 2 | Logic, DL walk, command build: all on core 0 in series. Core 1 runs audio, log writer, flash [C pm_audio.c:103, pm_log.c:487, pm_flash.c:86] | Does not fit on N3DS (core 2 unused). Little room on O3DS [I] |

## 4. Where the critical CPU path goes
- Per frame on core 0, in series: game logic, then `nuGfxTaskStart` walks bg and main task, then FrameEnd [C pm_nusys.c:85-102]. No overlap of logic and walk. [C]
- Per op in `pm_gbi_run`: range check, two streak calls, epoch bump on every non-geometry op, switch [C pm_gbi.c:2477-2515]. At every G_DL: `dlc_try`, `pdl_try`, `pdl_try_m` probes before the push [C pm_gbi.c:2541-2557]. 358 probes x3 per frame with 0 pdl hits [M]. Cost per probe not measured [I].
- Texture ops emulate TMEM: `ldrec`, `blk` scans, tile-1 window math, then the slot lookup at first draw [C pm_gbi.c:3071-3229, pm_tex.c:920-962]. The tex-op streak is the largest timed class (6.1 ms of about 17 ms walk in WORLD, Azahar) [M]. Of it, closes are 2.3 ms and lookups 0.76 ms. About 3 ms is op handling and timer cost [M][I].
- us per op: Azahar WORLD about 2.9 us (17 ms / 5772). Console yard 27 ms walk. With 3.4k to 5.7k ops that is 4.7 to 7.9 us, about 1300 to 2100 ARM11 cycles per op [I]. A table interpreter with a large state set and no L2 on O3DS is the likely cause [I].

## 5. What can move or be cached
| Candidate | State now | Fit |
|---|---|---|
| DL cache keyed by DL address + heap generation + input-state hash | Exists (`pdl_try`, C0-C7). Refuses unless the list is class 2/3 and `state_in` matches, else recompiles [C pm_dlc.inc.c:898-914]. WORLD: pdl 0 [M] | Best fit. Not working in the world |
| Map shape lists (DLC1) | `dlc 15` hits per frame [M] | Fits, small share |
| Pre-baked GPU packets per pipeline state | C6 PSO packets exist [C pm_gpu.c:627-700] | Fits |
| Texture pre-swizzle at load | `tb_mode` baked dump read from file [C pm_tex.c:994-1013] | Done |
| Combiner plan cache | `decide` table + `scm` memo [C pm_gbi.c:956-1045] | Done. 0.43 ms [M] |
| DL walk on a second thread | Not present. Needs one frame of latency (user decision open, agent_gpu.md) | N3DS core 2: good [I]. O3DS core 1: limited and shared [I] |

## 6. Ranked recommendations
| # | Change | Size | Estimated effect | Tag |
|---|---|---|---|---|
| 0 | One console live log at the yard with `gpusplit flsplit thsplit taskpl`. The log was empty after build 1141 | push only, 0 lines | Gives the console split. All ranks below use Azahar shares | [M] gap |
| 1 | Diagnose why `pdl_try` never hits in WORLD: count refuse, `state_in` mismatch, heap-gen mismatch, class reject per frame in `taskpl` | row, about 30 lines | None alone. Names the blocker of #2 | [I] |
| 2 | Make the DL cache hit for static model and map lists: key on the state the list reads only, replay pre-built draws and PSO packets | epic (C-stage work, more than 100 lines) | If 40 to 60 % of ops are inside static lists, walk drops by about that share: 27 ms to 12 to 17 ms on O3DS | [I] |
| 3 | Close on resolved state, not on N64 ops: mark dirty at SETTILE/SETTILESIZE/LOADTLUT, close only at the next draw if the resolved slot, wrap or palette differs | row, about 80 lines in pm_gbi.c | Removes the 228 empty TEX closes and the closes where the slot stays the same. Up to the 2.3 ms close time in WORLD Azahar, likely 1 to 3 ms console | [I] |
| 4 | Move `uvso` from a batch uniform to a vertex attribute (or add the offset to T at load, PT86 idea) | row, about 100 lines, both vertex paths | File-select flip 231 draws toward 53. Text strips and scrolled textures share a batch | [I] |
| 5 | Remove the 3 G_DL probes when the cache is off for the class: one class test first, then one probe | row, about 20 lines (composer row) | Small, about 358 probes per frame | [I] |
| 6 | Pipeline split: logic of step N+1 on core 0, walk of step N on N3DS core 2 with the double-buffered gfx pool | epic | N3DS wall goes from logic + walk to max(logic, walk). O3DS little gain. One frame of input latency | [I] |
| 7 | Small-texture atlas for clamp-only CI textures, origin in the vertex data | epic | Fewer real texture closes (65 per yard frame). Needs #4 first | [I] |
Do not repeat: combiner-flush skip, texture-bind skip, per-op timers (HANDOFF). [M]

## 7. Verdict
- Fits: texture cache and format mapping, VRAM slab, combiner to TEV, GPU vertex path with matrix palette, KICK1 split, I/O on core 1.
- Does not fit: re-walking the whole N64 list on the ARM11 every frame, batch closes at N64 op granularity, per-texture data in uniforms, no use of the N3DS spare core.
- Next step: #0 and #1, then #2 as the epic. #3 is the best single row while #2 runs.
