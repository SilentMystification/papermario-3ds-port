# P4 findings: build 970, O3DS, hand-played session (hw_pull4)

Source: 3ds/logs/perf/hw_pull4/hw_o3ds_log.txt (5668 lines, 269 s of play, build 970, main 0dca531db).
Comparison: 3ds/logs/perf/hw_pull3/hw_o3ds_log.txt (build 926), 3ds/logs/perf/hw_pull2 (build 902, no phs data).
Read-only analysis. No source edit. Units: ms per drawn frame unless said otherwise. "Step" is one game logic step (30 Hz). "Drawn" is a step that was not skipped by FS1.

## 1. What the user played

- The run starts with a warp (line 118): `warp: mac_00 entry 0 (area 1 map 1)`. No title, no file select, no intro scene. Game mode sequence: 0 (boot), 7 (enter world), then only 4 (world) and 5 (map change).
- Maps: area 1 map 1 (mac_00, Toad Town) steps 51 to 939, then area 1 map 2 (mac_01) steps 978 to 2278, then map 1 again steps 2317 to 6682, then map 2 steps 6723 (about) to 8066. 3 map loads (lines 811, 1749 and the load at step 6682, which has no `load map` line, only phs 5 windows at lines 4728 and 4743).
- No battle. No game mode change for the pause menu or any other menu. No save line. No `route` line.
- From step 3200 to 6200 (about 100 s) the position is constant: pos -221 0 127, actionState 0, wall 15. The user stood still (or had a menu open that does not change the game mode). This is 37 percent of the run.
- The run ends with `cpu limit: set 30 (HOME menu opens)`, `boot_main returned` and `exit: audio job done, saving flushed, shutting down ndsp, citro3d, gfx, soc`. Clean exit.

Scene types in the log: town walking (area 1 map 1 and map 2, 3 windows of each, 255 s), map load (3 loads, about 1.3 s each: steps 939 to 978, 2278 to 2317, 6682 to about 6723), boot and enter world (steps 0 to 51). Battle: none. Menu: none that the log shows.

## 2. Frame rate

pace lines (265, one per second) and the fs1 lines (`fs1 skipped N of M steps`, every 300 steps).

| Scene (log lines) | seconds | steps/s | drawn/s | Notes |
|---|---|---|---|---|
| Town 1:1, first visit (166 to 803) | 29 | 29.83 | 14.6 | FS1 skip 51 percent |
| Town 1:2 first visit (853 to 1737) | 43 | 29.94 | 13.1 | |
| Town 1:1 second visit (1781 to 4696) | 146 | 29.91 | 11.8 | includes the 100 s standing still at the heavy spot |
| Town 1:2 second visit (4751 to 5665) | 45 | 29.90 | 13.9 | |
| Map loads (3) | about 4 each | 25.6 to 27.8 | 14.7 | |
| All the run | 269 | 29.95 | 12.8 | FS1 skipped 4481 of 7800 steps (57 percent) |

- The `pace:` field `drawn` counts loop passes, skipped passes included. Drawn per second is the sum of the interval counters (1:, 2:, 3:, 4+:) of the same line, or steps minus skipped. The numbers above use that sum.
- Build 926 (user figure): 24.3 steps/s, 14.4 drawn/s. Build 970: 29.9 steps/s (game speed is now correct), drawn 11.8 to 14.6 (no gain, 12.8 mean).
- The HUD counter is `presents`, which is incremented once per loop pass, also for a skipped pass: 3ds/src/pm_main.c:410 (and :342 for key frames). The HUD value is therefore the step rate (about 30), not the drawn rate. The log field `late steps (3+, over 33.3 ms)` (10 of 3455 in the whole run) is also blind: a skip adds 2 vblanks to the schedule, so every drawn interval is logged as "2" (pm_main.c:372 to 380, iv counters).
- Steps over 33.3 ms: all drawn frames are over 33.3 ms. phs ok share (steps with wall up to 34.5 ms) is 0 percent in every town window. Drawn frame wall: 66.7 ms (4 vblanks) when the CPU part is up to about 60 ms, 83.3 ms (5 vblanks) and 100 ms (6 vblanks) above that. Share of spike lines at 80 to 100 ms in town: 41 percent (1:1 second visit), 27 percent (1:2 first visit), 11 percent (1:2 second visit), 1 percent (1:1 first visit).

Phase split, town (phs windows, mode 4, ms per drawn frame; gb = gbi, gp = gpu submit, gc = gcache, gd = gdraw, fb = FrameBegin wait, fe = FrameEnd, ot = other; dl = display list walk):

