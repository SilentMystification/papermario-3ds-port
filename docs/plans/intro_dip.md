# Intro dip: the step time doubles when the back of Bowser appears

Written 2026-10-20 (row DIP). Read-only study of existing logs. No Azahar start was used. No source was edited.
All ms are Azahar guest ms (emulated ARM11 clock). Azahar has no cache model and no PICA timing. The O3DS is slower (docs/plans/PO3.md:16, EST).
Data: 3ds/logs/perf/full/cur_normal.txt (build 840, cpulimit 30), 3ds/logs/perf/au1/t_main80b.txt (build 855), 3ds/logs/perf/po3/ga_noaudit.txt (859), po3/gp4.txt (859, gbiprof), po3/pg0m.txt (862, gbiprof), po3/pg1_on.txt (862, PG1 on, HEAD code), po3/pg2_1_nopg2.txt (866).
The `pc N` number is the frame number. The intro is pc 0 to 3936. No hardware log exists (build3ds/hw_live_log_*.txt are empty).

## 1. Where it happens

Step pc 1702 (30 fps time 56.7 s). Step pc 1701 is a quiet scene. Step pc 1702 is the first step with the Bowser model.
Before (pc 1614 to 1701), build 840, same in all builds: 60 input triangles, 53 drawn (triout), 84 vertex loads, 135 draws, 1231 gbi ops, gbi 7.3 ms.
From pc 1702 (same in every build): 1808 input triangles, 290 drawn, 3168 vertex loads, 1704 to 1764 vertex slots, 185 to 195 draws, 5679 gbi ops, gbi 29.9 ms (build 840).
So 84% of the 1808 triangles are rejected by the clip test (Bowser is mostly below the screen). The CPU still reads and tests every one.
The new load also has many material changes per frame (pg0m, window pc 1800 to 2099, against window pc 1200 to 1499): G_SETCOMBINE 207 against 13, sync_combine calls 616 (381 with equal inputs), G_SETOTHERMODE_L (e3) 266, G_SETTILE 520 against 261, G_SETTILESIZE 225 against 127, G_VTX 191, G_TRI2 899. G_TEXRECT is equal: 148 against 156 per frame.

## 2. What grows (ms per step, all numbers from the phase and pc lines)

| Build, cpulimit | step before | step during | gbi before | gbi during | other phases that change |
|---|---|---|---|---|---|
| 840, 30 (cur_normal.txt) | 33.4 | 66.9 for pc 1702 to 1808 (15 steps/s), then 50.1 | 7.3 | 29.9 | audio join 8.4 to 18.1, tex 0.0 to 2.7 (first 50 steps, texmiss max 12, texKB max 95), dl 0.3 to 1.0, gdraw 2.9 to 3.5, fbegin 12.3 to 9.0 |
| 855, 80 (t_main80b.txt) | 33.4 | 50.1 | 8.3 | 33.0 | audio join 0.0, tex 4.9 in the first 50 steps |
| 859 noaudit, 80 (ga_noaudit.txt) | 33.4 | 33.3 | 5.3 | 18.3 | fbegin 16.6 to 7.0 |
| 862 PG1 on, 80 (pg1_on.txt, HEAD code) | 33.3 | 33.3, 40 of 100 steps over 34 ms in pc 1700 to 1799 (texture misses), then 6, 6, 1 | 4.8 | 18.7 | fbegin 16.6 to 7.0, tex 4.9 in the first 50 steps |
| 866 nopg2 + gbiprof | 33.3 | 66.8 | 7.5 | 44.5 | gbiprof ticks inflate gbi (1.336 us per read) |

Result: the phase that grows is gbi, by +22.6 ms in build 840 and by +13.9 ms in the HEAD code. All other phases grow by under 1 ms in steady state (dl +0.7, gdraw +0.6 to +1.1, gpu +0.2, rom +0.2 to +0.4, logic +0.1). fb (fbegin wait) falls by the same amount, because it is the idle part of the 2 vblank step.
The 15 fps step (66.7 ms) in cur_normal.txt needs more than gbi. It has cpulimit 30: the audio thread takes the game core, audio cost is 4.7 ms per 60 Hz tick (474 us before pc 330), and 4 ticks fall in a 66 ms step (audio join 18.1 ms). Audio is a result of the longer step and of cpulimit 30. It adds about 10 ms. From pc 1809 the audio cost falls to 2.4 ms per tick (the music changes), the cpu time is 45 ms and the step is 50.1 ms (3 vblanks, 20 fps).
Pace line, pc 1702 to 1730: `steps 15 ... intervals 4+:15` (build 840). With cpulimit 80: `intervals 3:30` per second.
Does the dip show in Azahar? Yes in builds 840 to 859 and 866 with gbiprof (steps of 50.1 or 66.7 ms). The HEAD code (build 862, pg1_on.txt) does not dip in steady state: the gbi time is 18.7 ms, the cpu time is about 26 ms and fbegin has 7 ms slack. The Azahar pacing (2 vblanks) turns every overrun of 33.3 ms into 50 ms or 66.7 ms: a small overrun costs a 50 percent step.

## 3. Why gbi grows by 13.9 ms in the HEAD code (E: gbiprof raw minus tick reads)

Raw gbiprof of pg0m window pc 1800 to 2099 minus 1.336 us per tick read (about 9600 laps and 5700 op reads per step). Sum of the table: 13.9 ms. It matches the measured +13.9 ms.

