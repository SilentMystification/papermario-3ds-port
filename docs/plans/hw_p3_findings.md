# Push 3 findings (build 926, O3DS)

Written 2026-10-21 by P3-ANALYSIS. Read-only analysis. No build, no Azahar start, no console access.
Source: 3ds/logs/perf/hw_pull3/hw_o3ds_log.txt (22550 lines). Switches: `audiostat pacestat logstat phase phsum noaudit loglevel=debug fixedseed statehash=10 phasecsv=300:400 abtog=base,gpuvtx,nodr1,nokick,notx1,phasegbi:30`.
Tables (not committed): 3ds/logs/perf/hw_926_P3_table.txt, hw_926_P3_ratio.txt, hw_926_P3_seg.txt.
Tags: M = measured. E = estimate from code or counts.

## 0. Read this first

1. The user played by hand. The first 4449 steps (logos, intro, title) are hands-off: `statehash f1800 5a6bb12a` matches the expected value. After the title the demo cycle ran, then the user walked in world maps 1:1, 1:2, 4:8, 5:0, 5:1, 0:15 (hundreds of steps each). So the world data is "hand play", with heavier maps than the demo cycle.
2. What each A/B state is (pm_phase.c:ab_words, ab_apply): `base` = the build default (DR1, KICK1, TX1 on, gpuvtx off). Every other state changes ONE thing from base: `gpuvtx` on, `nodr1`, `nokick`, `notx1`, or `phasegbi` (fine gbi timers on). The A/B rotation is by drawn step, so each of the 5 non-gbi states has about 2700 world steps (13812 pooled) and every big scene has all states.
3. Steps per second and drawn steps are two different numbers now. FS1 skips the draw of a late step but the game logic still runs. The `pace:` line counts game steps (`steps`). `fs1 skipped 15091 of 39600` at the end = 38.1 percent of steps had no draw. The `phs`, `pc` and `phase W` lines count drawn steps only.
4. Method of the gains in part 2: for each scene (mode, area, map) with at least 20 samples in both states, delta = state minus base. Then the mean of the deltas weighted by min(n base, n state). Spread = min and max of the per-scene deltas. cpu50 is the n-weighted mean of the window medians. Scratch scripts are in the session scratchpad, not committed.
5. Dumps: 38 dumps (crash_dump_00000000 to 00000037). None is newer than 00000037. `diff -rq` against hw_pull2/hw_o3ds_dumps shows all 38 files are equal to the build 902 pull. No new crash dump.

## 1. Measured gain per row (part 2 of the task)

All numbers are hardware ms per drawn step unless said otherwise. "World" = mode 4. n = drawn steps summed over the compared scenes.

