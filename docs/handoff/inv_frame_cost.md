# inv_frame_cost: where frame time goes on the O3DS (read only)

Question: on the Old 3DS (O3DS), where does frame time go, and which part can a native replacement of the display list (DL) walker remove? Goal: 60 fps (16.7 ms per frame). Now: 24 to 28 fps with logging off (36 to 42 ms per frame).

Source data: console build 1588, git bff514be5, file build3ds/ftp_170b/hw_o3ds_log.txt. Switches: `phase fsplit pmu loglevel=debug` (line 12). Phase on. All numbers are phase on unless marked "phase off". Phase on runs slower than play. Use the factor 0.74 to 0.79 for phase off (docs/handoff/plan_dl_design.md section 2).

Abbreviations: DL = display list. GBI = graphics binary interface (the N64 command list). Walker = the code that reads the DL command by command (src/pm_gbi.c). Close = one batch closed and sent to the GPU (flush_draw, src/pm_gpu.c). PICA = the GPU command set. TEX = texture. TEV = texture combiner. st = state set (depth, alpha, scissor, cull, blend). pal = palette and uniform upload. bind = texture bind. draw = citro3d draw call (CPU side write of the draw command).

## 0. Prof line status (requirement from the coordinator)

- No console log in the tree has a `prof` line. Checked: build3ds/*.txt, build3ds/*.log, build3ds/ftp_*/, build3ds/hw_pull/, build3ds/azt/*.txt, 3ds/logs/perf/*.txt. The `prof` switch exists in source (commits 6656d5870 PF1, 286dcac01 PF2, 9491c1222 PF3; line 333 of 3ds/src/pm_phase.c). The push record in 3ds/logs/perf/pushed_builds.txt shows no push with `prof`.
- The Azahar test `mac01t_prof` is not in docs/handoff/test_catalog.md. The PF epic has no recorded Azahar check. CLAUDE.md rule 6 blocks a console push of `prof` until the epic check passes.
- Push arguments when the check passes (docs/handoff/plan_prof_mode.md line 160): `PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 prof`. A second push with `prof profstall` gives the instruction-fetch stall share.
- Scope of the prof line (3ds/src/pm_phase.c:333): gbi, dlc, tex and gpu per sampled frame, plus frame time (ft), max frame time and frame-interval bins (iv). It does not cover the five close stages (pal, bind, tev, st, draw), the walker split inside gbi, the kick, or the GPU wait inside gbi.
- Therefore every number below comes from the older fsplit, dcsum, opsplit, mr0 and phase lines of build 1588. None comes from prof.

## 1. Table: ms per frame by stage (console, build 1588, phase on)

Three named scenes, all in Toad Town (area 1, map 2, loaded at line 340). Each scene is one 300-frame window. The dcsum window ends 14 frames before the fsplit window (same offset in every pair), so the close numbers are paired with the nearest dcsum window.

- S1 = still camera, window end f1813. Log lines: fsplit 945, dcsum 909, opsplit 918, mr0 943.
- S2 = early Toad Town, window end f1213. Log lines: fsplit 635, dcsum 598, opsplit 608, mr0 633.
- S3 = camera moving, window end f2713. Log lines: fsplit 1420, dcsum 1376, opsplit 1393, mr0 1418.

Log file: build3ds/ftp_170b/hw_o3ds_log.txt. Line numbers are from grep -n.

| Stage | S1 | S2 | S3 | Source (file:line) |
|---|---|---|---|---|
| Game logic (fsplit `logic`) | 3.2 | 3.3 | 3.4 | log 945, 635, 1420; pm_phase.c:673 (mark before game step) |
| Game DL build (fsplit `dl`) | 4.4 | 4.2 | 4.4 | log; pm_phase.c:675 and pm_nusys.c:119 (game draw code before the task) |
| Audio (fsplit `audio`) | 0.0 | 0.0 | 0.0 | pm_nusys.c:98 |
| gbi total (fsplit `gbi`, walker plus close) | 45.6 | 37.2 | 43.4 | log; pm_nusys.c:99 and 79 (gbi mark and begin wait) |
| - walker / interpret (gbi minus close) | 39.8 | 32.8 | 38.0 | derived: gbi minus close total |
| - close total (dcsum cl x sum of stages) | 5.77 | 4.37 | 5.43 | log 909, 598, 1376; pm_gpu.c:2691 (dcsum print) |
| - close pal | 1.10 | 0.94 | 1.07 | dcsum us per close x cl; pm_gpu.c:822-830 |
| - close bind | 0.66 | 0.47 | 0.60 | same |
| - close tev | 0.66 | 0.47 | 0.60 | same |
| - close st | 0.80 | 0.57 | 0.74 | same |
| - close draw (citro3d call) | 2.56 | 1.92 | 2.41 | same; draw marks pm_gpu.c:920 and 1166 |
| - close kick | not printed | not printed | not printed | measured in pm_gpu.c:829 (not in dcsum print) |
| Frame begin GPU wait (fsplit `gwait`) | 0.1 | 0.1 | 0.1 | pm_gpu.c:2011 |
| Frame end flush (fsplit `flush`) | 0.3 | 0.3 | 0.3 | pm_gpu.c:2053 |
| Swap (C3D_FrameEnd, end of frame work) (fsplit `swap`) | 0.8 | 0.8 | 0.8 | pm_gpu.c:2095 |
| Rest (fsplit `rest`) | 0.6 | 0.5 | 0.6 | pm_phase.c:226 |
| Wall (fsplit `wall`) | 55.0 | 46.6 | 52.9 | pm_phase.c:226 |

Sub-split of walker classes (opsplit, us per frame, /1000 = ms; these are inclusive of nested closes):

| Class | S1 | S2 | S3 | Source |
|---|---|---|---|---|
| 0c model class (G_PM_MODEL, nested model work) | 20.3 | 19.9 | 19.7 | log 918, 608, 1393; pm_gbi.c:2275 |
| 0b texture state (G_PM_TEXI) | 6.7 | 3.2 | 5.8 | same |
| all texture class (`tex`) | 7.6 | 3.7 | 6.8 | same |
| all other named classes (vtx, mtx, tri, cc, geo, dl, dlc) | 11.0 | 10.5 | 11.5 | same, sum of the fields |

Cross-checks:
- Per model census (mr0, 90 plain models per frame, pm_gbi.c:2231): close 54 us per model in S1 (log 943), so 4.9 ms per frame. The dcsum close total in S1 is 5.77 ms. The difference (0.9 ms) is closes outside plain models.
- The opsplit class sum (S1: 44.8 ms) matches the fsplit gbi bucket (45.6 ms) within 2 percent.
- The mr0 line also prints `console x2.86`. That factor is an Azahar to console ratio. It does not apply to this console log.

Not in the table: the GPU execution time. Build 1588 has no gpusplit line. The older build 1227 has one (build3ds/hw_pull/hw_o3ds_log.txt, Yard). diag_dc_console.md reports gdraw 8.2 ms and idle 8.5 us per frame for that build. The source of the gpusplit line is not in 3ds/src (grep finds none). Its labels are not verified.

## 2. CPU display list cost against GPU and other

- CPU display list cost (walker plus close, the fsplit gbi bucket): S1 45.6 ms (83 percent of wall 55.0). S2 37.2 ms (80 percent of 46.6). S3 43.4 ms (82 percent of 52.9).
  - This bucket includes the GPU tail wait (pm_gpu_begin_wait in gbi_task, pm_nusys.c:79). The wait has no own mark, so its size is not known from build 1588.
- Game code that is not the walker: logic 3.2 to 3.4 ms and DL build 4.2 to 4.4 ms (about 14 percent of wall).
- GPU: no direct number in build 1588. Two proxies only. The frame begin wait is 0.1 ms (gwait), so the GPU does not hold the frame at its start. The gbi tail wait cannot be split. So a GPU limit is not ruled out, and not shown.
- Other CPU: flush 0.3, swap 0.8, rest 0.5 to 0.6, audio 0.0 ms. Total about 1.7 ms.

## 3. Ms per frame a full native walker replacement could remove

Upper bound: the walker part of gbi, all of it (S1 39.8 ms, S2 32.8 ms, S3 38.0 ms). A native path still needs the close and the citro3d draw calls (5.4 to 5.8 ms). It still needs the game DL build and logic (7.6 ms). The walker part also holds model work that a native path still computes (matrix loads, texture state, DLC box test and replay). So the upper bound is not reachable.

Realistic estimate (assumptions, not measured; from docs/handoff/plan_dl_design.md section 3):
- Texture state, class 0b (path 1): 50 to 80 percent of 3.2 to 6.7 ms gives 3.2 to 5.4 ms phase on (S1 basis).
- Per-model record for static plain models (path 2): 50 to 80 percent of the ops part (15.9 ms phase on, 90 models at 177 us, log line 943) gives 7.9 to 12.7 ms phase on.
- Sum: 11.1 to 18.1 ms phase on, which is 8 to 14 ms phase off (factor 0.74 to 0.79).

Effect on the goal: the phase-off frame is 36 to 42 ms now. The realistic removal (8 to 14 ms) leaves 22 to 34 ms. This does not reach 16.7 ms by itself. The upper bound (walker minus close, 24 to 31 ms phase off) would be enough only if the whole walker part went away. That is not plausible.

## 4. The st stage (10 us per close) and the 31 us gap

Stage st (pm_gpu.c:1103 to 1152, DC_T(3) to DC_T(4)): sh_depth_test, sh_depth_map, sh_early, sh_alpha_test, scissor (signed divide by 4, which is a shift), sh_cull, blend and blend_off. No timer is inside (the fine phase switch PHF is off, pm_port.h:100).

Answer for the logs:
- Build 1588: st is 11 us per close (dcsum, log 909). Build 1227: 10.0 us (diag_dc_console.md). Stage sample: one close in 8 (pm_gpu.c:2770).
- About 1 us of each stage is timer overhead. One tick read costs 1.029 us on this console (build 1588 log line 12). Each stamp (DC_T) adds its read cost to a neighbouring stage. This is a fact about the method. The exact split is not measured.
- The compare work costs about 100 to 300 cycles per close (map_rtb1.md section 0), which is 0.4 to 1.1 us at 268 MHz. So the compares do not explain the rest.
- Hypothesis (not verified): the setters write PICA command words into the command buffer. Build 1588 prints 70 to 71 words per sampled close in the S1 to S3 windows (dcsum line 909 field cmd). A per-setter timer or a per-stage word count would confirm or reject this. The logs do not give it.

The 31 us gap:
- Build 1588 cannot answer it. It has no flush total per close. The flsplit line is absent in this build. The opsplit `fl` field is a sum over one top-level op in 64 (pm_gbi.c:2003), not a total, so it is not used.
- Build 1227 (diag_dc_console.md): total 97.4 us per close, stage sum 66.3 us, gap 31 us. The total is a subtraction, (7229 - 267 x 2.6 - 10) / 67. The 2.6 us per empty close comes from diag_ec2.md and was not measured in build 1227.
- Known code with no mark, which can hold part of the gap: flush_draw entry (pm_gpu.c:952 to 982, before DC_T(0)), the tail of flush_draw_cpu after DC_T(5) (pm_gpu.c:920 onward), and the kick (pm_gpu.c:835 to 838, about 1.4 us). These are estimates. The logs do not split them.
- To close the gap, one new mark before flush_draw entry and one after the flush_draw_cpu return, plus a flush total per close in the dcsum line, is needed. That needs a new console run with `phase fsplit` and a new code row. Under CLAUDE.md rule 3, a fresh investigation agent must do that work.

## 5. Recommendation

Cut the texture-state ops of the walker (class 0b, G_PM_TEXI) first. It is the one measured block that does not depend on model content. S1 shows 6.7 ms phase on (S2 3.2, S3 5.8). Its producers are known: src/sprite.c:236, src/entity/Shadow.c:28, src/hud_element.c:365 and 477, src/msg_draw.c:1758 (plan_dl_design.md section 3). The first row should cover one producer, as plan_dl_design.md proposes. The class 0b time may include closes that texture changes trigger. This is not verified, and it lowers the gain if true. Log the 0c model class split (matrix, bind, DLC replay, close) in the same console push, because 0c is 20 ms and it is not split yet (plan_dl_design.md section 4). Do not start the close stages (5.4 to 5.8 ms; RTB1 ceiling about 0.4 ms by map_rtb1.md) before the walker part is cut. The walker cut does not remove the close stages.

## 6. Limits

- One console build (1588). Phase on. Three windows in one map. No logging-off console number exists for this build.
- No prof line exists yet (section 0). No GPU busy time exists for build 1588.
- The window pairing (14 frame offset) is by position in the log. Each dcsum window and each fsplit window is 300 drawn frames.
- The factors 0.74 to 0.79 and the 50 to 80 percent cut shares are assumptions from earlier plan documents.
