#include <3ds.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "pm_port.h"

#define RING 32768

static int log_fd = -1;
static int listen_fd = -1;
static int client = -1;
static unsigned seq;
static char ring[RING];
static unsigned ring_pos;
static unsigned ring_len;
static char switches[256];

/* L01 measurement: lines and bytes logged, and a per call site count (by format string address). */
#define SITES 96
static unsigned st_lines, st_bytes, st_prev_lines, st_prev_bytes;
static const char* st_fmt[SITES];
static unsigned st_cnt[SITES];
static int st_nsites;
static LightLock con_lock; /* the bottom screen console (stdout) is not thread safe: writer, HUD and console setup take this */
static int st_on;
static int writer_on;
static volatile int writer_core = -1;
static volatile u32 writer_hb;
static u32 q_dropped, q_drop_total;
static int sw_lognull;
static int log_level = 1; /* L02: 0 error, 1 warn, 2 info, 3 debug. A line above this level is dropped before any formatting. Default warn. */
static int log_file_on = 1; /* L02: 0 = no write to log.txt (switch log=off, options panel) */

static void ring_add(const char* s, int n) {
    for (int i = 0; i < n; i++) ring[ring_pos++ % RING] = s[i];
    if (ring_len + (unsigned)n < RING) ring_len += (unsigned)n;
    else ring_len = RING;
}

static void ring_replay(int fd) {
    unsigned n = ring_len;
    unsigned start = ring_len < RING ? 0 : (ring_pos % RING);
    unsigned off = 0;
    while (off < n) {
        unsigned idx = (start + off) % RING;
        unsigned chunk = RING - idx;
        if (chunk > n - off) chunk = n - off;
        if (send(fd, ring + idx, chunk, 0) < 0) return;
        off += chunk;
    }
}

static void accept_client(void) {
    if (listen_fd < 0) return;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int fd = accept(listen_fd, (struct sockaddr*)&addr, &len);
    if (fd < 0) return;
    fcntl(fd, F_SETFL, O_NONBLOCK);
    if (client >= 0) close(client);
    client = fd;
    ring_replay(fd);
}

void pm_log_poll(void) {
    if (!__atomic_load_n(&writer_on, __ATOMIC_ACQUIRE)) accept_client(); /* the writer thread accepts once it runs */
}

/* PERF-GBI: a test run has a switch string, and every pm_debug_has call scanned it (4.6 us in Azahar). Real play has none and returns at once. The cache makes a
 * test run cost what real play costs. Entries hold a copy of the word, so a word in a changing buffer is safe. The cache is cleared when the string changes. */
typedef struct { char w[20]; signed char v; } DbgEnt;
static DbgEnt dbg_cache[64];
static void dbg_cache_clear(void) { memset(dbg_cache, 0, sizeof dbg_cache); }
static int debug_scan(const char* word) {
    const char* p = switches;
    size_t n = strlen(word);
    while ((p = strstr(p, word)) != NULL) {
        int edge0 = (p == switches) || p[-1] == ' ' || p[-1] == '\n' || p[-1] == '\t';
        char after = p[n];
        int edge1 = after == 0 || after == ' ' || after == '\n' || after == '\t' || after == '\r';
        if (edge0 && edge1) return 1;
        p += n;
    }
    return 0;
}
int pm_debug_has(const char* word) {
    PH_ADD(C_DBG, 1);
    if (!word || !switches[0]) return 0;
    unsigned h = 5381;
    size_t n = 0;
    for (; word[n] && n < 19; n++) h = h * 33u + (unsigned char)word[n];
    if (word[n]) return debug_scan(word); /* a long word: no cache */
    DbgEnt* e = &dbg_cache[h & 63u];
    if (e->v && strcmp(e->w, word) == 0) return e->v > 0;
    int r = debug_scan(word);
    memcpy(e->w, word, n + 1);
    e->v = r ? 1 : -1;
    return r;
}

const char* pm_debug_text(void) { return switches; }