| Row | Compare | Result in world | Result, all scenes | Goal | Verdict |
|---|---|---|---|---|---|
| TX1 | notx1 against base, tex ms | -0.54 ms (11 scenes, n 2564, scene range -4.03 to +0.67, median -0.04) | -0.41 (17 scenes, n 3161) | tex of step 332 under 25, step 337 under 35 | No gain (M) |
| TX1 | tex cost per MB, all windows | base 247 ms/MB, notx1 251, gpuvtx 223, nodr1 233, nokick 243, phasegbi 239 | | | Same cost with and without TX1 (M) |
| TX1 | spawn steps (pc lines, state phasegbi = TX1 on) | step 332: tex 101.2 ms (64 misses, 400 KB). Step 334: tex 104.5 ms (57 misses, 417 KB). Step 331: tex 0, ro 45.0 ms. Steps 335 to 339: tex 0 | | Build 902: 332 = 82.1, 337 = 113.3 | No gain (M). 202 ms in this run against 195 ms in 902 for the same spawn. The spawn steps moved from 332/337 to 332/334 |
| TX1 | `tx1 f` lines (ns per byte of the tx1 path) | 331 lines, 80 to 437 ns/B, typical 180 to 250 | | Azahar new path 41, old path 154 | HW is 4 to 10 times the Azahar cost. The notx1 old path costs the same per byte (above) |
| DR1 | gp+gd per draw, nodr1 against base | base 66.1 us, nodr1 -0.09 us (12 scenes, n 2492, scene range -0.83 to +1.33 us) | base 65.2, nodr1 65.6 (+0.4, 18 scenes) | under 45 us (was 63) | No gain (M). Draws per step stay 98 |
| DR1 | `dr1 f` lines | merged 0, splits 0, uniform skips 3516 on f300 | | | Rect merge never fired in this run. Uniform skips fire |
| KICK1 | nokick against base, cpu50 | -0.88 ms (11 scenes, n 2548, median -0.20, range -12.30 to +5.08) | -0.31 (17 scenes) | ok share above 60 percent at cpu 29 to 31 ms | No gain (M) |
| KICK1 | on-time share (wall up to 34.5 ms) | base 11.4 percent, nokick +0.09 | base 15.6, nokick +1.2 | | World steps are all far over 34.5 ms, so the share cannot move |
| KICK1 | GPU wait (fb column, ms) | base 9.92, nokick +0.02 (median scene +0.22) | 9.98 and 9.86 | | None |
| KICK1 | gpu field of phc (citro3d drawing time, us per step) | base 7677 us, nokick -6 us (median -77) | 7661 and 7555 | | The GPU needs 7.7 ms of a 55 ms step. It is idle, so an earlier kick gains nothing (M) |
| FS1 | `fs1 skipped N of M` | 15091 of 39600 (38.1 percent). Per 300-step line: 0 of 300 early, 4195 of 11700 (35.9 percent), 9671 of 23400 (41.3), 14794 of 38700 (38.2) | | few skips | Skips are many, not few. They are the reason steps per second rose, see part 3 |
| AU2 | `audio wait avg` per step (core 0 join) | 135 windows, mean 3.37 ms, median 3.42, p90 5.25, max 7.67 | | Build 902: 31 windows, mean 10.07, median 9.47, min 5.14, max 19.67 | Wait falls 6.7 ms per step (-67 percent) (M) |
| AU2 | `audio cost avg` per 60 Hz tick | 829 lines, mean 3.07 ms, median 3.19, max line 5.55 | | 902: 249 lines, mean 7.34, median 7.22, max 11.53 | -4.3 ms per tick (M) |
| AU2 | `audio2:` lines (131) | jobs per 300 steps 873 to 1338, cmds/job avg 2 (max 7), emit cost 4 us per job, ring fill min 1 and mean 2.0 to 3.0 of 8, ring full waits 0 (0 us), render busy mean 3.3 to 4.0 ms (max per job 25.8 ms), drains: bank 1 (56 us), map 0, save 0, shutdown 0 | | | Ring never full. Render thread uses about 3.5 ms per job (E: it runs on core 1) |
| AU2 | `audio pcm:` | 96940 posts, 9302 underruns (9.6 percent), 0 dropped. By scene type: logos 0.7, intro 8.5, menu 7.9, world 10.0, battle 8.5 percent | | | New line. No 902 value. 1 post in 10 finds the ndsp queue empty. Needs an ear check by the user |
| AU2 | `au` phase in the pc and phs lines | world 5.58 ms per drawn step (902: 0.1 to 1.0 on the same maps) | | | The wait moved into the `au` phase. It is not comparable with 902. The step cpu did not rise: on 9 world maps in both builds cpu is 902 to 926: -5.2 (4:20), -5.1 (6:1), -10.0 (17:4), +2.4 (6:5), -1.8 (14:5), -0.7 (2:2), -0.2 (18:1), +3.3 (1:4), +18.2 (4:5) |
| gpuvtx | gpuvtx against base, gb | -0.39 ms (15 scenes, n 2695, median -0.75, range -7.18 to +1.45) | -0.18 (21 scenes) | | No gain, as the GV1 plan predicts for the partial path (M). gd -0.08, cpu50 -0.27 |
| GV0 | no picture check in this log | | | | The log cannot show the picture. The user must say if gpuvtx steps looked wrong |