| Window | n | lo | dl | gb | gp | gc | gd | tx | au | fb | fe | ot | sum | cpu without fb |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 970 1:1 (all) | 2160 | 8.41 | 3.24 | 40.38 | 4.50 | 1.64 | 3.79 | 0.18 | 0.82 | 10.99 | 1.20 | 2.68 | 77.9 | 66.9 |
| 970 1:2 (all) | 1169 | 7.95 | 3.25 | 38.80 | 3.95 | 1.61 | 3.28 | 0.23 | 0.63 | 8.96 | 1.16 | 2.66 | 72.5 | 63.6 |
| 970 1:1 steps 1689 to 2588 (standing at the wall) | 900 | 10.3 to 11.1 | | 43.1 to 44.5 | | | | | | 13.6 to 16.4 | | | 86 to 92 | |
| 926 1:2 base | 240 | 7.91 | 3.09 | 39.94 | 3.10 | 1.92 | 2.92 | 1.93 | 6.68 | 9.97 | 0.56 | 1.49 | 79.6 | 69.7 |
| 926 1:1 base (other spot, 576 tri) | 308 | 7.57 | 3.35 | 51.35 | 4.55 | 1.82 | 4.23 | 0.69 | 10.51 | 9.64 | 0.63 | 1.67 | 96.1 | 86.5 |

- Dominant cost in town: gbi, 38.8 to 40.4 ms (58 to 60 percent of the CPU part; 44 ms at the heavy spot). Next: logic 8.0 to 8.4 ms (11 ms at the heavy spot), then gpu submit gp+gd+gc 9.0 to 9.9 ms, then dl 3.2, other 2.7, fe 1.2, audio 0.6 to 0.8.
- GPU time (citro3d) is 7.6 to 8.1 ms per drawn frame (phs gpuUs). The GPU is not the limit. fb 9 to 16 ms is the wait for the vblank that follows the CPU part, not work.
- The gbi cost does not follow the visible geometry. Counters per drawn frame (phc): 1381 to 1422 triangles read, 2248 to 2362 vertex loads, 6650 to 7290 gbi ops, but only 240 to 570 triangles output. gv1 rejects 927 to 962 triangles per frame (68 percent) early (pm_gbi.c:1286). The window range 35.1 ms (77 draws) to 44.5 ms (124 draws) gives about 0.2 ms per draw on top of about 20 ms for the fixed 1420 triangles, 2350 vertices and 7000 ops.
- Scene types other than town: the 3 map loads and the boot are described in section 4. A battle and a menu were not played, so there is no number.

What changed with GV1, RAW1, FL1 and AU2 on the console (build 926 to build 970, ms per drawn frame):

- Audio join (au): 6.68 (926, 1:2) and 10.51 (926, 1:1) down to 0.63 and 0.82. Gain 6.0 and 9.7 ms. This is the largest measured gain and comes from the dspaudio path, not from GV1.
- tex (tx): 1.93 down to 0.23 in 1:2 (gain 1.7 ms). Texture cache has 0 evictions in the whole run (texstat: miss 20 to 389 per 300 steps, uploads 80 KB to 2167 KB per 300 steps, only after a map load or a new area of the map).
- gbi per op: 926 1:2 base 39.94 ms for 6255 ops = 6.4 us per op, 970 1:2 38.80 ms for 6649 ops = 5.8 us per op (minus 9 percent). Per triangle read: 37.0 us down to 28.1 us. The 970 frame has 28 percent more triangles and 27 percent more vertex loads than the 926 frame, so the same ms hides about 22 percent more geometry work.
- 926 itself showed no gv1 gain on the console: 1:1 gpuvtx gb 51.82 against base 51.35, 1:2 gpuvtx 41.02 against base 39.94 (3ds/logs/perf/hw_pull3). The gain in 970 is per op only, not per frame.
- gp+gd per draw: 970 1:1 79 us (4.50+3.79 ms over 105 draws), 970 1:2 79 us (7.23 ms over 92 draws), 926 1:2 68 us (6.02 ms over 88 draws), 926 1:1 69 us. The goal of DR1 was under 45 us. In 970 the cost per draw is not lower than in 926 (9 to 16 percent higher in the same map 1:2).
- New counters per drawn frame, 1:1 and 1:2: flushcalls 6 and 6, flushkb 60 and 56, rawd 49 and 45, rawcall 141 and 128 (the phase count line prints these four as `(null)`: `cname` has 22 names and C_N is 26, 3ds/src/pm_phase.c:21 and 3ds/include/pm_port.h:62). The raw call count (128 to 141) is higher than the draw count (92 to 105): about 1.4 raw calls per draw. Flush cost: 6 calls, 56 to 60 KB per frame, inside gp (3.95 to 4.50 ms).
- Logic 7.95 (1:2) is equal to 926 (7.91). ot 2.7 against 1.5 (plus 1.2 ms in 970; the skipped step work is probably counted here).