/* F11-3: add or remove one word at run time (the options panel). Every pm_debug_has call sees it at once. */
static void sw_impl(const char* word, int on) {
    size_t n = strlen(word), len = strlen(switches);
    char* p = switches;
    if (pm_debug_has(word) == (on != 0)) return;
    if (on) {
        if (len + n + 2 < sizeof(switches)) snprintf(switches + len, sizeof(switches) - len, " %s", word);
        return;
    }
    while ((p = strstr(p, word)) != NULL) {
        int edge0 = (p == switches) || p[-1] <= ' ';
        char after = p[n];
        if (edge0 && (unsigned char)after <= ' ') {
            memmove(p, p + n + (after ? 1 : 0), strlen(p + n + (after ? 1 : 0)) + 1);
            return;
        }
        p += n;
    }
}

void pm_log_switch(const char* word, int on) {
    dbg_cache_clear();
    sw_impl(word, on);
    dbg_cache_clear();
}

void pm_log_set_switches(const char* s) {
    dbg_cache_clear();
    switches[0] = 0;
    if (s) strncat(switches, s, sizeof(switches) - 1);
    st_on = pm_debug_has("logstat") ? 1 : 2;
    sw_lognull = pm_debug_has("lognull");
    {   /* L02: loglevel=<0..3 or error, warn, info, debug>, log=off */
        extern int pm_debug_value(const char* key, char* out, unsigned n);
        char v[16];
        if (pm_debug_value("loglevel", v, sizeof v)) {
            int lv = v[0] >= '0' && v[0] <= '3' ? v[0] - '0' : v[0] == 'e' ? 0 : v[0] == 'w' ? 1 : v[0] == 'i' ? 2 : v[0] == 'd' ? 3 : log_level;
            log_level = lv;
        }
        if (pm_debug_value("log", v, sizeof v) && (!strcmp(v, "off") || !strcmp(v, "0"))) log_file_on = 0;
    }
}

int pm_log_level(void) { return log_level; }
int pm_log_file(void) { return log_file_on; }
void pm_log_set_level(int lv) { log_level = lv < 0 ? 0 : lv > 3 ? 3 : lv; }
void pm_log_set_file(int on) { log_file_on = on != 0; }

void pm_log_init(int live) {
    LightLock_Init(&con_lock);
    /* Keep the last run: a crash trace in log.txt must survive the next boot. */
    unlink("log.old");
    int rot = rename("log.txt", "log.old");
    int rot_err = errno;
    log_fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (rot != 0) pm_log_w("log rotate failed errno %d (no previous log.txt on the first boot)", rot_err);
    unsigned soc_bytes = 0;
    u32* soc = (u32*)pm_soc_buffer(&soc_bytes);
    Result soc_rc = (!live || !soc || soc_bytes < 0x1000) ? -1 : socInit(soc, soc_bytes);
    if (soc_rc != 0) {
        if (live) pm_log_w("live log: off (buffer %p, %u bytes, socInit %08lx)", (void*)soc, soc_bytes, (unsigned long)soc_rc);
        return;
    }
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) return;
    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(17492);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0 || listen(listen_fd, 1) < 0) {
        close(listen_fd);
        listen_fd = -1;
        pm_log_e("live log: bind 17492 failed\n");
        return;
    }
    fcntl(listen_fd, F_SETFL, O_NONBLOCK);
    pm_log_w("live log: port 17492\n");
}

static void stat_site(const char* fmt) {
    int i;
    for (i = 0; i < st_nsites; i++) if (st_fmt[i] == fmt) { st_cnt[i]++; return; }
    if (st_nsites < SITES) { st_fmt[st_nsites] = fmt; st_cnt[st_nsites++] = 1; }
}