Same-map change from build 902 to 926 (cpu per drawn step, 902 = mean of pc lines, 926 = mean of window medians; maps with at least 20 samples in both): gb falls 1.0 to 3.9 ms on the same maps (4:20 26.9 to 22.8, 6:1 43.8 to 39.5, 17:4 43.0 to 34.8, 14:5 23.7 to 23.0). cpu falls 0 to 10 ms on 7 maps. This is the sum of HWM1, DR1, KICK1, TX1, GV0, AU2. No single row owns it. The rows tested here as A/B states own none of it, so AU2 and the loglevel/comb_audit differences are the candidates (E).

## 2. GV1 inputs: state phasegbi against base (world, 13 scenes, n 2601)

Pooled over all world windows of state phasegbi (n 2675). ms per drawn step. The state adds 7.55 ms of timer reads (gbiops 4032 x 1.866 us), so phasegbi cpu50 is +6.36 ms over base. The report subtracts that from `gb`.

| Column | ms | Per unit |
|---|---|---|
| gb self (raw, with its timer reads) | 24.53 | 4032 ops, 6.1 us per op including 1.9 us of timer reads |
| gt (triangle commands) | 10.37 | 894 commands per step, 11.6 us per command (250 triangles drawn, 72 percent rejected) |
| gv (vertex loads) | 2.78 | 1357 vertices per step, 2.05 us per vertex |
| gm (matrix ops) | 0.80 | |
| go (other ops) | 0.00 | The column is 0 in every window. No op class is left over |
| gd (draw) | 3.17 | 99.4 draws per step |
| gp (state setup, rest) | 1.13 | gtev 0.88, gbind 1.25, gst 1.23. Sum of gp, gtev, gbind, gst = 4.5 ms = 45 us per draw |
| gc (cache) | 1.89 | |

Base world gb is 35.05 ms. The phasegbi sum gb+gt+gv+gm = 38.5 ms; minus 7.55 ms of timer reads = 30.9 ms. The 4 ms difference to 35.05 has no measured cause (E: the timer-read subtraction is too large, or base has work that phasegbi skips). So the split is correct to about 4 ms, not better.
Scenes: gt is 16 ms in 5:1, 4:8 (14.4) and 6:1 (15.9). gt is 1.4 ms in the title (2:4:20, 14:4:20) and 0.04 in 0:15. gv ranges 0.07 to 4.53.

What GV1 can remove (E): gv + gt = 13.2 ms raw. Minus the timer reads in these two classes (about 0.93 us x 894 + 90 loads = 0.9 ms) = 12.3 ms. The GV1 plan keeps the op decode, the live check and the index stores per triangle (put at 2 to 3 us per command = 2 ms) and the 16 B ring copy per vertex (0.4 us = 0.5 ms). So the ceiling for GV1a to GV1c is about 12.3 - 2.5 = 9.8 ms in the world, 6.4 ms in the intro (gv 2.56, gt 7.48), 0 to 1 ms in the title and file select. GV1b (state epoch) may take part of "gb self" (24.5 ms raw, of which about 16 ms is real after the timer reads) as well.

## 3. Overall speed (part 3 of the task)

### 3.1 Steps per second and drawn steps per scene type

From the `pace:` lines (1615 lines, 1 s each, scene type = the last `game mode` line) and the `phase W` lines (drawn steps, mean wall, share of drawn steps over 34 ms).

| Scene type | Time (s) | Game steps per s | Drawn steps per s | Drawn step wall mean (ms) | Drawn steps over 34 ms | Drawn steps with cpu over 33.3 ms |
|---|---|---|---|---|---|---|
| World (mode 4) | 1234 | 24.29 | 14.4 | 69.5 | 92.6 percent | 81.3 percent |
| Battle (8) | 112 | 24.79 | 15.8 | 63.1 | 98.5 | 90.4 |
| Intro (16) | 146 | 24.74 | 16.6 | 60.3 | 79.0 | 46.9 |
| Menu (title, file select) | 74 | 26.08 | 20.1 | 49.7 | 75.5 | 42.7 |
| Logos (1) | 10 | 29.87 | 30.7 | 32.7 | 33.1 | 0.6 |
| Map change (5) | 34 | 24.72 | | 60.0 (all transition modes) | 69.4 | 50.9 |

