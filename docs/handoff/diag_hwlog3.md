# HWLOG3: O3DS live log, build 1467 (git 06e62c0aa), cheap flag set

Source: build3ds/hw_live_log_170.txt, ids #0 to #2137 (file lines 1 to 2047, 244,340 bytes).
The file grew to 2478 lines during the review. Ids after #2137 are not in this report. Tags: [M] measured, [E] estimate. Ln is the file line.

## 1. Boot, span, maps, gaps
- #8 (ln 9): build 1467, git 06e62c0aa, loglevel 3. [M]
- #10 (ln 11): flags --verbose livelog pacestat logstat dcsum loglevel=debug. No phase flag. [M]
- #13 (ln 14): rom mode RAM resident 24.66 MB. #3 (ln 4): live port 17492, socInit 00000000. [M]
- No timestamps. 418 pace lines sum to 424.8 s. [M]
- Map loads, no names: #155 area 5 map 5, #261 area 1 map 2, #392 area 1 map 1, #488 area 0 map 8, #805 area 1 map 1, #870 area 1 map 2. [M]
- Gap at ln 1794: #1792-#1884 (93 ids), frames f10044 to f10377. [M]
- Writer dropped 0. Live eagain 14 and stall 1 from #1906. No fatal, crash or sleep line. [M]

## 2. Frame time per map (logstat 300-frame windows, pace 1 s lines)
| Ids | Map | Windows | Wall ms mean (min to max) | fps | pace fps |
|---|---|---|---|---|---|
| #144 | pre-map | 1 | 33.34 | 30.0 | 29.9 |
| #213 | area 5 map 5 | 1 | 34.75 | 28.8 | 28.5 |
| #311 to #375 | area 1 map 2, first | 2 | 42.73 (41.39 to 44.07) | 23.4 | 22.9 |
| #436 | area 1 map 1, first | 1 | 42.22 | 23.7 | 23.3 |
| #514 to #793 | area 0 map 8 | 7 | 39.54 (33.41 to 46.02) | 25.3 | 31.4, not reconciled |
| #846 | area 1 map 1, second | 1 | 37.04 | 27.0 | 24.9 |
| #911 to #1011 | area 1 map 2, ramp | 3 | 42.52 (37.84 to 45.41) | 23.5 | 23.0 |
| #1081 to #2137 | area 1 map 2, steady | 14 | 48.49 (48.25 to 48.83) | 20.6 | 20.6 |

- Toad Town = area 1 map 2 (mac_01, inferred in diag_hwlog2). The log names no map. Yard (kmr_*) is not identified. [M]
- Steady view is static: dlc2 tri 277040 is equal from f7499 to f11699. [M]
- Toad Town steady: 60.6 ms (29f504d7b, full set), 57.0 ms (final5, full set), 48.5 ms (this build, cheap set). [M]
- Change: -12.1 ms (-20 %) against 60.6, -8.5 ms (-15 %) against 57.0. Builds and scenes differ. [M]
- Azahar full-set cost was 1.08 ms, so the console gap is larger. Unlogged 20 fps is 48.8 to 51.3 ms. Residual logging cost is not measured. [E]

## 3. dcsum gates (console, f7638 to f11838, 14 lines, ids #1073 to #2136)
| Field | Value |
|---|---|
| Non-empty closes per frame | 76.6 (76 to 77) [M] |
| pal / bind / tev / st / draw, us per close | 10.0 / 9.0 / 8.9 / 10.9 / 30.6 [M] |
| gc, us per flush | mean 6.0, median 1, max 28 [M] |
| Flush calls per frame, svc fallbacks | 7 and 0 on all 14 lines [M] |
| cmd permil per sampled close | 0 on all lines. Field unusable. [M] |

- Build 1456 (diag_hwlog2) read gc 31 and 33 us per flush. Now median 1 us. Gc per frame falls from about 220 us to about 10 us. [M, E]
- The svc flush change is the likely cause. [E]
- The log has no bind count. Plan_draw_cost gives 1.09 binds per close, so bind per bind is 9.0 / 1.09 = 8.3 us. [E]
- DS4 gate (bind above 30 us per bind): FAIL, about 8 us per bind. [E]
- DS5 gate (draw, st, tev and bind above 60 us per close): FAIL. Sum is 59.4 us mean, 57 to 60 us per line. Rounding allows +/- 2 us. [M]

## 4. Split of the Toad Town frame (steady)
- Work 48.18 ms, wall 48.49 ms. Wall minus work is 0.31 ms. The cheap set has no phase, so no gbi, gpu, flush, swap or logic split exists. [M]
- Top three parts from dcsum, closes times us per close: draw 2.3 ms, st 0.8 ms, pal 0.8 ms. [E] Audio join is 0 us (ln 221, ln 453). [M]
- The log cannot show gbi ms inside work, GPU wait, swap or logic. About 43 ms of 48.5 ms has no split. [M]

## 5. Other counters
- CROSS2 splits: 2.00 per frame in steady state (f8700 to f11700). Zero from f4800 to f6300. [M]
- texstat is off. texmiss 0/0KB on all spike lines. Texture failures 0 (#1984). [M]
- VRAM A and B: used 2007 KB, free 1065 KB, min 41 KB, fail 0 (#874, #1895). [M]
- MEM (#114): used 32.2 of 43.6 MB, free 11.3 MB. Heap free-in-arena 62 KB (#876). [M]
- Writer maxima: console 1142405 us (#146), file 461114 us (#377), socket 333985 us (#708). fbuf 971 of 131072 B, waits 0. [M]
- Steady spike lines (frames over 50 ms): 350 of about 4200 frames, about 8 %. Mean 52.2 ms, max 57.7 ms. [M, E]
- Sleep: none. hudint: not logged. Map loads after #2137 (area 8) are not analysed. [M]