/* Called by the frame loop every 300 drawn frames when the logstat switch is on. */
void pm_log_stat(float avg_ms, unsigned frames, float secs) {
    unsigned all = st_lines + q_drop_total + q_dropped;
    unsigned dl = all - st_prev_lines, db = st_bytes - st_prev_bytes;
    st_prev_lines = all;
    st_prev_bytes = st_bytes;
    /* Also to the debugger channel (the Azahar log): under logflood the queue is full and drops the line below. */
    char dbg[160];
    int dn = snprintf(dbg, sizeof(dbg), "logstat frames %u work %.2f ms wall %.2f ms lines %u (%.1f/frame %.0f/s)", frames, avg_ms,
                      secs * 1000.f / (float)frames, dl, (float)dl / (float)frames, (float)dl / secs);
    svcOutputDebugString(dbg, dn);
    pm_log_d("logstat frames %u work %.2f ms wall %.2f ms lines %u (%.1f/frame %.0f/s) bytes %u total %u writer on %d core %d hb %u dropped %u", frames, avg_ms, secs * 1000.f / (float)frames, dl,
           (float)dl / (float)frames, (float)dl / secs, db, all, writer_on, writer_core, writer_hb, q_drop_total + q_dropped);
}

void pm_log_top(void) {
    for (int k = 0; k < 8; k++) {
        int best = -1;
        for (int i = 0; i < st_nsites; i++) if (st_cnt[i] && (best < 0 || st_cnt[i] > st_cnt[best])) best = i;
        if (best < 0) return;
        pm_log_d("logtop %u x \"%.60s\"", st_cnt[best], st_fmt[best]);
        st_cnt[best] = 0;
    }
}

/* ---- L01: non-blocking log ----
 * Producers (game thread, audio thread) format the line and copy it into a slot of a lock-free queue. They make no
 * SD, socket or stdout call. One low-priority writer thread (core 1, or core 0 when core 1 is not available) does
 * write, fsync about once per second, socket send and stdout. Full queue: the new line is dropped and counted.
 * Before the writer thread runs, pm_log writes directly (boot lines). */
#define SLOTS 512
#define SLOT_TEXT 184
typedef struct {
    u32 state; /* 0 free, 1 committed (release store by the producer, acquire load by the writer) */
    u16 len;
    u16 pad;
    char text[SLOT_TEXT];
} Slot;
static Slot slots[SLOTS];
static u32 q_head;    /* next slot to reserve (producers, compare and swap) */
static u32 q_tail;    /* next slot to read (writer or crash flush, under drain_lock) */
static int writer_stop;
static int crashed;
static int active_id;
static int dirty;
static int writer_started; /* id of the writer thread that has run its first line */
static Thread writer_th;
static LightLock drain_lock;

void pm_con_lock(void) { LightLock_Lock(&con_lock); }
int pm_con_trylock(void) { return LightLock_TryLock(&con_lock); }
void pm_con_unlock(void) { LightLock_Unlock(&con_lock); }
static LightEvent ev_wake, ev_started, ev_flushed;

static void direct_write(const char* line, int n, const char* body, size_t bl) {
    if (log_fd >= 0 && log_file_on) {
        if (write(log_fd, line, (size_t)n) > 0) fsync(log_fd);
    }
    ring_add(line, n);
    if (client >= 0 && send(client, line, (size_t)n, 0) < 0) {
        close(client);
        client = -1;
    }
    if (!pm_menu_open) { fwrite(body, 1, bl, stdout); fflush(stdout); }
}