Whole run: mean 24.0 steps per second (build 902: 16.75, same formula over its 481 `pace:` lines, includes the boot). Build 902 world: 15.7. Hands-on world maps: 4:8 23.9 (45 percent skipped), 1:1 23.0 (55), 5:1 24.1 (45), 1:2 23.8 (50), 5:0 25.0 (31). The light scene 0:15 gives 28.4 steps per second with 0 skipped (cpu50 27.6 ms, on-time share 71 percent).
The hw_rerank estimate was 20 to 25 steps per second in world scenes. The measured 24.3 is at the top of that range. 30 steps per second is not reached in any scene type except the logos.

### 3.2 What limits the step rate now

- The FS1 skips bring the game logic up, not the picture. A world draw takes 69.5 ms (2.1 steps). The picture updates 14.4 times per second (build 902: 15.7). So the world looks a little less smooth than before and the game runs 55 percent faster.
- The sum of (drawn steps x drawn wall) = 1232 s of the 1234 s of world time. The skipped steps use about 0 ms of wall time of their own (E). So more skips are free. FS1 allows 2 skips in a row (`FS1_MAX_RUN`, pm_main.c:~330). The world needs 1.08 skips per drawn step for 30 steps per second and gets 0.69. The cause is probably the skip rule: it skips only if the last drawn step was later than 2 vblanks and the schedule credit (`step_vb += 2` per skip) has run out (E from pm_main.c:332 to 345). A draw that takes 3 vblanks gives 0.5 step of debt that the rule cannot repay. This is a code reading, not a measurement. Pace intervals support it: 46 percent of world draws arrive 3 or more vblanks after their scheduled start (intervals 3: 3656, 4+: 4537, 2: 9539).
- Where the world drawn step goes (55.2 ms cpu, pooled over 5 states, n 13812): gbi 35.05 (63 percent), audio join wait 5.58, logic 3.97, gp 3.35, gd 3.14, dl 1.99, gc 1.77, other 1.37, tex 0.92, fe 0.64. Then fb 9.91 (waiting for the GPU or the vblank) and idle. The GPU itself needs 7.6 ms (phc gpu field), so it is not the limit.
- Battle (n 416): cpu50 51.6: gbi 28.9, au 6.6, tex 3.1, gp+gd 5.2. 557 triangles drawn per step, 79 draws.
- Intro (n 1859): cpu50 37.7: gbi 28.5, au 5.9, gp+gd 8.8 (138 draws). The on-time share is 32.8 percent.
- Menu (title and file select, n 1134): cpu50 36.1: gbi 22.2, gp+gd 8.0 (129 draws, 52 triangles). Here the draw count, not the geometry, is the cost.

To reach a true 30 drawn steps per second the world needs cpu50 under about 33 ms, so -22 ms from 55. GV1 (ceiling about 10 ms) and the audio phase (5.6 ms) cannot do this alone. They reach about 40 ms.

### 3.3 Cost model: hardware over Azahar

Azahar reference: 3ds/logs/perf/hwm1_A.txt (build 907, phsum, one state `base`, 911 windows). Matched scenes only (same mode, area, map, at least 20 steps on both). HW = states base, gpuvtx, nodr1, nokick, notx1 pooled. AZ value is taken per scene and weighted by the HW steps of that scene. Ratio = HW ms / AZ ms.

