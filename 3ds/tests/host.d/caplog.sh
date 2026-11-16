#!/bin/sh
# CAPLOG: the writer serves the live socket with logging off (pm_live_serve) and a new client gets the #PMLIVE line first.
# The seed (serve only with the queue on, the old rule) must fail "serve logging off".
mkdir -p /tmp/caplog_seed
sed 's/return qon || listening;/return qon;/' 3ds/include/pm_livesrv.h > /tmp/caplog_seed/pm_livesrv.h
gcc -m32 -w -I /tmp/caplog_seed -o /tmp/caplog_seed/t 3ds/tools/pm_livesrv_test.c || exit 1
got=$(/tmp/caplog_seed/t); if [ "$got" != "caplog FAIL serve logging off" ]; then echo "caplog seed FAIL: got [$got]"; exit 1; fi
grep -qF "if (pm_live_serve(qon(), listen_fd >= 0)) LightEvent_WaitTimeout" 3ds/src/pm_log.c || { echo "caplog FAIL: writer wait"; exit 1; }
grep -qF "int hn = pm_live_hello(hb, sizeof hb, log_file_on, lv_live, lv_disk, prof_mode);" 3ds/src/pm_log.c || { echo "caplog FAIL: no hello"; exit 1; }
grep -qF "CAPLOG: take every waiting connection" 3ds/src/pm_log.c || { echo "caplog FAIL: no accept drain"; exit 1; }
grep -qF "CAPLOG: accept, read the command and answer sendlog with logging off" 3ds/src/pm_log.c || { echo "caplog FAIL: no serve pass"; exit 1; }
gcc -m32 -Wall -Wextra -I 3ds/include -o /tmp/caplog_t 3ds/tools/pm_livesrv_test.c && /tmp/caplog_t || exit 1
