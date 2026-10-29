# LOGCOST: cost of logging and phase instruments (O3DS, build 1462, git 835be2fd7)

Tags: [M] measured from log or code count. [E] estimate. [C] code fact.
Source: build3ds/hw_live_log_170.txt (4456 lines, push flags). Azahar starts used: 0 of 3.

## 1. Log numbers (push flags)
- Drawn frames: 4503 in 106 phase windows. Span 249.3 s. [M]
- Frame rate: 18.1 fps over the span. Without boot window f1: 18.7 fps, mean wall 53.5 ms. [M]
- Late window f4471-4503: wall 62.4 ms. Pace line #4448: 16.1 steps/s. [M]
- Lines: 0.99 per frame, 17.9 per s. [M]
- Bytes: 128 per frame, 2.3 KB per s, 576062 B total. [M]
- logstat #4441 gives 1.6 lines per frame and 209 B per frame. This differs from the file count. Open. [M]

| Family | Lines | Bytes | Share of bytes |
|---|---|---|---|
| phase, phs, phh, phc | 1415 | 230567 | 40.0 % |
| spike (frame over 50 ms) | 1224 | 147603 | 25.6 % |
| pace, b7 | 355 | 59876 | 10.4 % |
| census and ops (mdlsum, opsplit, dcsum, prod, mops, dlc, gv, gp, tx1) | 655 | 56283 | 9.8 % |
| audio (audio, audio2, auddsp, V09F5, bgm) | 530 | 56017 | 9.7 % |
| tex, mem, vram, logstat | 246 | 24196 | 4.2 % |

Log volume is small. The per-frame work costs more than the lines.

## 2. Per-frame cost with phase on
Late frames #4399-#4410 [M]: gbi 38.9, fbegin 5.3, gpu 5.2, gdraw 4.1, dl 3.7, logic 2.8, other 1.4, fend 0.7 ms.
Counts per frame [M]: gbiops 1708, draw 120, flush 120, texcall 96, vtxslot 3528, dbgcall 29.

| Part | Work per frame | ms per frame | Tag |
|---|---|---|---|
| tick reads (pm_ticks is one SVC, 0.99 us [M]) | about 1.7k: 2 push/pop pairs per draw, 2 timers per flush and kick, 2 per texcall, DC timer 1 in 8, gbi op sample 1 in 64 | 1.5 to 1.9 | E |
| per-op hooks (pm_gbi.c:3307-3309) | os_op_hook, mops_count, mdc_op: 3 calls per op, plus loss of fast loop (pm_gbi.c:3231) | 1.0 to 2.0 | E |
| texture timers | tc_on (pm_tex.c:805): pm_tex.c 924, 934, 1173, 1370 | 0.1 to 0.5 | E |
| switch lookups | 29 per frame x 0.70 us [M] | 0.02 | M |
| log formatting | 1 line per frame, spike line, 2 s summary | 0.05 to 0.1 | E |
| writer, sink, socket, SD | core 1. vlog takes no lock (atomics only, pm_log.c:322-386) | 0 on game thread | C |
| phsum extras | ps_step per frame, qsort every 300 frames | below 0.02 | E |
| Total, phase and logging | | 3 to 4.5 | E |

Console gap: 20 fps (50 ms) off, 15 to 17 fps (59 to 67 ms) with flags. That is 9 to 17 ms. The estimate covers 3 to 4.5 ms. So 5 to 13 ms is not explained. Open.

## 3. Cheapest set for speed
Keep: `pacestat logstat loglevel=debug`.
- pacestat: one pace line per s (fps, steps/s, late steps over 33.3 ms). Gate pm_main.c:340.
- logstat: frame ms per 300 frames. Uses pm_log_hw, so loglevel=debug is needed. pm_main.c:523-533.
- loglevel=debug enables tc_on (pm_tex.c:805). This is a hidden cost (see section 4).
Drop: phase, phsum, texstat, audiostat, gbiprof. Each one of them is per-op or per-draw cost.
- dcsum cannot be kept without phase. It runs from pm_gbi.c:2317 under pm_ph_on. A dcsum run costs the full phase cost (3 to 4.5 ms).
Frame-time overhead of the set: 0.1 to 0.4 ms [E]. After the tc_on fix in section 5: below 0.05 ms [E].

## 4. Instruments that run with phase off (default build)
| Site | What runs | Cost | Status |
|---|---|---|---|
| pm_depth.c:14-19 | 2 pm_ticks before the id test, no gate, pm_b7 always | 2 SVC per query, about 2 us [M]. Queries 0 in this log (#4359) | BUG, ungated |
| pm_tex.c:805 | tc_on = texstat OR loglevel 3 | 0 at default level 1. Texture ticks at loglevel 3 | BUG, hidden cost with loglevel=debug |
| pm_audio.c:190-220 | kick tick, join waits, retrace send and end | about 5 ticks per frame, 5 us [E] | ungated, small |
| pm_au2.c:110-146 | sync ticks when pm_au2_active | 1 to 2 per call [C], count not in log | ungated |
| pm_log.c:118 | pm_debug_has counter and cache lookup | 0.70 us per call [M] | ungated, small |
| pm_main.c:486-494 | svcGetSystemTick per frame (pace clock) | needed by pace | OK |
| writer and sink threads | run by default (log_file_on = 1, pm_log.c:61). Writes warn lines to log.txt | 0 on game thread [C] | OK |

## 5. Fixes for the next code row (not this row)
1. pm_tex.c:805: drop `|| pm_log_level() >= 3`. Texture timers then follow texstat only.
2. pm_depth.c:14-19: gate the ticks on pm_ph_on.
3. pm_audio.c tick sites: gate on audiostat or pm_ph_on.

## 6. Azahar cost test (item 5)
Not run. The catalog has no yard scenario with phase on. The yard scenario has loglevel=debug and no phase (azt_scenarios.json). A phase-on variant needs a new scenario and its catalog row in one commit. That change is outside this doc-only commit. 0 of 3 starts used.
Console A/B is the test. Run three states and compare fps: (a) no flags, (b) `pacestat logstat loglevel=debug`, (c) full push flags.

## 7. Open
- Two frame counters. Phase windows end at f4503 (#4399). b7 and V09 lines reach f6900 (#4455). [M]
- logstat line count (1.6 per frame) differs from file count (0.99 per frame). [M]
- 5 to 13 ms of the console gap is not explained. Run the console A/B in section 6.