| Phase | World (12 scenes, HW n 2907) HW / AZ / ratio | Battle (5 scenes, n 416) ratio | Intro (2 scenes, n 1856) ratio | Title (2 scenes, n 686) ratio | Build 902 world ratio |
|---|---|---|---|---|---|
| cpu50 | 70.2 / 18.6 / 3.79 | 3.01 | 2.30 | 2.76 | 3.14 |
| gb | 42.85 / 11.57 / 3.70 | 2.81 | 2.69 | 4.81 | 3.51 |
| gp | 3.63 / 0.97 / 3.75 | 2.92 | 3.03 | 3.46 | 3.85 |
| gd | 3.40 / 1.75 / 1.94 | 1.56 | 1.73 | 2.00 | 2.14 |
| gc | 1.79 / 0.06 / 28 | 29 | 26 | 25 | n/a |
| tx | 1.51 / 1.82 / 0.83 | 1.91 | 2.01 | 1.89 | 1.76 |
| lo | 5.96 / 1.21 / 4.92 | 4.92 | 3.53 | 3.15 | 2.88 |
| dl | 2.59 / 1.04 / 2.48 | 2.33 | 2.14 | 2.76 | 3.07 |
| fe | 0.69 / 0.32 / 2.19 | 2.11 | 2.41 | 1.56 | 2.87 |
| fb (wait) | 10.15 / 14.29 / 0.71 | 0.82 | 0.68 | 0.66 | 0.65 |

Caveats: the world sample is dominated by scene 1:1 (1674 of 2907 HW steps, cpu50 87 ms). Without 1:1 the world cpu ratio would be lower (E). The AZ file is build 907 and has no DR1, KICK1, TX1 and AU2. The `tx` ratio is 0.83 to 2.0, not 3.5: tex cost does not scale with instruction count. The `au` ratio has no meaning (AZ runs the audio on core 0).
Rule of thumb for the plan: HW gb ~ 3.5 x Azahar gb (range 2.7 to 4.8 by scene type), HW gp ~ 3.7 x, HW gd ~ 1.9 x, HW dl ~ 2.4 x, HW lo ~ 3 to 5 x, tex ~ 1 to 2 x, draws ~ 66 us each (Azahar equal share 33 us).
Use for GV1: an Azahar gb gain of 1.7 ms gives the 6 ms hardware gate (6 / 3.5).

## 4. Log health

