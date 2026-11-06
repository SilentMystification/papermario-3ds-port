# Row LOGNET: console live log missing while the game runs

## Goal
push_3ds.sh to the O3DS XL (192.168.1.131) sent the build and the game ran (user confirmed), but no live log line reached the PC in 240 s, twice. Earlier pushes today worked. Make the cause visible and fix the side that is ours.

## Oracle plan (2026-11-06, Opus)
Connect: push_3ds.sh sends with `3dslink -a $ip ... livelog`, then 3ds/tools/livelog.py connects to $ip:17492. Game side: 3ds/src/pm_log.c pm_log_init: socInit, bind and listen on 17492; a failure is kept in live_msg and written to log.txt on the SD. livelog.py swallows every connect error (OSError -> sleep 1), so refused and timeout look the same. The console IP changes (pushed_builds.txt .170, .224, .131; push-o skill says .168).
Causes: 1 game side socInit or bind failed (soc buffer alloc, errno): game runs, no listener (connection refused). 2 network: AP client isolation, or the console leaves or changes Wi-Fi after the launch (timeouts). 3 writer thread: connected but no bytes (livelog says "silent").
Rows (one commit each):
LOGNET-1 livelog.py prints the connect error class (refused, timeout, unreachable) once per 10 s and at the end ("NO LOG: last error <class>"). pm_log.c: the live log init result (socInit code, bind errno) also goes to the first live line and to log.txt. Host check: run livelog.py against a closed local port and against a dead IP, the two classes print.
LOGNET-2 push_3ds.sh: before the send, `ping -n 1` the IP; after a NO LOG, try one TCP connect to 17492 and print its class. push-o and push-n skill files and go.ps1: drop the fixed default IPs (take --ip or the 3dslink broadcast result). 
LOGNET-3 (needs the user, the orchestrator runs it): one push; the class printed names cause 1, 2 or 3. Cause 1: fix pm_log_init from the logged code (soc buffer size or alignment, bind retry after a short sleep). Cause 2: user side (DHCP reservation, AP). Cause 3: writer core or priority in pm_log.c.
Gate: push shows `[livelog] connected` and the boot line within 30 s, twice in a row.

## Rules
Files: 3ds/tools/livelog.py, 3ds/push_3ds.sh, 3ds/src/pm_log.c (init lines only), .claude skill files push-o/push-n only for the IP default, 3ds/go.ps1. Main folder, branch nds1, lane log. No console push by the agent (the orchestrator pushes). Pathspec commits, own hunks only. No git add -A, commit -a, reset, stash, checkout, restore. No push, no attribution. Never commit 3ds/src/map_restore_table.c. Hand back after LOGNET-2.
