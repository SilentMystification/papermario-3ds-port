# plan_toad_33: Toad Town (mac_01) on O3DS, 48.5 ms to 33.4 ms
Plan only. Rules: hw_guidelines.md. Tags: [M] measured, [E] estimate, [C] code read, [L] citro3d behaviour.
Sources: build3ds/ftp_170c/hw_o3ds_log.txt (build 1462, full set, Toad Town lines 501-1202, steady windows f732-768 and f840-877), diag_hwlog3 (build 1467, cheap set, 48.49 ms).

## 1. Where the 48.5 ms goes (cheap set)
| Part | ms | Tag | Basis |
|---|---|---|---|
| gbi walk, closes not included | 26 to 29 | E | full set gbi 32.1 to 33.1 [M], minus phase hooks 3 to 4.5 (diag_logcost) |
| of it: G_PM_TEX bind body | 4 to 5 | E | opsplit f899 tex class 31 % of walk [M], minus closes |
| of it: dlc replay, 56 per frame, about 100 us each | 5 to 6 | E | opsplit dlc 18 % [M]; dlc2 static 100 % [M] |
| of it: cc / geo+lights / mtx / tri+vtx / dl+oth | 3 / 3 / 2.5 / 4 / 3 | E | opsplit shares [M] |
| Closes (gpu + gdraw phases) | 5.3 | M | 76.6 x 69 us; full set gpu 3.1-3.3 + gdraw 2.3-2.5 |
| FrameBegin GPU wait (fbegin) | 3.5 to 5 | E | full set 3.2 to 5.1 [M], p95 8 to 10 |
| Game logic (step_game_loop) | 3.2 to 3.5 | M | full set |
| Display list build (dl) | 3.6 to 3.9 | M | full set |
| FrameEnd (fend) | 0.8 to 1.1 | M | full set |
| other (no phase) | 1 to 3 | M | 6 to 11 ms on part of the frames |
| tex, gcache, audio on core 0 | 0.3, 0.01, 0 | M | |
- Sum 46 to 51 ms. The CPU is the limit (GPU busy about 17 %).
- Order in a drawn frame [C pm_main.c:442, pm_nusys.c:85]: FrameBegin waits for the GPU tail of the last frame, THEN logic, dl and gbi run. The GPU tail and the logic do not overlap. This is the cheapest lever.
- Console mops f899 [M]: cc_same 85 of 115, tex_same 29, geo_same 53, lt_same 157 of 157. The Azahar post-office view gave 35/15/12. So "repeats are rare" is not true for the console view.

