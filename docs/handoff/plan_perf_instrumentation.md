# Plan: whole-frame performance instrumentation (Azahar and console, same lines)

Branch dlc-compiler-c1c2, HEAD 4d88c3355. Rules: pm_ticks() costs about 1.3 us, so no tick read per op. Counters per frame and totals per class only. No toggles: every field prints under the existing `phase` switch. Do not restore the combiner-flush skip or the texture-bind skip. Azahar numbers show direction only. The console gives the speed result. Do not use `gbiprof` for timing (it reads ticks per op).

## 1. Existing instrumentation and gaps
| Stage | Exists (file:function) | Gap |
|---|---|---|
| Game logic | PH_LOGIC, `__wrap_step_game_loop` (pm_phase.c) | none |
| RSP task / DL walk | PH_GBI in `nuGfxTaskStart` (pm_nusys.c), walk time without nested phases. `n_ops` to C_CMD (`pm_gbi_end_frame`). RS_DLC counts DLC hits | No tasks-per-frame count. No split by task kind (background, main, F06 key replay). No hop counters (G_DL push, G_ENDDL return, NOPUSH, `host_range` miss, max depth, pdl hits) |
| Op streaks | `gbisplit sync/gv/tex` (pm_gbi.c `gv_streak_op`, `tex_streak_op`, `sync_combine_work_timed`) | `tex` streak includes the `pm_gpu_flush_at` batch close |
| Batch close | C_FLUSH count, PH_GPU in `flush_draw` (pm_gpu.c), cause by line only under `gbiprof` | No time by cause class. No empty-batch count |
| Texture | PH_TEX on a miss (pm_tex.c `load_inner`), C_TEXCALL/TEXMISS/TEXBYTE/FLCALL/FLKB, `texstat` every 300 frames | Hit time only under `gbiprof` (`tprof_*`). `pend_flush_one` (GSPGPU_FlushDataCache) not timed |
| GPU submit / wait | PH_FBEGIN, PH_FEND, PH_GPU in `kick_check`, `pm_ph_gpu_note` (C3D_GetDrawingTime/ProcessingTime of previous frame), `pm_ph_cmd_pm` | GPU times print on `pcx` lines only. FrameSplit time mixed into PH_GPU. `kick_n` prints every 300 frames |
| Vsync / present | PH_IDLE (pm_main.c `pm_frame_loop`), `pacestat`, `spike` | none |
| Log writer | `logstat`, `logstat2` (pm_log.c) | Game-thread cost of the `phase` lines and `pm_gpu_hud` not measured |

## 2. Rows (each about 60 changed lines or less, all under `pm_ph_on`)
- PI1 Task and hop counters. `nuGfxTaskStart`: one tick pair per task, kept for 3 kinds (background = no swap flag, main, key replay in `pm_interp_key_draw`), plus `n_ops` at entry and exit. `pm_gbi_run` cases G_DL and G_ENDDL: counters for push, NOPUSH, return, `host_range` miss, max `sp`, `pdl_try` and `pdl_try_m` hits. No tick reads in the op loop. Add fields to `rsplit`. Files: pm_nusys.c, pm_gbi.c, pm_port.h (RS_ enum), pm_phase.c `pm_ph_frame_end`. ~45 lines.
- PI2 Batch-close cost by cause class. pm_gbi.c stores the current op in `pm_gbi_op` (one store per op, no tick). `pm_gpu_flush_at`: one tick pair around `flush_impl`, kept per class TEX (SETTIMG, SETTILE, SETTILESIZE, LOAD*, G_TEXTURE), CC (combine, prim, env, othermode, fog), GEO (MTX, POPMTX, GEOMETRYMODE, MOVEWORD, MOVEMEM), RECT, TRI. Flushes not from pm_gbi.c (`flush()` macro: `pm_gpu_end`, `set_mvp`, off-target) go to class GPU in `flush_impl`. Count empty batches per class. Add to `gbisplit` as `fl <cls> n/e/us`. `gbisplit tex` stays. Files: pm_gbi.c, pm_gpu.c, pm_phase.c. ~55 lines.
- PI3 Texture hit time. In `load_impl`, change the gate of the existing `tprof_*` timer from `gbiprof` to `pm_ph_on`. Start `t0` after `pm_gpu_rect_sync()` so a close is not counted twice. Time `pend_flush_one`. Add to `gbisplit` as `th n/us tm n/us dc n/us`. `pm_tex_prof_report` reads the same counters. Files: pm_tex.c, pm_phase.c. ~30 lines.
- PI4 One line per frame: `gpusplit f cpu fb fe idle gdraw gproc kick n/us draws cmd vb`. In `kick_check`, put C3D_FrameSplit in its own tick pair (kept out of PH_GPU). `gdraw` and `gproc` are of frame f-1, and the line says so. Vblank interval `iv` from pm_main.c to a global. Files: pm_gpu.c, pm_phase.c, pm_main.c. ~40 lines.
- PI5 Logging cost on the game thread. In `pm_ph_frame_end`, one tick pair around the `rsplit`, `gbisplit`, `combsplit`, `gpusplit` lines. Print the value next frame as `gpusplit ... log us`. One tick pair around `pm_gpu_hud`, add as `hud us`. Files: pm_phase.c, pm_gpu.c. ~15 lines.