| Item | Real ms per step | Basis |
|---|---|---|
| draw_tri, all sections | 6.8 | A 2.8 (with the debug audit), ens 0.1, dual 0.3, early 0.6, kept triangles (old) 1.5, rest |
| of it, the 1518 rejected triangles up to the reject | about 3.4 | 2.3 us per triangle (A, ens, dual, early minus laps, divided by 1808) |
| G_VTX (191 per step, 3168 vertices) | 2.3 | op 01: 2.6 raw minus 0.26 |
| G_SETCOMBINE and sync_combine (616 calls, 62% equal inputs) | 2.2 | op fc: 2.5 raw minus 0.28. Was under 0.2 |
| G_SETOTHERMODE_L (266) | 1.1 | op e3 |
| G_SETTILE (520) | 0.9 | op f5 11 raw minus 0.7 |
| G_SETTILESIZE (225) | +0.5 | op f2 3.4 raw against 2.8 |
| draws +59 (1.5 triangles per draw) | about +1.4 | gpu + gdraw 4.6 against 3.4, 24.1 us per draw (post_po3_path.md, row 20) |

Triangle draw count: 290 drawn triangles in about 50 new draws. The model uses one material change per few triangles.

## 4. Hypotheses, ranked

| Rank | Hypothesis | Verdict | Number |
|---|---|---|---|
| 1 | e, f: CPU gbi path, many ops, many rejected triangles, many state changes | Cause | gbi +13.9 ms (HEAD) or +22.6 ms (840). gbiops 1231 to 5679, tri 60 to 1808, vtxload 84 to 3168 |
| 2 | g: audio | Amplifier at cpulimit 30 only | audio join 8.4 to 18.1 ms, audio cost 4.7 ms per tick. 0.0 at cpulimit 80 |
| 3 | c: texture miss | Only the first 50 steps | tex 4.9 ms avg in pc 1700 to 1749, texmiss max 12, texKB max 95, then 0.0 to 0.3. This causes 40 late steps in HEAD |
| 4 | a: fill rate | Not measurable in Azahar. No sign in the counters | Only 290 triangles drawn. Texrects are equal (148 against 156). Large blended quads: not counted. Needs counter 2 of post_po3_path.md section 3 (GPU busy time) on the O3DS |
| 5 | d: ROM and Yay0 | No | rom 0.3 to 0.5 ms, yay0 0.0 |
| 6 | h: logic | No | logic 0.2 to 0.3 ms |
| 7 | b: grabs, readback | No | b7 line: grabs 0, splits 0, swirl capture 0, F03 query 0 |
| 8 | e: slow path (lit, texgen, mixed matrix, screen vertex) | No | po3a: 0.1% lit, 0% texgen, 0 screen vertices over the cycle. The CPU path is the normal path, no special mode |

## 5. Why the user sees about 15 fps

HUD about 15 fps = steps of 66.7 ms. In Azahar that needs cpu time over 50 ms: it happens only with cpulimit 30 (audio on the game core) or on the O3DS. The O3DS is slower than Azahar and has caches. The HEAD code needs 26 ms of cpu time in Azahar. A factor of 1.27 or more on the O3DS gives a step over 33.3 ms. A factor of 1.9 or more gives 66.7 ms. Both are plausible for gbi. If the report came from an older build or cpulimit 30, the 66.7 ms step is already in cur_normal.txt.
Not measured: the O3DS time of this scene. No hardware log of pc 1702 exists. The PICA time is not known (no counter).

## 6. Cheapest fixes (EST, Azahar ms)

No single change removes the 13.9 ms. The cost is the number of ops, and about 200 material changes per step in one model. The fixes below add up to about 6 to 8 ms. All are in the rows O3, O5, O2 of post_po3_path.md section 6, now with a scene that has a count.
1. O3 and the early reject first (pm_gbi.c:1147-1222): keep off_screen_check and box_fold_sync before the reject (as O3 says), move the stat block (pm_gbi.c:1174-1185), ensure_tile and dual_bind after the reject, and compute the clip bits once per vertex at G_VTX (pm_gbi.c:1874-1939) so that a rejected triangle costs one AND. Gain about 1.5 to 2.5 ms (the 1518 rejected triangles, 3.4 ms now). Counters and statehash stay equal if n_out, st_tri and the stat block counts stay the same.
2. O5 sync_combine compare (pm_gbi.c:935-952, callers 2022, 2043-2127): 381 of 616 calls have equal inputs. Gain about 1.0 to 1.5 ms. PG1 (278c5d7be) did the tile part only. The log of pg0m (before PG1) shows 616 calls. Check with `gbiprof same` that O5 is not done.
3. G_SETOTHERMODE_L and G_SETTILE: skip the flush and sync_render when the new word equals the old one (op e3 1.1 ms, f5 0.9 ms). Gain about 1.0 ms. `gbiprof same` shows tile0 169 of 224 equal after PG1.
4. Texture misses in pc 1700 to 1749 (4.9 ms avg, 95 KB peak): prefetch the Bowser textures at the load of the scene. This removes the 40 late steps. Needs the O9 prefetch (post_po3_path.md) and is bigger work.
5. Batch the draws (O2): 1.5 triangles per draw in the Bowser part, 24.1 us per draw.

Cannot measure: (a) fill rate, (b) the O3DS cost of this scene, (c) the share of the new 59 draws that is blend. A hardware run with the GPU busy counter (post_po3_path.md section 3, item 2) and `phase` on pc 1650 to 1850 answers (a) and (b). That run is part of row B7 (O3DS measure) and needs the user.