Next biggest cost per scene type: town: gbi dispatch (about 28 ms of the 43 ms gb in the 926 phasegbi run is outside gv, gt, gm, gtev, gbind and gst: gv 2.5 to 3.9, gt 7.1 to 10.8, gm 1.4 to 2.0, gtev 0.8 to 1.2, gbind 1.2 to 1.7, gst 1.2 to 1.6), then logic, then gp+gd. This split is from build 926 only. The build 970 run had no `phasegbi` state, so the split is not measured on 970.

To reach drawn rates (CPU part, frames lock to whole vblanks): under 50 ms for 20 per second, under 33 ms for 30 per second. Today the CPU part is 57.9 to 67 ms (66.9 mean in 1:1, 63.6 in 1:2).

## 3. Audio (dspaudio)

All numbers from the 23 reports of `audio dsp:` (one per 300 steps, about 10 to 13 s each; total 269 s = 4.5 min).

| Item | Value | Per minute |
|---|---|---|
| Blocks produced | 44968 (block = 184 samples at 32 kHz, 5.75 ms) | 9990 |
| ndsp frames | 53204 (ratio blocks to frames 0.850 in every report, as expected from 160 against 188 samples) | |
| ndsp frames dropped | 0 in all 23 reports | 0 |
| Cache failures, evictions | 0 and 0 (peak 1071 KB of 1536 KB) | |
| Cache misses and decodes | 24 total, all early in a map or at a new sample, avg 5.1 to 25.6 ms, max 61 ms | 5.3 |
| Cache hits (voice starts) | 5100 starts | 1133 |
| Voice stops and ends | 2810 stops, 891 ends | 624 and 198 |
| Voices | avg 2.8 to 10.5, max 17 (never above 17 of 23) | |
| Block cost avg | 83 to 174 us; max 61152 us (decode included); 7 of 23 reports have a max above 5 ms | |
| Backlog (blocks queued) | avg 6.0 to 8.0, max 12 (no trend to zero, no growth) | |
| Late resyncs | 20 (3, 0, 0, 1, 3, 0, 0, 1, 0, 0, 0, 0, 2, 2, 0, 3, 1, 2, 1, 1, 0, 0, 0) | 4.4 |
| ndsp param writes | 1958 to 7483 per report, avg 4.1 to 7.5 us each | |
| Aux callbacks | 0 on bus 0, 0 on bus 1, core 0, effect frames queued 0, dropped 0, in all 23 reports | |
| audio cost (60 Hz tick) | avg 250 us over 134 lines, max 44150 us, 41 lines with a max above 3 ms | |
| audio wait (core 0 join per step) | avg 241 to 724 us, max 1029 to 29155 us | |
| audio2 render busy | avg 7.6 to 8.3 ms per job, max 17.9 to 83.6 ms | |
| audio2 ring full waits | 257 waits, 1.246 s in total (68 and 0.52 s in the first report) | 0.9 s per run after the first report: 0.18 s per minute |
| Core probe (core 1 share) | avg over min 1.08 to 1.28 (min 2855 to 3058 us, avg 3347 to 3649 us) | |

Warnings, errors: none from the audio code (no `audio dsp` error or `pm_log_e` line, no `wave buffer` line, failures 0).

Signs, in the order of weight:

