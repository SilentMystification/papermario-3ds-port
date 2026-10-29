# diag_opsplit: W1 op-class timer and P1 producer census (yard, Azahar)

Source: build3ds/lane-w1/last_log.txt, lane w1, PM_ROW=w1, run 3 (budget 3/8). Shot 1 of 3 checked: Mario, textured ground, house visible. Shadow not separately confirmed.
Window: 300 frames, line `opsplit f599`. Values per frame. us = estimated time, n = exact ops.

## W1 class table (opsplit f599)
| class | us/frame | ops/frame |
|---|---|---|
| vtx | 298 | 17 |
| mtx | 265 | 59 |
| tri | 261 | 48 |
| tex | 5963 | 1084 |
| cc | 2674 | 608 |
| geo | 1083 | 419 |
| dl | 1012 | 343 |
| dlc | 1043 | 73 |
| sync | 2237 | 819 |
| oth | 1606 | 104 |
| sum | 16442 | 3576 |

Top ops by us: f5 2286, e4 1442, f0 1395, fc 1253, e7 1192, ff (dlc) 1043.
taskpl window (frames 300 to 599, 300 lines): ops 3575.6, bg 2364 us, main 9841 us, bg+main 12205 us.

## Result
- Ops: opsplit 3576 against taskpl 3575.6. Pass (within 1%).
- Class sum: 16442 against 12205 us. Fail (+35%, gate 15%).
- Cause [I]: each sample reads svcGetSystemTick, and the read cost (about 1.3 us in Azahar) lies inside the sampled window. Scaled by the op count, this adds a constant per op. The raw estimate is therefore an upper bound. Direction across rows holds.
- Fix not done: subtract a measured per-window constant, or sample an empty window once per report.

## P1 producer census (prod f599)
Sum of producers = 3576 = opsplit ops. Pass (exact).
| producer | ops/frame | us/frame (est.) |
|---|---|---|
| 0015fee8 (fn key) | 1206 | 4575 |
| 00163914 (fn key) | 837 | 3591 |
| WorkerBackUI | 549 | 2874 |
| bg | 358 | 2807 |
| 00214320 (fn key) | 300 | 1237 |
| 00211608 (fn key) | 172 | 699 |
| sprite_component | 104 | 509 |
| camera_render | 24 | 80 |

Two producers are above 800 ops per frame: 0015fee8 and 00163914. Name them with the 3dsx .map file before C2.
us for producers scale with all ops, including nested replay ops, so treat them as rough.

## Notes
- Code: pm_gbi.c (os_* and pk_* block before pm_gbi_end_frame, wrapper pm_gbi_run, hook before the dispatch switch), pm_interp.c (pm_interp_open calls pm_gbi_prod_mark under phase).
- Nested dlc replay runs count exact ops into os_nn, not into class terms.
- The table is Azahar only. Console numbers come from the final build's opsplit and prod lines.
