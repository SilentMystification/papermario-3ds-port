# ws_CAPLOG code (exact text; the goal page is ws_CAPLOG.md)
## A. 3ds/include/pm_livesrv.h (new, LF endings)
```c
#ifndef PM_LIVESRV_H
#define PM_LIVESRV_H
#include <stdio.h>
/* CAPLOG: the writer thread serves the live socket (accept, the sendlog command) when the log queue is on or a listen socket exists. */
static inline int pm_live_serve(int qon, int listening) { return qon || listening; }
/* CAPLOG: the first line a new live client gets: the log gate and the levels of this run (read by pm_logrecv.py and livelog.py). */
static inline int pm_live_hello(char* out, unsigned n, int log_on, int live, int disk, int prof) {
    return snprintf(out, n, "#PMLIVE log_on %d live %d disk %d prof %d\n", log_on, live, disk, prof);
}
#endif
```
## B. pm_log.c accept_client, after `    if (fd < 0) return;` (line 164)
```c
    for (;;) { /* CAPLOG: take every waiting connection, keep the newest: dead PC connections never fill the backlog (listen 2) */
        len = sizeof(addr);
        int nx = accept(listen_fd, (struct sockaddr*)&addr, &len);
        if (nx < 0) break;
        close(fd);
        fd = nx;
    }
```
## C. pm_log.c accept_client, after `    live_sent = ring_pos - ring_len; /* LOGX1: ... */` (line 171)
```c
    char hb[96];
    int hn = pm_live_hello(hb, sizeof hb, log_file_on, lv_live, lv_disk, prof_mode);
    if (hn > 0) send(client, hb, (size_t)hn, 0); /* CAPLOG: the client sees the log gate at once (log_on 0 and prof 0: no line follows) */
```
## D. pm_log.c writer_main
Line 1039 before: `        if (qon()) LightEvent_WaitTimeout(&ev_wake, 250000000LL);`
Line 1039 after: `        if (pm_live_serve(qon(), listen_fd >= 0)) LightEvent_WaitTimeout(&ev_wake, 250000000LL); /* CAPLOG: a listen socket keeps the 250 ms pass with logging off */`
Line 1043 before: `        if (!qon() && q_head == q_tail && !io_drain_req && !__atomic_load_n(&writer_stop, __ATOMIC_ACQUIRE) && !__atomic_load_n(&crashed, __ATOMIC_ACQUIRE)) continue;`
Line 1043 after (the same test, `continue;` becomes this block):
```c
        if (!qon() && q_head == q_tail && !io_drain_req && !__atomic_load_n(&writer_stop, __ATOMIC_ACQUIRE) && !__atomic_load_n(&crashed, __ATOMIC_ACQUIRE)) {
            if (listen_fd >= 0 && __atomic_load_n(&writer_on, __ATOMIC_ACQUIRE) && !pm_io_quiet) { /* CAPLOG: accept, read the command and answer sendlog with logging off */
                accept_client();
                read_cmd();
                LightLock_Lock(&drain_lock);
                if (dump_req && client >= 0) { dump_req = 0; if (log_fd >= 0) fsync(log_fd); send_file("crash.txt"); send_file("log.txt"); } /* LOGX2 */
                LightLock_Unlock(&drain_lock);
            }
            continue;
        }
```
## E. 3ds/tools/pm_livesrv_test.c (new)
```c
/* CAPLOG host test: pm_live_serve and pm_live_hello (pm_livesrv.h). */
#include <stdio.h>
#include <string.h>
#include "pm_livesrv.h"

int main(void) {
    char b[96];
    if (!pm_live_serve(0, 1)) { printf("caplog FAIL serve logging off\n"); return 1; }
    if (pm_live_serve(0, 0)) { printf("caplog FAIL serve no socket\n"); return 1; }
    if (!pm_live_serve(1, 0)) { printf("caplog FAIL serve queue on\n"); return 1; }
    pm_live_hello(b, sizeof b, 0, 3, 3, 0);
    if (strcmp(b, "#PMLIVE log_on 0 live 3 disk 3 prof 0\n")) { printf("caplog FAIL hello [%s]\n", b); return 1; }
    printf("caplog ok\n");
    return 0;
}
```
## F. 3ds/tests/host.d/caplog.sh (new, LF endings)
```sh
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
```