static void vlog(int lv, const char* fmt, va_list ap) {
    if (lv > log_level) return;
    int on = __atomic_load_n(&writer_on, __ATOMIC_ACQUIRE);
    /* Queue full: one atomic add and out, before any formatting work, so a flood costs almost nothing. */
    if (on && __atomic_load_n(&q_head, __ATOMIC_RELAXED) - __atomic_load_n(&q_tail, __ATOMIC_ACQUIRE) >= SLOTS) {
        __atomic_fetch_add(&q_dropped, 1, __ATOMIC_RELAXED);
        return;
    }
    __atomic_fetch_add(&st_lines, 1, __ATOMIC_RELAXED);
    if (st_on == 1 && fmt[0] != 'l') stat_site(fmt);
    char body[240];
    vsnprintf(body, sizeof(body), fmt, ap);
    size_t bl = strlen(body);
    if (bl == 0 || body[bl - 1] != '\n') {
        if (bl + 1 < sizeof(body)) {
            body[bl++] = '\n';
            body[bl] = 0;
        }
    }
    __atomic_fetch_add(&st_bytes, (unsigned)bl + 8, __ATOMIC_RELAXED);
    /* lognull: measurement switch, drops every line that is not an error. */
    if (sw_lognull && !strstr(body, "CRASH") && !strstr(body, "FATAL") && !strstr(body, "logstat")) return;
    if (!on) {
        char line[280];
        int n = snprintf(line, sizeof(line), "#%u %s", __atomic_fetch_add(&seq, 1, __ATOMIC_RELAXED), body);
        if (n < 0) return;
        if (n >= (int)sizeof(line)) n = (int)sizeof(line) - 1;
        direct_write(line, n, body, bl);
        return;
    }
    u32 h = __atomic_load_n(&q_head, __ATOMIC_RELAXED);
    for (;;) {
        if (h - __atomic_load_n(&q_tail, __ATOMIC_ACQUIRE) >= SLOTS) {
            __atomic_fetch_add(&q_dropped, 1, __ATOMIC_RELAXED);
            return;
        }
        if (__atomic_compare_exchange_n(&q_head, &h, h + 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) break;
    }
    /* The number is taken after the slot, so the queue order is the number order. */
    Slot* sl = &slots[h % SLOTS];
    int n = snprintf(sl->text, SLOT_TEXT, "#%u %s", __atomic_fetch_add(&seq, 1, __ATOMIC_RELAXED), body);
    if (n < 0) n = 0;
    if (n >= SLOT_TEXT) {
        n = SLOT_TEXT - 1; /* long line cut to the slot, keep the newline */
        sl->text[n - 1] = '\n';
    }
    sl->len = (u16)n;
    __atomic_store_n(&sl->state, 1, __ATOMIC_RELEASE);
    /* Wake the writer when the queue was empty or is half full. Otherwise its 250 ms timeout collects the lines. */
    u32 t = __atomic_load_n(&q_tail, __ATOMIC_RELAXED);
    if (h == t || h - t == SLOTS / 2) LightEvent_Signal(&ev_wake);
}

#define PM_LOG_FN(name, lv) void name(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog(lv, fmt, ap); va_end(ap); }
PM_LOG_FN(pm_log_e, 0)
PM_LOG_FN(pm_log_w, 1)
PM_LOG_FN(pm_log_i, 2)
PM_LOG_FN(pm_log_d, 3)
PM_LOG_FN(pm_log, 2) /* an old call is info */

/* Output of one batch: log.txt, replay ring, live socket, stdout. Writer thread (or crash flush) only. */
static void out_batch(const char* lines, int n, const char* bodies, int bn) {
    if (n <= 0) return;
    if (log_fd >= 0 && log_file_on && write(log_fd, lines, (size_t)n) > 0) dirty = 1;
    ring_add(lines, n);
    if (client >= 0) {
        int off = 0;
        while (off < n) {
            int r = send(client, lines + off, (size_t)(n - off), 0);
            if (r > 0) {
                off += r;
                continue;
            }
            if (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                close(client);
                client = -1;
            }
            break; /* a full socket drops the rest for the live view only, log.txt has it */
        }
    }
    /* After a crash the lock owner may be the dead thread: then skip the console. */
    if (__atomic_load_n(&crashed, __ATOMIC_ACQUIRE) ? LightLock_TryLock(&con_lock) == 0 : (LightLock_Lock(&con_lock), 1)) {
        if (!pm_menu_open) { fwrite(bodies, 1, (size_t)bn, stdout); fflush(stdout); } /* F11-6: the menu owns the bottom screen */
        LightLock_Unlock(&con_lock);
    }
}

/* Report lines that producers dropped because the queue was full (writer or crash flush). */
static void report_drops(void) {
    u32 d = __atomic_exchange_n(&q_dropped, 0, __ATOMIC_RELAXED);
    if (!d) return;
    q_drop_total += d;
    char m[80];
    int mn = snprintf(m, sizeof(m), "#%u log: %u lines dropped (queue full)\n", __atomic_fetch_add(&seq, 1, __ATOMIC_RELAXED), d);
    out_batch(m, mn, m, mn);
}

/* The caller holds drain_lock. */
static void drain_locked(void) {
    static char lb[4096], bb[4096];
    int ln = 0, bn = 0;
    u32 t = q_tail;
    for (;;) {
        Slot* sl = &slots[t % SLOTS];
        if (__atomic_load_n(&sl->state, __ATOMIC_ACQUIRE) != 1) break;
        int n = sl->len;
        const char* sp = (const char*)memchr(sl->text, ' ', (size_t)n);
        int skip = sp ? (int)(sp - sl->text) + 1 : 0;
        if (ln + n > (int)sizeof(lb) || bn + n - skip > (int)sizeof(bb)) {
            out_batch(lb, ln, bb, bn);
            ln = bn = 0;
            report_drops(); /* a flood keeps the queue full, so report between batches */
        }
        memcpy(lb + ln, sl->text, (size_t)n);
        ln += n;
        memcpy(bb + bn, sl->text + skip, (size_t)(n - skip));
        bn += n - skip;
        __atomic_store_n(&sl->state, 0, __ATOMIC_RELEASE);
        t++;
        __atomic_store_n(&q_tail, t, __ATOMIC_RELEASE);
    }
    out_batch(lb, ln, bb, bn);
    report_drops();
}

static void writer_main(void* arg) {
    int id = (int)(intptr_t)arg;
    writer_core = svcGetProcessorID();
    __atomic_store_n(&writer_started, id, __ATOMIC_RELEASE);
    LightEvent_Signal(&ev_started);
    u64 last_sync = svcGetSystemTick();
    for (;;) {
        LightEvent_WaitTimeout(&ev_wake, 250000000LL);
        if (__atomic_load_n(&active_id, __ATOMIC_ACQUIRE) != id) return; /* a replacement thread took over */
        writer_hb++;
        if (!__atomic_load_n(&writer_on, __ATOMIC_ACQUIRE)) continue;
        accept_client();
        LightLock_Lock(&drain_lock);
        if (!__atomic_load_n(&crashed, __ATOMIC_ACQUIRE)) drain_locked();
        LightLock_Unlock(&drain_lock);
        if (__atomic_load_n(&crashed, __ATOMIC_ACQUIRE)) LightEvent_Signal(&ev_flushed);
        u64 now = svcGetSystemTick();
        int stop = __atomic_load_n(&writer_stop, __ATOMIC_ACQUIRE);
        if (dirty && log_fd >= 0 && (stop || now - last_sync >= SYSCLOCK_ARM11)) {
            fsync(log_fd);
            dirty = 0;
            last_sync = now;
        }
        if (stop) return;
    }
}

int pm_log_writer_core(void) { return writer_core; }
unsigned pm_log_writer_hb(void) { return writer_hb; }
unsigned pm_log_dropped(void) { return q_drop_total + q_dropped; }

static int start_writer(int id, int core) {
    LightEvent_Clear(&ev_started);
    __atomic_store_n(&active_id, id, __ATOMIC_RELEASE);
    writer_th = threadCreate(writer_main, (void*)(intptr_t)id, 32 * 1024, 0x3F, core, false);
    if (!writer_th) return 0;
    u64 t0 = svcGetSystemTick();
    int to = LightEvent_WaitTimeout(&ev_started, 1000000000LL);
    unsigned waited_ms = (unsigned)((svcGetSystemTick() - t0) / (SYSCLOCK_ARM11 / 1000));
    int ran = __atomic_load_n(&writer_started, __ATOMIC_ACQUIRE) == id;
    pm_log_d("log: writer id %d core %d: wait result %d, thread ran %d after %u ms, svcGetProcessorID in thread %d", id, core, to, ran, waited_ms, writer_core);
    return ran;
}

/* Start the writer thread. Call once, after pm_log_init. */
void pm_log_start(void) {
    if (writer_on || writer_th) return;
    LightEvent_Init(&ev_wake, RESET_ONESHOT);
    LightEvent_Init(&ev_started, RESET_ONESHOT);
    LightEvent_Init(&ev_flushed, RESET_ONESHOT);
    LightLock_Init(&drain_lock);
    /* Core 1 (system core): a thread there is allowed only after APT_SetAppCpuTimeLimit (libctru thread.h). */
    /* AU1d: the audio thread runs on core 1 too. Switch cpulimit=<30..80> sets the share (default 30 as before; AU1e tries 50 and 80 on the O3DS). */
    int lim = 30;
    { char v[16]; if (pm_debug_value("cpulimit", v, sizeof v)) lim = atoi(v); if (lim < 5 || lim > 80) lim = 30; }
    Result r = APT_SetAppCpuTimeLimit(lim);
    pm_log_d("log: APT_SetAppCpuTimeLimit(%d) = %08lx", lim, (unsigned long)r);
    int ok = start_writer(1, 1);
    if (!ok) {
        pm_log_w("log: writer thread on core 1 %s, using core 0", writer_th ? "did not start" : "not created");
        writer_core = -1;
        ok = start_writer(2, 0);
    }
    if (!ok) {
        pm_log_e("log: writer thread failed, the log stays direct");
        __atomic_store_n(&active_id, 0, __ATOMIC_RELEASE);
        return;
    }
    pm_log_d("log: writer thread on core %d, game thread on core %d", writer_core, (int)svcGetProcessorID());
    __atomic_store_n(&writer_on, 1, __ATOMIC_RELEASE);
}

/* Normal exit: stop the writer after it has written everything. */
void pm_log_stop(void) {
    if (!writer_th) return;
    __atomic_store_n(&writer_stop, 1, __ATOMIC_RELEASE);
    LightEvent_Signal(&ev_wake);
    threadJoin(writer_th, 2000000000ULL);
    __atomic_store_n(&writer_on, 0, __ATOMIC_RELEASE);
    LightLock_Lock(&drain_lock);
    drain_locked(); /* the writer may have stopped on the timeout */
    LightLock_Unlock(&drain_lock);
    if (log_fd >= 0) fsync(log_fd);
}

/* Crash handler: write everything in the queue now, in the calling thread. */
static void crash_flush(void) {
    if (!__atomic_load_n(&writer_on, __ATOMIC_ACQUIRE)) return;
    __atomic_store_n(&crashed, 1, __ATOMIC_RELEASE);
    int got = LightLock_TryLock(&drain_lock) == 0;
    if (!got) {
        LightEvent_WaitTimeout(&ev_flushed, 500000000LL); /* the writer is in a batch, let it finish */
        got = LightLock_TryLock(&drain_lock) == 0;
    }
    if (got) {
        drain_locked(); /* the lock stays taken: the writer must not run again */
        if (log_fd >= 0) fsync(log_fd);
    }
    __atomic_store_n(&writer_on, 0, __ATOMIC_RELEASE); /* later lines go direct */
}

void pm_assert_fail(const char* statement) {
    static const char* seen[8];
    static int nseen;
    int i;
    if (!statement) statement = "assert";
    for (i = 0; i < nseen; i++) if (seen[i] == statement) return;
    if (nseen < 8) seen[nseen++] = statement;
    pm_log_e("assert %s", statement);
}

void pm_crash_line(const char* s, int n) {
    static int crash_fd = -1;
    static int flushed;
    if (!flushed) {
        flushed = 1;
        crash_flush();
    }
    svcOutputDebugString(s, n);
    if (log_fd >= 0) {
        write(log_fd, s, (size_t)n);
        fsync(log_fd);
    }
    /* crash.txt keeps every crash (append), so it stays after the next boot. */
    if (crash_fd < 0) crash_fd = open("crash.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (crash_fd >= 0) {
        write(crash_fd, s, (size_t)n);
        fsync(crash_fd);
    }
    if (client >= 0) send(client, s, (size_t)n, MSG_DONTWAIT);
}
