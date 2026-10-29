# OPS2: op count, console scene A against Azahar

Row OPS2 (diagnostic). No source edits. Azahar starts used: 1 of 3 (scenario tour10).

## 1. Build and log identity
- build3ds/pm_3ds.3dsx SHA-256 = 0df21e8c3a76b2d0db03b1cf1142cb3a0f4f87edcf4cc28215ce7bc50b2d549c.
  Same hash as 3ds/logs/builds/pm_3ds_final_29f504d7b.3dsx and FINAL_WRITEUP.md line 4. The binary is git 29f504d7b.
- No build id, git hash or rom mode line in either log. The first FTP line is "#11 menu: tab 2 row 4 Logging = On".
- Log level: no boot line. In-game menu sets "Log level = Debug" at build3ds/ftp_170/hw_o3ds_log.txt line 31266 (#32672), inside scene A.
- Live log build3ds/hw_live_log_170.txt (18646 lines, #35409 to #63955) and FTP log (62660 lines, #11 to #64066) are one run.
  Check: "#38125 dlc2 f3899 tri 99382 opsrep 286010" is at live line 1055 and FTP line 36719, same values.
- pushed_builds.txt has no push line for 29f504d7b. Only the archive line (line 30).

## 2. Op counters, console scene A (FTP lines 16308 to 34716)
- Scene A = "load map area 1 map 2" (FTP line 16308). Opsplit samples: f2482 (line 19242), f2782 (24120), f3082 (28981).
- Total ops per frame: 2145 (mean of 3 samples). gbiops 2157 (diag_hw_log1.md).
- Per class, ops per frame (console): tex 597, geo 470, dl 289, cc 239, mtx 197, dlc 96, tri 95, oth 102, vtx 58, sync 0.
- Other counters, console scene A mean: rmsplit n (G_PM_RM ops) 89.5, sk (sync and no-op skipped) 622, sks (state ops skipped) 402, tbsplit n (G_PM_TEX binds) 87.6, dlc ops 1122.

## 3. Scene identity and Azahar comparison
- Console scene A = area 1 map 2 = mac_01 (Toad Town). Source: build3ds/last_log.txt line 2914 (tour: next mac_01 entry 0 (area 1 map 2)).
- Azahar yard = kmr_20 = area 0 map 11. Source: build3ds/lane-dl/last_log.txt line 149. The scenes differ.
- Azahar scenario tour10 (run at 11:03, row OPS2) visits mac_01 at log lines 3271 to 4190.
  Taskpl ops mean on mac_01: 2045 (52 frames with ops above 0). Console scene A: 2156 (1132 frames). Difference +5.4 %.
- Azahar kmr_20 segment in the same run (lines 7157 to 8116): taskpl ops mean 993 (54 frames).
- The 1204 yard figure (FINAL_WRITEUP.md line 55) came from build3ds/lane-final/last_log.txt. That file no longer has taskpl lines. It cannot be checked from the tree.
- Azahar opsplit: one line in the tour10 run, f299 (build3ds/last_log.txt line 5459). Map is dgb_01, not mac_01. Total 1489. Classes: tex 400, geo 357, cc 158, dl 182, mtx 127, dlc 81, oth 86, tri 51, vtx 37, sync 0.
- Azahar G_PM_RM n at f299: 85. Azahar tbsplit n at f290 to f299: 82. Azahar sk at f299: 376.

## 4. Cause of the gap
- Not found as a platform gap in op count. The 2157 against 1204 comparison used two different maps.
- Same map (mac_01), console and Azahar differ by 5.4 %.
- Largest term on console: producer appendGfx_model (src/model.c line 1384; address 0x00161328 in lane-final pm_3ds.map line 29598).
  Console: 1696 ops at f2482 (FTP line 19244), 1960 at f2782 (line 24122), 2041 at f3082 (line 28983). That is 79 to 94 % of ops per frame.
  Azahar f299: 1242 of 1489 ops (build3ds/last_log.txt line 5460).
- Not measured: ops per draw, and class counts for mac_01 on Azahar (no opsplit at f299 in that window).
- Open: the 1204 yard baseline cannot be reproduced. Run the yard with phase on to replace it.
