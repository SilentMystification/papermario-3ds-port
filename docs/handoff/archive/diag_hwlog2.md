# HWLOG2: O3DS log of build 1456 (6e965c84b), empty live log
Source: build3ds/ftp_170b/hw_o3ds_log.txt, 3261 lines, sha256 ce607d3c07df7d9b. Cited hash f010c91f is the 3dsx hash. Tags: [M] measured, [C] code.
## Job 1. Log readout
### Boot [M]
- #8: build 1456, git 6e965c84b, loglevel 3.
- #10: switches --verbose audiostat texstat pacestat logstat phase phsum loglevel=debug.
- #9: MODE = O3DS / 64MB cap. #13: RAM resident 24.66 MB.
### Span, maps, gaps, fatal [M]
- Line ids #0 to #3260 are contiguous. Phase windows cover f2 to f3774 with no gap.
- Map loads: area5 map5 (f302), area0 map6 (f687), area0 map10 (f1347), area0 map9 (f1529), area0 map8 (f1952), area1 map1 (f2314), area1 map2 (f2638). The log has no map names.
- "lines dropped" and "phrec dropped": 0.
- No fatal, crash or abort line. Normal exit at #3257 (menu: close).
### Frame time per map [M]
| Map | Frames | Mean wall ms |
|---|---|---|
| area1 map2 (mac_01, inferred) | 1137 (f2638-3774) | 57.0 |
| area1 map1 | 324 (f2314-2637) | 49.9 |
| area0 maps 6, 8, 9, 10 | 362 to 660 each | 33.4 to 33.5 |
- Toad Town frame time changed from 60.6 ms to 57.0 ms. Change: -3.6 ms, -5.9 %.
- The mac_01 mapping to area1 map2 is an inference. This is one run.
- gbi mean per frame is not in this log. The spike mean (gbi 36.0 ms) is biased to slow frames.
### Ops per frame [M]
- opsplit: f2699 1553, f2999 1677, f3299 1662, f3599 1687. Mean 1645.
- Earlier mac_01: 2156 (diag text says 2157). Change: about -510 ops (-24 %).
- Yard 1204: no yard map in this log.
### CROSS2 splits [M]
- dr1 counter (cumulative, every 300 frames), splits per frame:
  - f2400-2700: 1.55. f2700-3000: 1.85. f3000-3300: 1.77. f3300-3600: 2.00.
- Zero splits from f1500 to f2400.
- The rate is 1.5 to 2.0 per frame in map2. This is not exactly two. The b7 splits counter stays 0.
### Stage cost and gcache [M]
- dcsum f2699: 36 closes per frame. Per close (us): pal 17, bind 11, tev 12, st 13, draw 40. gc 31 us per flush, 7 flushes per frame.
- dcsum f3599: 63 closes per frame. Per close (us): pal 17, bind 10, tev 10, st 12, draw 38. gc 33 us per flush, 8 flushes per frame.
### Pace, HUD, producers [M]
- pace: 20.07 steps/s at #3250. 16.96 steps/s at #3034.
- #124 hud: MEM used 32.2 / 43.6 MB. VRAM min 2130 KB, peak 4014 KB.
- #1542-1545: texcache VRAM full, one eviction each.
- f2999 producer 0015f0f0: 1541 of 1677 ops (92 %).
## Job 2. Empty live log
### Path [C]
- push_3ds.sh:172 runs livelog.py. Output file: build3ds/hw_live_log_<octet>.txt (push_3ds.sh:171). Default read time 3600 s (push_3ds.sh:170).
- livelog.py:23 opens the file "wb" before connecting. The 0-byte file at 15:07 shows that the client started.
- livelog.py:38 connects to port 17492 every 1 s. It prints a message only on connect, and "no connection" only at normal exit.
- Game: pm_log.c:228 listens on 17492. pm_log.c:591 accepts in the writer loop. pm_main.c:610 sets live. pm_main.c:617 calls pm_log_init before pm_log_start.
### Why no bytes [C] [M]
- The game sends nothing until it accepts a client. Then it sends its 32 KB ring.
- A 0-byte file means no connect succeeded.
- Cause 1, most likely: the socket never listened. pm_log.c:210 socInit failed, or live was 0.
- Hidden status: vlog returns when no writer exists (pm_log.c:~305). pm_log_init runs before pm_log_start, so the "live log: port" or "live log: off" line is dropped. The FTP log has no such line. [M, absence]
- Cause 2, possible: Ctrl-C stopped livelog. Python then prints nothing.
- Git: the live path worked at 29f504d7b (HWLOG1: 18646 lines). Since then, 3141a0913 and b7375d4f0 changed pm_log.c and pm_main.c. A keyword diff shows no change in the socket path. push_3ds.sh and livelog.py are unchanged.
### Smallest fix [C]
- Emit the live-init status after pm_log_start, or write it to log_fd directly. The next push then shows the socInit code in log.txt.
- Missing evidence: the push terminal output ("3DS at", "netload attempt", "[livelog] connected", any Ctrl-C).
