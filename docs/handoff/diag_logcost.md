# LOGCOST: cost of logging and phase instruments (O3DS, build 1462, git 835be2fd7)

Tags: [M] measured from log or code count. [E] estimate. [C] code fact.
Source: build3ds/hw_live_log_170.txt (4456 lines, push flags). Azahar starts used: 0 of 3.

## 1. Log numbers (push flags)
- Drawn frames: 4503 in 106 phase windows. Span 249.3 s. [M]
- Frame rate: 18.1 fps. Without boot window f1: 18.7 fps, mean wall 53.5 ms. [M]
- Late window f4471-4503: wall 62.4 ms. Pace #4448: 16.1 steps/s. [M]
- Lines: 0.99 per frame, 17.9 per s. Bytes: 128 per frame, 2.3 KB per s. [M]
- logstat #4441 says 1.6 lines per frame. The file count says 0.99. Open. [M]

| Family | Lines | Bytes | Share |
|---|---|---|---|
| phase, phs, phh, phc | 1415 | 230567 | 40.0 % |
| spike (frame over 50 ms) | 1224 | 147603 | 25.6 % |
| pace, b7 | 355 | 59876 | 10.4 % |
| census and ops | 655 | 56283 | 9.8 % |
| audio | 530 | 56017 | 9.7 % |
| tex, mem, vram, logstat | 246 | 24196 | 4.2 % |

## 2. Per-frame cost with phase on
Late frames #4399-#4410 [M]: gbi 38.9, fbegin 5.3, gpu 5.2, gdraw 4.1, dl 3.7, logic 2.8, other 1.4, fend 0.7 ms.
Counts per frame [M]: gbiops 1708, draw 120, flush 120, texcall 96, dbgcall 29.

| Part | Work per frame | ms per frame | Tag |
|---|---|---|---|
| tick reads (one SVC, 0.99 us [M]) | about 1.7k: push/pop per draw, timers per flush, kick, texcall, DC 1 in 8 | 1.5 to 1.9 | E |
| per-op hooks (pm_gbi.c:3307-3309) | 3 calls per op, plus loss of fast loop (pm_gbi.c:3231) | 1.0 to 2.0 | E |
| texture timers (pm_tex.c:805) | tc_on timers | 0.1 to 0.5 | E |
| switch lookups | 29 x 0.70 us [M] | 0.02 | M |
| log formatting | 1 line, spike line, 2 s summary | 0.05 to 0.1 | E |
| writer, sink, socket, SD | core 1. vlog takes no lock (pm_log.c:322-386) | 0 on game thread | C |
| Total, phase and logging | | 3 to 4.5 | E |

Console gap: 9 to 17 ms (20 fps off, 15 to 17 fps with flags). The estimate covers 3 to 4.5 ms. 5 to 13 ms is not explained.

## 3. Cheapest set for speed
Keep: `pacestat logstat loglevel=debug`.
- pacestat: one line per s (fps, steps/s, late steps over 33.3 ms). Gate pm_main.c:340.
- logstat: frame ms per 300 frames. Uses pm_log_hw, so loglevel=debug is needed.
Drop: phase, phsum, texstat, audiostat, gbiprof.
- dcsum cannot be kept without phase (pm_gbi.c:2317 runs under pm_ph_on). A dcsum run costs the full phase cost.
Frame-time overhead of the set: 0.1 to 0.4 ms [E]. Below 0.05 ms after the fix in section 5.

## 4. Instruments that run with phase off (default build)
| Site | What runs | Cost | Status |
|---|---|---|---|
| pm_depth.c:14-19 | 2 ticks before id test, no gate | 2 SVC per query [M]. 0 queries in this log | BUG, ungated |
| pm_tex.c:805 | tc_on = texstat OR loglevel 3 | texture ticks at loglevel 3 | BUG, hidden with loglevel=debug |
| pm_audio.c:190-220 | kick, join, retrace ticks | about 5 us per frame [E] | ungated, small |
| pm_au2.c:110-146 | sync ticks when pm_au2_active | 1 to 2 per call [C] | ungated |
| pm_log.c:118 | pm_debug_has counter | 0.70 us per call [M] | ungated, small |
| writer and sink threads | run by default (pm_log.c:61) | 0 on game thread [C] | OK |

## 5. Fixes for the next code row (not this row)
1. pm_tex.c:805: drop `|| pm_log_level() >= 3`.
2. pm_depth.c:14-19: gate the ticks on pm_ph_on.
3. pm_audio.c tick sites: gate on audiostat or pm_ph_on.

## 6. Azahar cost test (item 5)
Not run. The catalog has no yard scenario with phase on. A new scenario needs a catalog row in the same commit. 0 of 3 starts used.
Console A/B: (a) no flags, (b) `pacestat logstat loglevel=debug`, (c) full push flags. Compare fps.

Open: two frame counters (phase ends f4503 at #4399, b7 reaches f6900 at #4455). [M]