Order: PI1, PI2, PI3, PI4, PI5. Each row commits after its shot passes. PI2 and PI3 both change `gbisplit`, so they run one after the other.

## 3. Azahar test per row (one start each, row budget 3)
Plan change: the default test scene is the attract loop, not the yard preset. Run over a window that covers several scenes. Report per-scene numbers (rsplit, taskpl, gbisplit, flsplit, thsplit lines grouped by scene). No attract-loop Debug switch exists in 3ds/src (grep of pm_main.c and pm_warp.c found only demoscene=<n> and demobattle). Until that switch exists, the yard command below stays in use.
Command: `PM_ROW=<row> powershell -File 3ds/run_azahar.ps1 -Until 'shot 40 frame' -Debug 'loglevel=debug phase shots shotmode=4:80 preset=yard battlef=80'`. The shot must show Mario, the ground and the shadow. Read frames 80 to 90.
| Row | Quantity (one per shot) | Pass |
|---|---|---|
| PI1 | `rsplit` task fields: tasks per drawn frame, us per task | Fields present. Per frame, sum of task us equals phase gbi + gpu + tex within 5 % |
| PI2 | `gbisplit fl TEX us` | Present. Sum of classes equals PH_GPU (excluding kick) within 5 %. TEX n about 65 |
| PI3 | `gbisplit th us` | Present. th n + tm n = C_TEXCALL for the frame |
| PI4 | `gpusplit gdraw` against `cpu` | Present on every drawn frame. kick n is 0 or 1 |
| PI5 | `gpusplit log us` | Present. Frame wall equals the PI4 run within 1 ms |
After PI5: one console push (CLAUDE.md rule 6 command). Compare the same fields at the yard.

## 4. Decision table (yard, console numbers rule; Azahar shows direction)
W = phase gbi (walk only), C = sum of `fl` us, X = th + tm us, G = gdraw, ops = C_CMD, hops = push + return + range miss.
| Measured result | Supports |
|---|---|
| Tasks > 2 per drawn frame with a large background or key task, OR hops/ops > 0.15 and W > 50 % of wall, OR empty closes > 20 % of closes | (a) tasks split badly / too many hops |
| C + X > W, and C/closes > 30 us on console, AND G < 40 % of cpu with fb near 0 (GPU waits for CPU, small batches) | (b) RDP state model does not fit the 3DS CPU/GPU split |
| fl TEX and fl CC dominate C AND W/ops stays constant while closes/ops is high. Cross-check on file-select flip (SLICES PT86: 231 draws) | (c) both come from display-list work |
| fb > 30 % of wall | none: GPU-bound, look at fill rate |
| logic > 40 % of wall | none: game thread |
| log + hud > 1 ms | instrumentation cost: fix logging first |
Compare between Azahar and console: us/op of W, us per close of C, us per hit of X. Absolute Azahar ms are not valid.

Skipped: per-op timing (forbidden), new phases (phase time is already exclusive), new switches (no toggles).
