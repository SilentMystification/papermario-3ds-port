# Yard walk W split, O3DS build 1175 (diagnosis, no source edits)
Source: build3ds/hw_pull/hw_o3ds_log.txt, 2718 yard frames (game mode 4, ops>0). Script: python 3ds/tools/console_walk_split.py LOG.
All values ms per frame, averaged. [M] measured, [I] inference.

## Field meaning (3ds/src/pm_gbi.c, pm_gpu.c, pm_phase.c)
- gbisplit tex = time from the first of SETTIMG/SETTILE/SETTILESIZE/LOADTILE/LOADBLOCK/LOADTLUT/TEXTURE to the next op outside that set (tex_streak_op, line 55). It contains everything nested in that span: the fl tex closes (flush at tex ops), texture cache lookup and upload if they run there. [M code]
- gbisplit gv = time from the first G_TRI1/TRI2 to the next non-tri op (gv_streak_op, line 42). It is the triangle streak, not G_VTX. [M code]
- gbisplit sync = sum of sync_combine_work_timed (line 480): combiner and othermode resolve. Called from sync_combine at draw time. It is not exclusive to gv or tex streaks. [M code]
- flsplit = pm_gpu_flush_at time by class (pm_gpu.c 2099). Each flush is attributed to a class by pm_gbi_flush_class(). Nested in W. [M code]
- thsplit = texture hit/miss time. Nesting in the tex streak is not proven. [I]
- dlcsplit lines are absent in build 1175. Only rsplit dlc 3.3 per frame exists (dlc replay count), no time. [M]

## Yard numbers
| class | ms | field |
|---|---|---|
| W total | 34.3 | phase W / console_numbers.md |
| texture ops streak (includes fl tex and maybe th) | 17.97 | gbisplit tex |
| of it: fl tex closes, 281.9 closes, 207.7 empty | 8.39 | flsplit tex us |
| of it (maybe): texture cache hit 86.1 x 16.7 us | 1.43 | thsplit hit |
| of it (maybe): texture cache miss 1.6 x 1.0 ms | 1.63 | thsplit miss |
| tex streak remainder, own work of the tex ops | 6.6 to 8.2 | tex minus fl tex, minus th if nested [I] |
| triangle ops streak | 2.55 | gbisplit gv |
| combiner/othermode resolve | 2.99 | gbisplit sync (overlaps tex or gv, unknown) |
| fl cc 7.6 closes / rect 46.2 / tri 13.3 / oth 28.2 / geo 0 | 0.027 / 0.261 / 0.102 / 0.065 / 0 | flsplit us |
| dlc replay | not timed | rsplit dlc = 3.3 per frame |
| vertex ops (G_VTX, matrix) | no counter | missing |
| state/geometry ops (SETGEOMETRYMODE, MTX, POPMTX, DL jumps) | no counter | missing |
| unexplained in W after tex 17.97 + gv 2.55 | 13.8 | W minus those two [M arithmetic] |
| frame "other" (outside W, in wall) | 7.4 | phase other |

Sum check [M]: tex + gv = 20.5 ms is 60 % of W. The remaining 13.8 ms covers the other ~4000+ ops: vertex, matrix, geometry mode, combiner set ops (SETCOMBINE, SETOTHERMODE, SETPRIMCOLOR, SETENVCOLOR), DL branch, the dispatch loop itself, and any sync not inside the two streaks.
Counters per frame [M]: combsplit plan 49.2, mode 25.9, lay 45.0, con 4.2, n 11.3 (pf misses?); rsplit tile 84.6, comb 75.1, xdup 44.6, clip 25.1, pcomp 13.0. These are counts, not times. 49 plan + 26 mode + 45 lay at sync 2.99 ms gives about 25 us per resolve if all are separate, and 40 us per sync call if comb 75 [I].

## Answer
1. The largest owner is the texture-op streak, 18.0 ms of 34.3 ms (52 %) [M]. 8.4 ms is flush closes at tex ops, and 207.7 of 281.9 of those draw nothing (about 30 us each) [M]. The empty close cost is the biggest single removable item. Skipping empty tex closes at most saves 6 to 7 ms (207.7 x 30 us) [I].
2. The tex streak still holds 6.6 to 8.2 ms that is not a flush and not a cache lookup: the SETTILE/LOADBLOCK/TEXTURE handlers themselves (rsplit tile 84.6 per frame) [I]. At about 85 us per tile event if all of it were there. Unverified.
3. Triangle streak is small, 2.55 ms. Vertex transform is not shown by any field. It is in the 13.8 ms residual, not measurable [M that no counter exists].
4. "other" 7.4 ms is outside W and is not part of this split.

## CPU work or memory stalls
- 7.6 us/op = about 2040 cycles at 268 MHz [M arithmetic]. A plain G_VTX transform of 16 vertices or a combiner hash is a few hundred cycles. The ops average hides that most ops are cheap and a few classes cost 20 to 100 us [I].
- Evidence for CPU/state work, not DL read stalls: cost concentrates in tex-class closes (30 us per EMPTY close, 16.7 us per cache hit) and not evenly per op [M]. An empty close that draws nothing still costs about 8000 cycles. That fits state sync, hashing or a heavy flush_impl path, and also fits cache-missing on large state structs [I]. The data cannot separate the two.
- Evidence against DL-read stalls as the main cost: per-op cost differs by scene (7.6 yard, 12.3 intro, 15.6 title, 10.8 battle us/op) but marginal cost between scenes is 3.9 us/op (yard vs change_map, +2040 ops +7.9 ms) to 9.4 us/op (battle vs change_map) [I from console_numbers.md section 3]. A sequential DL read stall would be constant per op. Fixed per-close cost dominates.
- Azahar 2.8 us/op vs console 7.6 is a 2.7x ratio, which fits a pure clock/IPC difference for CPU work (Azahar host runs the same code) [I]. A memory-stall term would not appear in Azahar. So the 2.7x does not isolate stalls.
- Verdict [I]: CPU work in tex/state handling (closes, resolves) first. Memory stalls are possible but unproven. No counter in the log separates them.

## Missing counters and smallest design
Missing: time and count by op class for VTX, MTX/POPMTX, geometry mode, combiner/othermode set ops, DL branch, and a per-class op count (dls_ops exists under dls_on but is not logged by default). dlcsplit time is missing.
Design (sampled, no per-op ticks): in the dispatch loop (pm_gbi.c ~2488), keep op class from a 256-entry table. Every 64th op (n_ops & 63 == 0), take t0 = pm_ticks() before the switch and t1 after, add (t1-t0)*64 to cls_us[class] and count cls_n[class]++ (always, cheap increment, no tick). Cost: one tick pair per 64 ops, 71 pairs per frame, 71 x 1.3 us x 2 = about 0.2 ms. Print "opsplit f vtx us/n mtx us/n geo us/n cc us/n tex us/n tri us/n dl us/n" beside gbisplit. Error is statistical; use full 2718 frames. Exclude nested flush time (fl_us delta) by subtracting pm_fl_us delta over the same sample. Add a ticks pair around dlc replay for a dlc time field.
Cheaper alternative for the memory question: log cache-miss counters is not available on ARM11 from user mode, so compare two scenes at equal ops, one with a warm replay (dlc hit) and one cold, and look at W per op [I].