## 2. Answers to the 7 points
1. Second core. Core 1 already runs the log writer, the sink, the watchdog, the audio control and the mixer [M log #1, #874; C pm_au2.c:169]. Core 0 pays 0 ms audio in steady state. The app limit is 80 % (cpu_game, pm_log.c:635 [C]). Free share about 40 to 60 % = 13 to 20 ms per 33.4 ms frame [E]. The producer (logic + dl, 7.1 ms) fits. The gbi run (26 to 29 ms plus 5.3 ms closes) does not fit, and citro3d calls must stay on one thread. O3DS gain after LB1 is only logic + dl minus the exposed GPU wait: 2 to 4 ms [E], 300+ lines, high risk. N3DS: cores 2 and 3 make it a full pipeline.
2. Retained static model draw. Left per static map model [E]: about 11.7 top-level ops at 4 to 9 us, a bind body about 40 us, the dlc replay about 100 us, and a close of 69 us when the texture changes. About 190 us per model x 77 = about 15 ms. A G_PM_MODEL op can save 3 to 7 ms. Measure first (MR0).
3. Close merge. Each of the 76.6 closes is a real texture change (bind_w1 8 of 77). A sort only helps inside one run of opaque static surfaces. Picture risk only at equal Z. Ceiling 1 to 2 ms if MS0 shows 20 or more removable closes. The 19 HUD tail closes need an atlas: does not pay.
4. Spike frames over 50 ms (8 %). Not texture loads, not SD, not GC, not audio [M]. Causes [M f804-877]: (a) the view: draws 61 to 100 and gbiops 1598 to 2045 make gbi 31 to 46 ms; (b) "other" 6 to 11 ms on part of the frames, not timed; candidate pm_mem_hud (mallinfo and linearSpaceFree at 1 Hz, pm_mem.c:206) [E]; (c) fbegin 6 to 10 ms after a heavy frame. LB1 removes (c). MR1 cuts the slope of (a).
5. Draw stage 30.6 us [C pm_gpu.c:1054-1067, L]: sh_user_clip, BufInfo, then C3D_DrawElements. citro3d C3Di_UpdateContext emits here every dirty group set in the earlier stages. So "draw" is the command emission of the whole close. The cmd field reads 0: it is broken. Fix the field and count dirty groups first (DC6).
6. Producer side: logic 3.3 ms and dl 3.8 ms [M] = 15 % of the frame. No game-logic row until fsplit shows logic above 5 ms. The render task sort (model.c:4807-4858, O(n^2)) is about 0.2 to 0.5 ms [E].
7. Fallback. 25 fps does not divide 60 Hz: uneven pace. A stable 20 fps (3 vblanks) with FS1 skip and full game speed needs a drawn frame of 46 ms or less and a logic-only step of 4 ms or less [E]. LB1 alone reaches it.

## 3. Rows
| Row | Change | File, function | Console ms | Risk | Azahar check | Order |
|---|---|---|---|---|---|---|
| FSPLIT | in progress: logic/gbi/flush/gwait/swap/audio line | pm_main.c, pm_phase.c | 0 | none | line present | 0 |
| LB1 | Lazy FrameBegin when pm_interp_on <= 0: pm_gpu_begin keeps the CPU part; C3D_FrameBegin and the texture/noise/grab setup move to gpu_begin_wait(), called once at the first gbi_task (nuGfxTaskStart), at pm_gpu_end if not called, and at every CPU read of GPU memory (pm_gpu_read_frame, pm_gpu_depth_at, grab, pm_gpu_off_begin). fps60 path unchanged | 3ds/src/pm_gpu.c pm_gpu_begin 1760-1796; pm_nusys.c nuGfxTaskStart; pm_depth.c | 3 to 4.5 | low-medium (a missed GPU-memory reader) | mac01, mac01t, yard, pause_menu, battle swirl shots byte-equal; fbegin phase down | 1 |
| R5 | Skip equal light MOVEMEM/MOVEWORD (lt_same 157/157) in g2_same; guard every lt[] writer | 3ds/src/pm_gbi.c g2_same | 0.5 to 0.8 | medium | mac01t byte-equal; mops lt skips 150+ | 2 |
| R2 | Exact-equal SETCOMBINE/TEXTURE/GEOMETRYMODE skip, also box_folded == 0 and sc_ok | pm_gbi.c g2_same | 0.7 to 1.1 | medium | mac01t, yard, pause_menu byte-equal; cc+geo us down | 3 |
| MR0 | Census, phase only, cold: plain static model tasks per frame, sampled us per model split ops/bind/close/dlc; dlc replay split in 4 parts | src/model.c appendGfx_model (TARGET_3DS mark); pm_gbi.c, pm_dlc.inc.c | 0 | none | line present; byte-equal | 4 |
| LB1b | If gwait after LB1 is above 1 ms: record the bg task and run its gbi just before the main task | pm_nusys.c | 0.5 to 1.5 | medium | bg sky byte-equal | 5 |
| MR1 | G_PM_MODEL: model.c emits one op with a per-model record for plain static models; a differing key runs the original list. Gate: MR0 shows 50+ plain models and 120+ us per model outside dlc | src/model.c appendGfx_model; pm_gbi.c | 3 to 7 | high | 6 maps and mac01t byte-equal; prod model ops down 600+ | 6 |
| MS1 | Stable sort by texture inside one opaque static run only. Gate: MS0 count of removable closes 20+ | src/model.c execute_render_tasks | 1 to 2 | medium-high (equal Z) | 6 maps, 2 runs, byte-equal | 7 |
| DC6 | Fix the cmd field (words per close); count citro3d dirty groups per close; drop repeated groups at the shadow | pm_gpu.c flush_draw | 0.5 to 1.5 | medium | dcsum draw us down 20 %; byte-equal | 8 |
| SPK1 | Time pm_mem_hud; if 3+ ms, call mallinfo only when the memory rows show, else every 5 s | pm_main.c 512-520, pm_mem.c | avg 0.1 to 0.5; pace | low | HUD rows present | with FSPLIT data |
| CORE1 | Producer (logic + dl) on a core-1 thread one frame ahead; gbi and submit on core 0 | pm_main.c, pm_nusys.c | O3DS 2 to 4 after LB1 | high | full catalog | 9, only if above 36 ms after MR1 |
| FPS20 | Fallback: stable 20 fps cap, FS1 keeps game speed | pm_main.c | pace | low | yard pace | user decision |

Rows that do not pay: DS4 (8 us per bind), DS5 (failed gate), gcache (0), dlc transform skip (100 % static), MTX skip (mtx_same 0), entity/sprite/HUD rows (done), HUD atlas (19 closes, high risk), -Os pm_gpu.c (DS2 failed), W4/W2 (failed), gbi on core 1 on O3DS (does not fit the slice), log writer or audio move (already on core 1), 25 fps cap (uneven), culling or lower effect rates (forbidden).

## 4. Projection
48.5 - LB1 4 - R5 0.6 - R2 0.9 = about 43 ms (41.5 to 44.5). Then MR1 5, MS1 1.5, DC6 1, LB1b 1: about 34.5 ms (31 to 38). 33.4 ms needs MR1 at its mid-high end, or CORE1 after it. Honest result: 30 fps on O3DS is possible but not sure; stable 20 fps is sure after LB1.

## 5. First 3 rows for one Haiku code agent
1. LB1 (about 40 lines). 2. R5 (about 25 lines). 3. R2 (about 40 lines). One Azahar start per row; a row that changes one picture byte is reverted. Console push at the end.
Projected Toad Town after the 3 rows: about 43 ms (41.5 to 44.5), 23 fps; stable 20 fps with full game speed.

## 6. What is needed from the fsplit line (steady Toad Town, 3+ windows, mean and p95)
- gwait: 2.5 ms or more: LB1 first, expected save = min(gwait, logic + bg dl). Below 1 ms: drop LB1, start with R5.
- logic and dl: if logic + dl - gwait is under 3 ms, CORE1 does not pay on O3DS. If logic is above 5 ms, add a logic split row.
- gbi and flush: if gbi is 55 % of the frame or more, MR0/MR1 come before MS1 and DC6. flush x 1000 / 76.6 gives us per close for DC6.
- swap: above 1.5 ms means a FrameEnd row (KICK placement) before DC6.
- audio: must be 0; if not, check pm_audio_join_at waits.
- residual (wall minus the sum) and its max: 3 ms or more on single frames confirms SPK1.
