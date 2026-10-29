# HWLOG3: O3DS live log, build 1467 (git 06e62c0aa), cheap flag set

Source: build3ds/hw_live_log_170.txt, ids #0 to #2137 (file lines 1 to 2047, 244,340 bytes).
The file grew during the review to 2478 lines (297,866 bytes, last id #3869, 16:25:34).
Ids after #2137 are not in this report. Tags: [M] measured in the log, [E] estimate.
Ids are the log ids (#n). Ln is the file line.

## 1. Boot, span, maps, gaps
- #8 (ln 9): build 1467, git 06e62c0aa, loglevel 3. [M]
- #10 (ln 11): flags --verbose livelog pacestat logstat dcsum loglevel=debug. No phase, texstat or audiostat. [M]
- #13 (ln 14): rom mode RAM resident 24.66 MB. #3 (ln 4): live port 17492, socInit 00000000. [M]
- No timestamps. 418 pace lines sum to 424.8 s of paced run. [M]
- Map loads, no names: #155 area 5 map 5; #261 area 1 map 2; #392 area 1 map 1; #488 area 0 map 8; #805 area 1 map 1; #870 area 1 map 2. No warp lines. [M]
- Gap: "[livelog] GAP: lines #1792-#1884 missing" (ln 1794). 93 ids lost. [M]
- Game frames f10044 (#1790) to f10377 (#1885) across the gap. dcsum f10638 is absent. [M]
- Writer: dropped 0 in all 30 logstat windows. live eagain 14 and stall closes 1 from #1906. [M]
- Fatal, crash, panic, abort and sleep lines: none. ROM READ IN DROPPED RANGE: 2 lines (#337, #338). [M]

## 2. Frame time per map (logstat, 300-frame windows; pace = 1 s lines)
| Ids | Map | Windows | Wall ms mean (min to max) | fps | pace fps |
|---|---|---|---|---|---|
| #144 | pre-map | 1 | 33.34 | 30.0 | 29.9 |
| #213 | area 5 map 5 | 1 | 34.75 | 28.8 | 28.5 |
| #311 to #375 | area 1 map 2, first | 2 | 42.73 (41.39 to 44.07) | 23.4 | 22.9 |
| #436 | area 1 map 1, first | 1 | 42.22 | 23.7 | 23.3 |
| #514 to #793 | area 0 map 8 | 7 | 39.54 (33.41 to 46.02) | 25.3 | 31.4, not reconciled |
| #846 | area 1 map 1, second | 1 | 37.04 | 27.0 | 24.9 |
| #911 to #1011 | area 1 map 2, ramp | 3 | 42.52 (37.84 to 45.41) | 23.5 | 23.0 |
| #1081 to #2137 | area 1 map 2, steady | 14 | 48.49 (48.25 to 48.83) | 20.6 | 20.6 (19.5 to 21.1) |

- Toad Town = area 1 map 2 (mac_01, inferred in diag_hwlog2). The log names no map. [M]
- Yard (kmr_*): no map in this log can be identified. Not measured. [M]
- Steady view is static: dlc2 tri 277040 and opsrep 340800 stay equal from f7499 to f11699. [M]

Toad Town steady, compared:
| Build | Flags | Frame ms | fps |
|---|---|---|---|
| 29f504d7b (ftp_170) | full | 60.6 | 16.5 |
| final5 (ftp_170c) | full phase | 57.0 | 17.5 |
| 1467 (this log) | cheap set | 48.5 | 20.6 |

- Difference: -12.1 ms (-20 %) to 60.6, and -8.5 ms (-15 %) to 57.0. [M]
- Builds and scenes differ. The difference is not the instrument cost alone. [M]
- Azahar phase cost was 1.08 ms (FINAL_WRITEUP, LOGFIX). Console gap is larger. [E]
- Unlogged 20 fps (user report) is 48.8 to 51.3 ms. The cheap set sits at the fast edge. Residual logging cost is not measured. [E]

## 3. dcsum gates (console, f7638 to f11838, 14 lines, ids #1073 to #2136)
| Field | Value |
|---|---|
| Non-empty closes per frame | 76.6 (76 to 77) [M] |
| pal / bind / tev / st / draw, us per close | 10.0 / 9.0 / 8.9 / 10.9 / 30.6 [M] |
| gc, us per flush | mean 6.0, median 1, max 28 [M] |
| Flush calls per frame | 7 on all 14 lines [M] |
| svc flush fallbacks | 0 on all lines [M] |
| cmd permil per sampled close | 0 on all lines. Field unusable. [M] |

- Build 1456 (diag_hwlog2, f2699 and f3599) read gc 31 and 33 us per flush, with no svc field. Now median 1 us. [M]
- Gc per frame falls from about 220 us to about 10 us. [E]
- The svc flush change is the likely cause. [E]
- Bind per bind: the log has no bind count. Plan_draw_cost gives 1.09 binds per close (tb0 61 per 56, build 29f504d7b). Bind per bind is 9.0 / 1.09 = 8.3 us. [E]
- DS4 gate (bind above 30 us per bind): FAIL. About 8 us per bind. [E]
- DS5 gate (draw + st + tev + bind above 60 us per close): FAIL. Sum is 59.4 us mean, 57 to 60 us per line, 9 of 14 lines at 60. Printed values carry +/- 2 us rounding. [M]
- Stage sum with pal is 69.4 us per close, times 76.6 closes, is 5.3 ms per frame. [E]

## 4. Split of the Toad Town frame (steady)
- Work 48.18 ms, wall 48.49 ms (logstat means). Wall minus work is 0.31 ms. [M]
- The cheap set has no phase. No gbi, gpu, flush, swap or logic split exists in the log. [M]
- Top three parts from dcsum (closes x us per close, [E]): draw 2.3 ms, st 0.8 ms, pal 0.8 ms.
- Audio join: 0 us (ln 221, ln 453). [M]
- The log cannot show gbi ms inside work, GPU wait, swap, or logic. About 43 ms of 48.5 ms has no split. [M]

## 5. Other counters
- CROSS2 splits: 2.00 per frame in steady state (f8700 to f11700). Zero from f4800 to f6300. [M]
- Texture: texstat is off. texmiss 0/0KB on all spike lines. texture failures 0 (#1984). [M]
- VRAM A and B: used 2007 KB, free 1065 KB, min 41 KB, unaccounted 0 KB, fail 0 (#874, #1895). [M]
- VRAM free min 82 KB (#810). First area 1 map 2 load: used 3031 KB, free 41 KB (#265). [M]
- MEM (#114): used 32.2 of 43.6 MB, free 11.3 MB. Heap free-in-arena 62 KB, tex-cache 2842 KB (#876). [M]
- Writer maxima: console 1142405 us (#146), file 461114 us (#377), socket 333985 us (#708). fbuf 971 of 131072 B (#313), fbuf waits 0. [M]
- Spike lines (frames over 50 ms) in steady: 350 of about 4200 frames, about 8 %. Mean 52.2 ms, max 57.7 ms. [M, E]
- Sleep: none. hudint: not logged. Map loads after #2137 (area 8, ids #3634 to #3784) are not analysed. [M]
