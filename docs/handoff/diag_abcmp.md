# ABCMP: display list refactor and console frame time
Investigation only. No source edit. [M] measured, [E] estimate, [U] not logged.
## 1. Console logs
- build3ds/hw_live_log_170.txt: build 1433, commit b5c7b5226. Debug. Maps hos_05, a0 m6, a0 m10, kmr_11. No push line in pushed_builds.txt.
- build3ds/ftp_170/hw_o3ds_log.txt: build 29f504d7b, before LOGCLEAN. No boot line. Maps mac_01, a1 m3, kmr_11, a0 m10, a0 m6.
- build3ds/hw_pull/hw_o3ds_log.txt: build 1227-dc (commit 91b225755). Debug. Maps hos_05 only (f317-1198).
- build3ds/hw_o3ds_log.txt: build 1130. hos_05 f320-327 only.
- build3ds/hw_live_log_131.txt: build 1088. Boot lines only.
- Not console: build3ds/last_log.txt and 3ds/logs/perf/*.txt (Azahar).
- pushed_builds.txt has PUSH OK lines only for 1073 and 1083.
- Map ids: area 1 map 2 = mac_01. Area 0 map 11 = kmr_20 (yard). Area 0 map 9 = kmr_11. Area 5 map 5 = hos_05. No console log holds kmr_20.
- diag_frame_budget.md and diag_dc_console.md call the 1227 window "yard". It is hos_05, averaging 78.3 ms (f1157-1183).
## 2. Pairs (window averages)
Pair A (hos_05): old 1227-dc f462-521 (one window); new 1433 f409-588 (three windows).
Pair B (kmr_11): old 29f504d7b f3572-3632; new 1433 f2013-2372 (four windows).
| metric | A old | A new | B old | B new |
|---|---|---|---|---|
| wall ms, pace cap [M] | 33.4 | 33.4 | 33.2 | 33.4 |
| cpu busy ms [M] | 26.5 | 20.5 | 29.9 | 25.7 |
| gbi ms [M] | 16.7 | 12.8 | 17.2 | 16.6 |
| gbi ops/frame [M] | 1132 | 631 | 1165 | 1162 |
| draws/frame [M] | 104 | 69 | 32 | 28 |
| flush calls/frame [M] | 104 | 69 | 32 | 28 |
| tex ops/frame [M] | 96 | 60 | 34 | 37 |
| tex bind/frame | [U] | [U] | [U] | [U] |
| log lines/frame [M] | 7.1 | 0.6 | 10.3 | 0.6 |
| loglevel | debug | debug | debug | debug |
| camera region | [U] | [U] | [U] | [U] |
- Pair A: same map, loglevel and pace state. Camera not logged. Old side is one window. Conditional. Spans every change since 1227 (refactor rows, texture LRU, SD removal, census, LOGCLEAN, DS1).
- Pair B: same map and loglevel. Camera not logged. Both builds have the refactor. Commits between them add LOGCLEAN and DS1. Not a logging-only test.
- No valid pair for yard (kmr_20) or Toad Town (mac_01).
## 3. Change
- Pair A [M]: cpu busy -6.0 ms (-23 %). gbi -3.9 ms (-23 %). gbi ops -501 (-44 %). Wall 0.0 ms.
- Pair B [M]: ops 0 %. cpu -4.2 ms (-14 %). The cpu drop is not from ops.
- Split [E]: if pair A has the pair B non-op drop, about 4 ms of 6.0 ms is logging or DS1, about 2 ms is ops. One scene each. Not proven.
## 4. Answer
- Wall frame time on console: unproven. The paced walk stays at 33.4 ms in both builds.
- CPU busy time: down 6.0 ms (23 %) in one hos_05 window [M]. Conditional.
## 5. Procedure (Git Bash)
1. Copy 3ds/logs/builds/pm_3ds_merge_60e5a06e2.3dsx to build3ds/pm_3ds.3dsx. Save the current file first. Build 60e5a06e2 has none of the refactor rows, LOGCLEAN or DS1.
2. PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 -t 60 audiostat texstat pacestat logstat phase phsum loglevel=debug preset=yard
3. Stand Mario still at the yard start for 60 s.
4. Read build3ds/hw_live_log_170.txt: pace, phase W, phase count, logstat. Skip frames 0-299.
5. Copy 3ds/logs/builds/pm_3ds_final3_b5c7b5226.3dsx to build3ds/pm_3ds.3dsx. Repeat steps 2 to 4.
6. Add both pushes to 3ds/logs/perf/pushed_builds.txt.
Caveat: no build logs position. preset=yard comes from the Azahar scenario [I].