1. The aux callbacks never run on the console: `bus0 0 bus1 0 core 0` in all 23 reports, while the effect state is active (`audio vfx` shows bus 1 type 6 with 3 sections on every tick, line 117 `dspaudio: bus 1 effect type 6`). The effect return channel 23 gets no frames. `ndspAuxSetEnable(0/1, true)` and `ndspAuxSetCallback` are called (3ds/src/pm_dspaud.c:394 to 397), so the callback is registered but ndsp does not call it. The same zero is in 3ds/logs/perf/hwb/play.txt (the earlier run). All wet and reverb content is missing. This is not an underrun symptom. It can change the level balance if the dry send was scaled for the return.
2. No starvation and no growth: backlog avg 6.0 to 8.0 blocks (35 to 46 ms), max 12, 0 dropped ndsp frames. The mixer keeps up (block cost avg 83 to 174 us against 5750 us per block).
3. The 20 late resyncs (`now > next_due + 6 blocks`, pm_dspaud.c:416) are not tied to decode: 16 of the 20 are in the 16 reports with 0 decodes and a max block cost under 5 ms (reports at lines 1318, 1970, 3099, 3340, 3876, 4123, 4370, 4602, 4819). Likely cause: game-thread stalls (join wait max 28 to 29 ms in some reports, drawn frames of 80 to 100 ms). This link is not proven by a log line. Each resync is a step in the wave time line (a click or a jump).
4. Decode on the audio path: 24 decodes, longest 61 ms (line 336, first map). 61 ms is above the backlog (35 to 46 ms), so the first map start can run dry once. Later decodes are 8 to 35 ms and below the backlog.
5. Voice churn: 19 voice starts per second and 10 stops per second (5100 starts and 2810 stops in 269 s). Every start does `ndspChnWaveBufClear`, then format, interpolation, rate, mix and the wave buffers (pm_dspaud.c:174 to 197). A stop is a clear without a fade (pm_dspaud.c:210 to 214). Hard cuts at a non-zero sample are audible as clicks.
6. Loop with a finite count is cut at 4 repeats: `if (reps > 4) reps = 4;` (pm_dspaud.c:190). A sample with `loopCount` above 3 plays short.
7. No clipping control: no `ndspSetClippingMode` call in 3ds/src. Voice count 9 to 17 for 20 percent of the ticks (`ticks with 9-12`, `13-16`: about 1000 to 1900 of 2300 ticks per report in the long town windows have 9 or more voices). The sum of voices can pass full scale in the ndsp mixer.
8. Pitch is not the cause: `pitch>2 0` in every vstat line.

The log shows no number that explains "highly distorted, beats audible". Dry mixing, clipping and the VADPCM decode are not visible in the log. An A/B on the same push (dspaudio off, same scene) and a PC decode check of one sample against the N64 reference would separate a decode fault from a mix fault. Luma dumps were not decoded.

## 4. Spikes (steps over 50 ms, at most 10 lines per second)

Counts (lines per scene): intro and load 7, town 1:1 first visit 292, load 8, town 1:2 first visit 423, load 14, town 1:1 second visit 1397, load 19, town 1:2 second visit 433. Total 2275 plus 289 and 24 variant lines (the same `spike f` line with different text). The log cap (10 per second) hides the real count, but 2275 lines in 269 s is 8.5 per second: nearly every drawn frame in town is a spike (drawn rate 12.8 per second).

- Town spikes: the dominant phase is gbi in all 2545 town lines (gbi 34 to 47 ms, with fbegin 4 to 16.7, logic 4 to 13, gp 3 to 8, gd 3 to 7). 80 to 100 ms frames carry `fbegin 16.7` (one extra full vblank wait) and `logic` 11 to 13 ms (against 6 ms in the 66 ms frames) and `ticks 2 or 3`.
- Spikes of 100 ms and more: 9 lines. 6 are map load pairs: a first line of 187 to 217 ms (logic 70 to 88 ms, rom 55 to 70 ms for 7204 to 9168 reads and 700 KB, yay0 12 ms) at lines 133, 821, 1760, 4731 and a second line 6 to 15 steps later of 117 to 124 ms (gbi 54 to 60, tex 23 to 33, 79 to 111 texture misses, audio up to 10.5 ms) at lines 151, 833, 1775, 4735. Line 5444: 100.4 ms (gbi 60.8, gp 7.8, gd 6.8; no load, no texture miss, no rom).
- Texture evictions: 0 in the whole run, so no spike comes from a texstat eviction. 24 percent of town spike lines have texmiss above 0 (301 of 1397, 183 of 423, 97 of 292, 93 of 433) with 0 to 1 KB: not the cause.
- Saves: none in this run. flushcalls: constant 6 per frame, no correlation.
- Battle start: none.
- Audio join: lines with `audio 6 to 10 ms` are 28 of the 80 ms-and-over town spikes in 1:1 second visit (`ticks 2 to 3`).

## 5. Log health

