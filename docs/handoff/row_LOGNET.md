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

## Oracle plan for LOGNET-3 (2026-11-06, Opus; console result: one connect accepted by the soc module, 0 bytes, then every connect times out while ping replies; the game plays; the push had loglevel=debug texstale waterlog)
pm_log.c: listen backlog 1; accept_client() runs only in writer_main (prio 0x3F, core 1, wakes every 250 ms) inside `if (log_file_on) { accept_client(); read_cmd(); }` before drain_locked(); accept_client does accept, O_NONBLOCK, ring_replay. 0 bytes = accept() never ran: the handshake sits in the backlog, which stays full after the PC closes, so new SYNs are dropped (timeout, not refused).
Cause 1 (high): drain_locked never returns under a debug flood: the game thread on core 0 refills the queue faster than the writer empties it (fb_put waits on fsink_lock and writes the SD itself when fb is full), so the writer never comes back to accept_client. Fits: prof=4 with few lines works, debug floods fail. Cause 2: writer blocked on fsink_lock during a long SD write (fb_write_locked, 1 to 7 s fsync). Cause 3: writer starved on core 1. Cause 4: pm_io_quiet or log_file_on stuck.
Orchestrator decision: apply the cause-1 fix directly (the oracle names it safe for all 4 causes), then gate on the console.
Fix (pm_log.c only): bound drain_locked to at most SLOTS/4 slots per call; in writer_main call accept_client() and read_cmd() before every bounded drain and outside the `log_file_on` test; listen(listen_fd, 2); in accept_client close an existing client first when a new one comes. Cold path, no per-line cost. Add a `logstat` debug line every 5 s (writer heartbeat, queue fill of 1024, fb waits, dropped).
Gate (orchestrator console push): push with `loglevel=debug texstale waterlog`: `[livelog] connected` and lines within 30 s, steady through 240 s; a second connect after the PC side drops is accepted; logstat shows drops counted, not a hang. If it still fails: the logstat lines in log.txt (pull-o) name cause 2 (fb waits: fb_put drops the file copy into a counted overflow instead of waiting) or 3 (writer prio 0x30).
