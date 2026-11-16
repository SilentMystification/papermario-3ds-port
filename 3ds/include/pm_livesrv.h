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