- `lines dropped`: 0 lines (grep -c = 0, `phs_report.py --drops` = 0). `logstat` line `dropped 0` in all 132 reports.
- `logstat2` (132 lines): writer max us file 738326 (SD, one write), sock 73991, console 2111566. Console skipped 0 in all 132 lines. Queue fill max 20 of 1024. Average `logstat` frame work 39.75 ms (max 45.9 ms) per frame, 12 lines per second, 0.5 lines per frame.
- phs: 338 windows (64 base, 48 gpuvtx, 64 nodr1, 49 nokick, 53 notx1, 60 phasegbi; each has a phh and a phc line). 821 `abtog` change lines plus the start line: 136 base (with the start line) and 137 each of the other five. No gap in the order.
- pc lines: 101 (steps 300 to 400 only, `phasecsv=300:400`). So hw_seg.py has only the intro segment (101 frames) and shows 0 for demos. Its table is in hw_926_P3_seg.txt and carries nothing new. Use the `phs` lines.
- `spike` lines: 0. The word does not exist in the sources (grep of 3ds/src finds none). There is no spike line in this build.
- HUD and FPS: no log line carries the HUD value. `options: fps60=0 wide=0 hud=1` (read from options.txt at line #13, saved at #5986). The only FPS numbers are the `pace:` lines. The log says nothing about what the HUD showed.
- Statehash: 4013 `statehash` lines. f1800 5a6bb12a matches the expected value (intro not skipped). After the title the hash differs from the hands-off cycle, as expected from the hand play.
- Texture: texstat last line `461 slots 1151KB`, miss 87 in the last 300 frames, 0 texture failures, linear peak 12005 KB, VRAM free 4354 KB. 41 `vertex drops (total) 0` lines: no drops.
- Exit: the last lines are `#22528 warp: scene f30100`, `#22529 cpu limit: set 30 (HOME menu opens) result 00000000`, `#22530 boot_main returned`, `#22531 exit: audio job done, saving flushed, shutting down ndsp, citro3d, gfx, soc`. The game closed cleanly through the HW7 path. The log is not cut. No FAIL, CRASH or BOOT FAIL line. No new crash dump.

## 5. Findings that affect the plan

Which rows pay on hardware:
1. AU2 pays. Join wait 10.07 to 3.37 ms per step, audio cost per tick 7.34 to 3.07 ms. But 9.6 percent of the pcm posts find the ndsp queue empty (underrun). Needs a user listening check. If the sound has cracks, the next row is the ring depth or the render thread priority (E).
2. FS1 pays for game speed: world 15.7 to 24.3 steps per second. It costs picture rate (14.4 draws per second in the world). The skip rule leaves about 0.4 skips per drawn step unused (E, part 3.2). A policy change could reach 30 steps per second in the world at no cost in wall time. New row candidate FS1b (no user decision needed for the measurement; the picture-rate trade-off is a user decision).
3. TX1 does not pay on hardware: tex cost per MB equal with and without it, spawn steps 101 and 105 ms (goal 25 and 35). The cost is 200 to 250 ns per byte on the O3DS and it does not depend on the decode path. New investigation row: where the 200 ns/B goes (E: the write to the texture buffer, the cache flush, or the source read). Do not count TX1 in any gain total.
4. DR1 does not pay: 66 us per draw with and without it (goal 45). The draw count (98 per world step, 129 to 138 in menu and intro) is the cost. Only fewer draws help. The GV1c palette and the epoch key are the way, not the uniform shadow.
5. KICK1 does not pay: the GPU is busy 7.7 ms of 55. No overlap to gain. Keep it only if it is free (it is: delta -0.3 ms).
6. gpuvtx (partial path) does not pay: -0.39 ms in the world. Confirms the GV1 plan.

GV1 recommendation: GO for GV1a, then GV1b and GV1c with the gate after GV1c.
- The addressable cost is large and now measured: gv 2.78 + gt 10.37 = 13.2 ms raw (12.3 ms after timer reads) of 55.2 ms world cpu. Ceiling about 9.8 ms in the world (part 2), about 6.4 ms in the intro, near 0 in title and file select.
- The gate (gbi falls 6 ms or more after GV1c) needs 6 of the 12.3 ms of gv+gt, which is 49 percent. The plan expects 6 to 11.5 ms. The measured ceiling of 9.8 ms makes the gate reachable but not safe: GV1a alone (Azahar gb -1.0 to -1.6 ms x 3.5) gives 3.5 to 5.6 ms.
- Risk: the draw count. Each extra draw costs 66 us. 100 extra draws = 6.6 ms. The GV1c gate "draws equal to or lower than base" must hold on the O3DS, not only in Azahar. Measure `draw` in the phs line of the check push.
- Stop rule: if the push after GV1c shows gb gain under 6 ms in world scenes (compare with phs base, 55.2 ms cpu50 and 35.05 ms gb as reference, scene by scene), stop GV1 and move the work to the draw count (menu, intro) and to the `gb self` part.
- Reference numbers for the check push (world, hand play, base): gb 35.05, gt 10.37 (phasegbi), gv 2.78, draws 98, cpu50 55.2, 24.3 steps per second.

Other rows that follow from the data (all need no user decision):
- FS1b: raise the skip count per drawn step toward 1.08 (E). Gate: world steps per second at or above 29 in `pace:` lines on 4:8, 5:1, 1:1.
- TX2 investigate: 200 to 250 ns/B on hardware, equal in both paths.
- AU3 check: underruns 9.6 percent of posts. Read `audio pcm:` after a change of ring depth.
- The comparison of `au` between builds 902 and 926 needs care: the join wait moved into the `au` phase, so compare cpu per scene, not `au`.
- Note on the 3 ms comb_audit overhead: `noaudit` was set in this run but loglevel=debug was on. A play build (loglevel warn, no phase) is about 3.3 ms per step faster than these numbers (hw_rerank.md part 1.1). So world cpu in play is about 52 ms (E).
