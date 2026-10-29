# HWLOG diagnosis: O3DS log after build 29f504d7b

Sources: build3ds/ftp_170/hw_o3ds_log.txt (62660 lines), build3ds/hw_live_log_170.txt (18646 lines). Line numbers are FTP file lines.

## 1. Build and boot
- No boot lines in the FTP log. Line 1 is "#11 menu: tab 2 row 4 Logging = On". Build id and loglevel are not logged.
- Logging gate open (per-frame lines present). Live log starts at "#35409 gbisplit f2263". It is a partial capture.

## 2. Log gaps
- Gap A: frames 1854-1959 missing (line 30189). Window 1960-1972: span 3666 ms, max 3006 ms, other 2950 ms.
- Gap B: frames 2042-2087 missing (line 31687). Window 2088-2121: max 805.9 ms.
- Gap C: frames 3453-3571 missing (line 44132). Scene change (warp, line 43084).
- Total missing: 271 frames of about 5963 (4.5 %).
- "phrec dropped": 23 events, 1925 frame records, writer behind (lines 36720-61657).
- "lines dropped (queue full)": 5 events.
- Writer stalls (logstat2): fbuf max 6.7-7.1 s at lines 38997, 41024, 45179, 49352, 54307, 57515. Socket max 1.17 s (line 54307). Fbuf 122137 of 131072 bytes (line 56367).
- Likely cause of dropped records: writer behind, buffer near full. Cause of gaps A and C not determined.

## 3. Frame time per scene
Yard label is not in the log. Scene A is inferred from frame time.

| Scene (lines) | Frames | ms/frame | gbi ms | gbiops | texcall |
|---|---|---|---|---|---|
| S0 boot/menu (1-16307) | 998 | 63.7 | 31.1 | 2116 | 60.6 |
| A area1 map2 (16308-34716) | 1143 | 60.6 | 31.7 | 2157 | 63.0 |
| B area1 map3 (34717-43053) | 981 | 35.0 | 19.0 | 1416 | 27.4 |
| C area0 map9 (43054-45129) | 300 | 33.7 | 16.4 | 1131 | 33.2 |
| D area0 map10 (45130-53296) | 1198 | 35.3 | 13.6 | 731 | 36.1 |
| E maps 6,5,6 (53297-59537) | 877 | 34.6 | 15.6 | 913 | 37.0 |

- Scene A: 60.6 ms against yard baseline 58.0 ms.
- Scenes B to E: 33.7-35.3 ms against walk baseline 34.9 ms.
- Ops: scene A has 2157 gbi ops per frame. Expected 1204. Not reached.
- Texture close: no per-close timer. Scene A "tex" is 0.29 ms per frame. That gives about 4.6 us per close, not 97 us. Baseline not reproduced.

## 4. Per-frame cost split (scene A, mean of window averages)
- gbi (CPU in gbi run): 31.7 ms, 52 % of frame.
- fbegin: 4.4 ms. gpu: 3.8 ms. GPU wait not timed separately.
- logic 3.6, dl 3.4, gdraw 2.9, gcache 1.7, fend 1.4, tex 0.3, audio 0.1 ms.
- other: 12.8 ms, includes gaps A and B. Steady other is 2.3-2.8 ms in scenes B to E.
- Top 3 named costs: gbi 31.7 ms, fbegin 4.4 ms, gpu 3.8 ms.
- Flush: 5.8 calls per frame, not timed. Swap: not timed. Walk scenes idle 3.4-6.9 ms per frame.

## 5. Logging, texture cache, VRAM
- Log lines: 11.6 per frame at frame 300 (line 3483), 16.2 at frame 2100 (line 13112). Bytes 270-386 KB per 300 frames.
- Texcache evictions (texstat): f2400 15, f3900 37, f4800 89 (704 KB), f5400 266 (1495 KB).
- "texcache VRAM full": 8 lines (17872-18045). Texture failures: 0 (line 34136).
- VRAM A and B: used 2007 KB, free 1065 KB, min 41 KB, unaccounted 0 KB, fail 0 (line 16338).

## 6. Crash, fatal, sleep
- No crash, fatal, panic or sleep line.
- Log ends with "menu: close" (62507) and "boot_main returned" (62508). Reason not logged.
- Game frame counter 7209 (line 62009). Log frames end near 5963. Counters differ, cause not determined.

## Open items
- Read build id and loglevel from the O3DS boot.
- Find cause of gap A (3 s stall, line 30189).
- Reduce writer load: console sink max 1.8-2.2 s, fbuf near full.
- Time the texture close path. Reconcile 97 us.
- Find ops 2157 per frame in scene A against expected 1204.