- Dropped lines: 0 (`logstat frames 300 ... dropped 0` in all 26 reports; `phs_report.py --drops` gives 0). logstat2: writer max file write 375742 us (line 326, first report), later 95 to 370 ms; sock max 61 to 373 us; console max 392 to 1709 ms (one-time); `console skipped 0` in all reports; queue max 8 of 1024. About 21 to 33 lines per second, 30 KB per 300 steps.
- Exit: clean (`cpu limit: set 30 (HOME menu opens)`, `boot_main returned`, `exit: audio job done, saving flushed, shutting down ndsp, citro3d, gfx, soc`).
- grep -i 'fail|crash|error|warn|fatal|assert|unmapped': 29 matches, all counters: `audio dsp: cache ... failures 0` (23), `texture failures 0` (6). No FAIL, CRASH, error, warning or unmapped line.
- Luma crash dumps: not read.
- Cosmetic faults in the log: the phase count line prints `(null)` for four counters (see section 2). The `pace:` `drawn` field counts skipped passes.

## 6. Findings by user impact and cheapest next fixes

1. Drawn rate 11.8 to 14.6 per second. The CPU part per drawn frame is 57.9 to 67 ms (mean 66.9 in 1:1, 63.6 in 1:2) and it sits above the 50 ms limit for 20 fps and the 66.7 ms limit of 4 vblanks. The cost is gbi (58 to 60 percent), which reads all 1380 to 1420 triangles and 2250 to 2360 vertices of the scene each frame while 68 percent of the triangles are rejected and only 240 to 570 are output. Cheapest next step: measure the gbi split on 970 with `abtog=base,phasegbi:30` (the 926 split leaves about 28 ms of gb unexplained by gv, gt, gm, gtev, gbind and gst). Then cut the work done per rejected triangle (pm_gbi.c:1286 and the vertex load at pm_gbi.c:2196: reject a whole vertex-load group or display list segment by a bounding test before transforming vertices). A 12 to 17 ms gain moves town from about 13 to about 20 drawn per second.
2. HUD shows 30 (wrong). Fix: `presents++` at 3ds/src/pm_main.c:410 runs for skipped passes. Move it into the draw branch (after `pm_ph_frame_end();` near line 405) and keep a second counter for steps. A change of 2 lines. Also drop the blind `late steps (3+...)` field or count skipped passes as late (pm_main.c:372 to 380).
3. Audio: the aux callbacks do not run (n 0 for 270 s). Check on the console whether `ndspAuxSetEnable` needs to come after `ndspInit` and the channel mix `mix[4..11]` aux gains (pm_dspaud.c:380 to 397, voice mix at :183 and :202). Run a 60 s A/B with dspaudio off in the same scene to decide whether the distortion is the new path. Add `ndspSetClippingMode` (soft) as a test. Remove the `reps = 4` limit (pm_dspaud.c:190) or log how often `loopCount` is above 3.
4. 20 late resyncs (4.4 per minute): they follow stalls of the game thread (join waits up to 29 ms, 80 to 100 ms frames). The slow frames (point 1) are the cause. A 6-block (34 ms) tolerance at pm_dspaud.c:416 is below the 83 to 100 ms frame time; raise it to 20 blocks or fill silence for the gap.
5. Map load stall 187 to 217 ms (logic 70 to 88 ms plus 55 to 70 ms rom plus 12 ms yay0) and the first frame after it (117 to 124 ms, 23 to 33 ms tex). 3 times in the run. Low priority.
6. Fix the `(null)` counter names: add `flcall`, `flkb`, `rawd`, `rawcall` to `cname` in 3ds/src/pm_phase.c:21. One line.
7. DR1 per draw cost is 79 us (goal 45). gp+gd+gc 9.0 to 9.9 ms for 92 to 105 draws and 128 to 141 raw calls (1.4 per draw). Merge the raw calls (rawcall above draw count).
8. Standing at the wall (steps 3200 to 6200, spot -221 0 127) is the heaviest spot: 86 to 92 ms per drawn frame (11 fps), 124 draws, 570 triangles out, logic 11 ms. Use this spot as the benchmark (route or warp).

## 7. Surprises

- FS1 skips 57 percent of the passes. The game logic is at full speed (29.95 steps/s) but only 12.8 frames per second are drawn, and the HUD and the pace line hide it.
- The aux callbacks of dspaudio never ran on the console. The effect bus 1 is configured (type 6) and never mixed.
- The audio path is healthy by every counter (0 dropped ndsp frames, backlog stable at 6 to 8 blocks). The distortion is not a scheduling fault of the log numbers.
- Audio join and tex gains are large (6 to 10 ms and 1.7 ms) but the frame time did not change in map 1:2 (63.2 ms in 926, 62.7 ms in 970 at the p50), because gbi carries 28 percent more geometry in the 970 sample.
