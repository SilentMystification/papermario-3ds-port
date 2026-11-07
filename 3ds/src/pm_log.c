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
#include "pm_logfmt.h"

#define RING 32768
/* PI6: bytes waiting for log.txt (fb) and for the bottom console (cb). The writer copies into them, the sink thread writes them out.
 * fb_head and fb_tail count bytes (never wrap): index = count & (FB_SIZE - 1). Both are written under fb_lock. */
#define FB_SIZE (128u * 1024u)
#define CB_SIZE 8192u
static char fb[FB_SIZE];
static unsigned fb_head, fb_tail;
static char cb[CB_SIZE];
static unsigned cb_n;
static LightLock fb_lock;    /* fb, fb_head, fb_tail, cb, cb_n */
static LightEvent ev_sink; /* LOG4: the sink thread waits here while logging is off */
static u32 fb_drop_lines; /* LOG4: lines of log.txt dropped by a full fb (fb_put), beat line fbdrop */
static LightLock fsink_lock; /* every write and fsync of log_fd (sink thread, writer when fb is full, crash, stop) */

static int log_fd = -1;
static int listen_fd = -1;
static int soc_up;
static char live_msg[160]; /* LIVEMSG: the live-log status line, see pm_log_live_status */
static u32 live_eagain, live_stall_close;
static u64 live_ok_tick;
static int client = -1;
static unsigned seq;
static char ring[RING];
static unsigned ring_pos;
static unsigned ring_len;
static char switches[256];

/* L01 measurement: lines and bytes logged. */
static unsigned st_lines, st_bytes, st_prev_lines, st_prev_bytes;
static int con_up; /* the bottom console exists: the sink thread prints cb (lines logged before that wait in cb) */
static LightLock con_lock; /* the bottom screen console (stdout) is not thread safe: writer, HUD and console setup take this */
static int writer_on;
volatile int pm_io_quiet; /* PT68: set while the app is suspended or asleep, see io_gate */
static volatile int writer_core = -1;
static volatile u32 writer_hb;
static u32 q_dropped, q_drop_total;
static u32 q_head;    /* next slot to reserve (producers, compare and swap) */
static u32 q_tail;    /* next slot to read (writer or crash flush, under drain_lock) */
static u32 wr_file_max, wr_sock_max, con_skipped; /* HWM1: longest single step of the writer in us, since the last logstat line */
static u32 sink_file_max, sink_con_max; /* PI6: the sink thread's longest write and console step in us */
static int sw_lognull;
/* LOG3: one level per sink: -1 off, 0 error, 1 warn, 2 info, 3 debug. A line reaches a sink when its level is at or below that sink's level. */
static int lv_live = 1; /* the live socket and the ring that replays to a reconnect */
static int lv_disk = 1; /* log.txt (fb) */
static int lv_con = 1; /* the bottom screen console (cb) */
static int log_file_on = 1; /* the logging gate: 0 = off (switch log=off, options panel Logging). No line is formatted, queued, written or sent, and phase timing stops (pm_ph_log_gate). */
static int pass_lv = 1; /* LOG3: the highest level an enabled sink takes: max of lv_live, lv_disk, lv_con; -1 when the queue is off (qon). One compare in vlog. */
static int prof_mode; /* U2: Profiling on (prof, phase): logging is off, only profiling lines pass (live and console, never log.txt) */
static int sv_lv[3]; /* U2: live, disk, con levels saved when Profiling starts, restored when it ends */
static int prof_disk; /* U2 (user decision): profiling lines also go to log.txt when this is on (option prof_disk, switch profdisk=1). Off by default. */
void pm_log_set_prof_disk(int on) { prof_disk = on != 0; }
int pm_log_get_prof_disk(void) { return prof_disk; }
/* U2: the queue takes lines when logging is on or Profiling is on. Logging and Profiling are one choice, so this is one load and one compare. */
static int qon(void) { return log_file_on || prof_mode; }
static u32 cnt_disk, cnt_live, cnt_con; /* LOG3: slot lines each sink took since boot (beat line: sinks disk live con) */
static u32 con_lines; /* CONLV: console lines printed by the sink thread since boot (logstat2 console lines) */
static void pass_recalc(void) {
    int m = lv_live > lv_disk ? lv_live : lv_disk;
    if (lv_con > m) m = lv_con;
    pass_lv = qon() ? m : -1;
}
void pm_ph_log_gate(int on);

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

/* QUIT1: the PC sends one line on the live socket. Only "quit" is a command. The writer thread reads it (non-blocking, one line per
 * connection), pm_log_poll (game thread) acts on it. */
static char cmd_buf[16];
static unsigned cmd_n;
static int cmd_done;
static volatile int quit_req;

static void read_cmd(void) {
    if (client < 0 || cmd_done) return;
    int r = recv(client, cmd_buf + cmd_n, sizeof cmd_buf - 1 - cmd_n, 0);
    if (r <= 0) return; /* EAGAIN: nothing yet. 0 or an error: the send path sees the close */
    cmd_n += (unsigned)r;
    cmd_buf[cmd_n] = 0;
    char* nl = strchr(cmd_buf, '\n');
    if (!nl && cmd_n < sizeof cmd_buf - 1) return; /* the line is not complete */
    cmd_done = 1;
    if (nl) *nl = 0;
    if (!strncmp(cmd_buf, "quit", 4) && (cmd_buf[4] == 0 || cmd_buf[4] == '\r')) quit_req = 1;
}

static void accept_client(void) {
    if (listen_fd < 0) return;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int fd = accept(listen_fd, (struct sockaddr*)&addr, &len);
    if (fd < 0) return;
    if (client >= 0) { close(client); client = -1; } /* a new client replaces the old one (the PC closed it or the old link is dead) */
    fcntl(fd, F_SETFL, O_NONBLOCK);
    client = fd;
    cmd_n = 0;
    cmd_done = 0;
    live_ok_tick = 0;
    ring_replay(fd);
}

extern volatile int pm_exit_request; /* PT28: the main loop ends, pm_shutdown runs, main returns */

void pm_log_poll(void) {
    if (!__atomic_load_n(&writer_on, __ATOMIC_ACQUIRE)) accept_client(); /* the writer thread accepts once it runs */
    if (quit_req) { /* QUIT1: the same exit as the menu row Exit to Homebrew */
        quit_req = 0;
        pm_log_w("quit received");
        pm_exit_request = 1;
    }
}

/* PERF-GBI: a test run has a switch string, and every pm_debug_has call scanned it (4.6 us in Azahar). Real play has none and returns at once. The cache makes a
 * test run cost what real play costs. Entries hold a copy of the word, so a word in a changing buffer is safe. The cache is cleared when the string changes. */
typedef struct { char w[20]; signed char v; unsigned n; } DbgEnt;
static DbgEnt dbg_cache[64];
unsigned pm_dbg_gen = 1; /* changes whenever the switch string changes, for callers that cache a word and must see the options panel */
static void dbg_cache_clear(void) { memset(dbg_cache, 0, sizeof dbg_cache); pm_dbg_gen++; }
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
    if (e->v && strcmp(e->w, word) == 0) { e->n++; return e->v > 0; }
    int r = debug_scan(word);
    memcpy(e->w, word, n + 1);
    e->v = r ? 1 : -1;
    e->n = 1;
    return r;
}

/* PG0: the words most often asked since the last report (switch gbiprof calls this every 300 frames) */
void pm_debug_report(unsigned frames) {
    for (int r = 0; r < 6; r++) {
        int best = -1; unsigned bn = 0;
        for (int i = 0; i < 64; i++) if (dbg_cache[i].v && dbg_cache[i].n > bn) { bn = dbg_cache[i].n; best = i; }
        if (best < 0 || bn < frames) break;
        pm_log("gbiprof dbgword %s per frame %u", dbg_cache[best].w, bn / (frames ? frames : 1));
        dbg_cache[best].n = 0;
    }
    for (int i = 0; i < 64; i++) dbg_cache[i].n = 0;
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

/* LOG3: a level word: 0..3, e error, w warn, i info, d debug; 4, -, o (off) give -1. Other text keeps dflt. */
static int lv_word(const char* v, int dflt) {
    if (v[0] >= '0' && v[0] <= '3') return v[0] - '0';
    if (v[0] == '4' || v[0] == '-' || v[0] == 'o') return -1;
    if (v[0] == 'e') return 0;
    if (v[0] == 'w') return 1;
    if (v[0] == 'i') return 2;
    if (v[0] == 'd') return 3;
    return dflt;
}

/* LOG3: the level switches in this order: loglevel (live and disk), then livelevel, disklevel, conlevel, and log=off.
 * pm_options_init calls it after the options.ini levels, so a switch wins over the saved value. */
void pm_log_apply_switch_levels(void) {
    extern int pm_debug_value(const char* key, char* out, unsigned n);
    char v[16];
    if (pm_debug_value("loglevel", v, sizeof v)) lv_live = lv_disk = lv_word(v, lv_disk);
    if (pm_debug_value("livelevel", v, sizeof v)) lv_live = lv_word(v, lv_live);
    if (pm_debug_value("disklevel", v, sizeof v)) lv_disk = lv_word(v, lv_disk);
    if (pm_debug_value("conlevel", v, sizeof v)) lv_con = lv_word(v, lv_con);
    if (pm_debug_value("profdisk", v, sizeof v)) prof_disk = v[0] == '1'; /* U2: profiling to log.txt, 0 off (default), 1 on */
    if (pm_debug_value("log", v, sizeof v)) log_file_on = !(!strcmp(v, "off") || !strcmp(v, "0")); /* ARG1: log=on sets the gate too */
    pass_recalc();
}

void pm_log_set_switches(const char* s) {
    dbg_cache_clear();
    switches[0] = 0;
    if (s) strncat(switches, s, sizeof(switches) - 1);
    sw_lognull = pm_debug_has("lognull");
    pm_log_apply_switch_levels();
}

/* The higher of the live and disk level (the old single log level). */
int pm_log_level(void) { return lv_disk > lv_live ? lv_disk : lv_live; }
/* The console level, 4 = off (the bottom screen and the boot line). */
int pm_log_conlevel(void) { return lv_con < 0 ? 4 : lv_con; }
/* 1 when the queue takes lines: logging on, or Profiling on (phase timing and the measurement gates read this). */
int pm_log_file(void) { return qon(); }
/* LOG5: the three levels as run (-1 off, 0 error .. 3 debug), and the logging gate (pm_opt.log). */
int pm_log_live_level(void) { return lv_live; }
int pm_log_disk_level(void) { return lv_disk; }
int pm_log_con_level(void) { return lv_con; }
int pm_log_logging(void) { return log_file_on; }
static void wake_writer(void);
/* The queue changed (logging or profiling): the phase gate, the level pass and the sinks follow. */
static void gate_changed(void) {
    pm_ph_log_gate(qon());
    pass_recalc();
    LightEvent_Signal(&ev_sink); /* LOG4: the sink sleeps without a timeout while the queue is off */
    wake_writer(); /* the writer drains the queue, then sleeps while the queue is off */
}
void pm_log_set_file(int on) {
    log_file_on = on != 0;
    gate_changed();
}
/* LOG5: the menu and the options file set the logging gate and the three levels at once. Profiling keeps its own saved levels (pm_log_prof_row). */
void pm_log_set_levels(int on, int live, int disk, int con) {
    log_file_on = on != 0;
    lv_live = live < 0 ? -1 : live > 3 ? 3 : live;
    lv_disk = disk < 0 ? -1 : disk > 3 ? 3 : disk;
    lv_con = con < 0 ? -1 : con > 3 ? 3 : con;
    gate_changed();
}

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
    /* LIVEMSG: pm_log_w drops lines until pm_log_start, so the result is kept here and written by pm_log_live_status. */
    if (!live) {
        snprintf(live_msg, sizeof live_msg, "live log: off (no netload and no livelog switch)");
        return;
    }
    if (soc_rc != 0) {
        snprintf(live_msg, sizeof live_msg, "live log: off (socInit %08lx, buffer %p, %u bytes, alloc %08lx, netload %d)",
                 (unsigned long)soc_rc, (void*)soc, soc_bytes, (unsigned long)pm_soc_alloc_rc(), pm_started_by_netload());
        return;
    }
    soc_up = 1;
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        snprintf(live_msg, sizeof live_msg, "live log: off (socket failed errno %d, socInit %08lx)", errno, (unsigned long)soc_rc);
        return;
    }
    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(17492);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0 || listen(listen_fd, 2) < 0) {
        int bind_errno = errno;
        close(listen_fd);
        listen_fd = -1;
        snprintf(live_msg, sizeof live_msg, "live log: off (bind 17492 or listen failed errno %d, socInit %08lx)", bind_errno, (unsigned long)soc_rc);
        return;
    }
    fcntl(listen_fd, F_SETFL, O_NONBLOCK);
    snprintf(live_msg, sizeof live_msg, "live log: port 17492 (buffer %p, %u bytes, alloc %08lx, socInit %08lx)", (void*)soc, soc_bytes,
             (unsigned long)pm_soc_alloc_rc(), (unsigned long)soc_rc);
}

/* LIVEMSG: write the live-log status from pm_log_init. Call after pm_log_start, when the writer takes lines. */
void pm_log_live_status(void) {
    if (live_msg[0]) pm_log_w("%s", live_msg);
}

/* Called by the frame loop every 300 drawn frames when the logstat switch is on. */
/* CONCOL test aid, switch lvtest: one line per level at frame 100, the marker `lvtest: done` at frame 125. The colour check reads the bottom shots between them. */
void pm_log_lvtest(unsigned frame) {
    if (frame != 100 && frame != 125) return;
    if (!pm_debug_has("lvtest")) return;
    if (frame == 100) {
        pm_log_e("lvtest: error line");
        pm_log_w("lvtest: warn line");
        pm_log_i("lvtest: info line");
        pm_log_d("lvtest: debug line");
    } else pm_log("lvtest: done");
}

void pm_log_stat(float avg_ms, unsigned frames, float secs, unsigned spikes, float spike_max) {
    unsigned all = st_lines + q_drop_total + q_dropped;
    unsigned dl = all - st_prev_lines, db = st_bytes - st_prev_bytes;
    st_prev_lines = all;
    st_prev_bytes = st_bytes;
    pm_log_d("logstat frames %u work %.2f ms wall %.2f ms lines %u (%.1f/frame %.0f/s) bytes %u total %u writer on %d core %d hb %u dropped %u live eagain %u stall closes %u spikes %u max %.1f ms", frames, avg_ms, secs * 1000.f / (float)frames, dl,
           (float)dl / (float)frames, (float)dl / secs, db, all, writer_on, writer_core, writer_hb, q_drop_total + q_dropped, live_eagain, live_stall_close, spikes, spike_max);
    pm_log_d("logstat2 writer max us fbuf %u sock %u | sink max us file %u console %u, console skipped %u, console lines %u, queue %u of %u, fbuf %u of %u, fbuf dropped lines %u",
           wr_file_max, wr_sock_max, sink_file_max, sink_con_max, con_skipped, con_lines, __atomic_load_n(&q_head, __ATOMIC_RELAXED) - __atomic_load_n(&q_tail, __ATOMIC_RELAXED), 1024u,
           fb_head - fb_tail, FB_SIZE, fb_drop_lines);
    wr_file_max = wr_sock_max = sink_file_max = sink_con_max = 0;
}

/* ---- L01: non-blocking log ----
 * Producers (game thread, audio thread) format the line and copy it into a slot of a lock-free queue. They make no
 * SD, socket or stdout call. One low-priority writer thread (core 1, or core 0 when core 1 is not available) drains the queue:
 * socket send first, then copies the bytes into fb and cb. A second low-priority thread (sink, PI6) writes fb to log.txt,
 * prints cb on the bottom screen at 10 per second. Full queue: the new line is dropped and counted.
 * Before the writer thread runs, pm_log writes directly (boot lines). */
#define SLOTS 1024 /* HWM1: was 512. The first O3DS run lost 1207 lines when the writer fell behind. LOG2: 1024 slots of 256 B = 256 KB. */
#define SLOT_BYTES 256
#define VIP 16 /* level flag: a measurement line (pm_log_hw) keeps the last SLOT_RES slots, other lines are dropped first */
#define SLOT_RES 128
#define LINE_MAX 1024 /* the writer's formatted line: "#<seq> " body and newline */
#define ABANDON_TICKS (268111856ull / 20ull) /* PLAN E: a queue position uncommitted for 50 ms is abandoned */
/* LOG2/PLAN E: slot state (lap << 2) | k, lap = queue position >> 10 (SLOTS is a power of two). k: 0 free for this lap, 1 reserved,
 * 2 committed, 3 abandoned. The producer owns a slot from its reserve CAS (0 to 1) to its commit CAS (1 to 2). The writer frees it by
 * storing (lap + 1) << 2, which is free for the next lap. A producer that stalls past the abandon time loses its commit CAS and drops
 * the line (counted); the writer moves on. */
typedef struct {
    u32 state;
    const char* fmt;
    u32 seq;
    u32 th;      /* thread that logged the line (threadGetCurrent) */
    u8 nargs, nw, lv, flags; /* flags: bit 0 prof line (log.txt only), bit 1 fallback (str holds the formatted text) */
    u16 slen;    /* string area in use */
    u8 pad[10];
    u32 w[LF_MAX_ARGS];
    char str[LF_STR_SIZE];
} Slot;
_Static_assert(sizeof(Slot) == SLOT_BYTES, "Slot is 256 bytes");
static Slot slots[SLOTS] __attribute__((aligned(32)));
static LfDesc ldesc[512]; /* LOG2: descriptor cache, direct-mapped on (fmt >> 2) & 511 */
static int writer_stop;
static int crashed;
static int active_id;
static int dirty;
static int writer_started; /* id of the writer thread that has run its first line */
static Thread writer_th;
static Thread sink_th;
static LightLock drain_lock;

void pm_con_lock(void) { LightLock_Lock(&con_lock); }
int pm_con_trylock(void) { return LightLock_TryLock(&con_lock); }
void pm_con_unlock(void) { LightLock_Unlock(&con_lock); }
void pm_log_console_ready(void) {
    LightLock_Lock(&con_lock);
    con_up = 1; /* the sink thread prints the lines that waited in cb */
    LightLock_Unlock(&con_lock);
    LightEvent_Signal(&ev_sink); /* U3: one header draw, also when the queue is off at boot */
}
static LightEvent ev_wake, ev_started, ev_flushed, ev_quiet;
static volatile int io_drain_req;
static void wake_writer(void) { if (writer_th) LightEvent_Signal(&ev_wake); }

static int prof_force; /* PROF4: set only by pm_log_prof_line, the one line that prints in prof mode */
static void vlog(int lv, const char* fmt, va_list ap) {
    int vip = (lv & VIP) != 0;
    lv &= 15;
    if (!qon()) return; /* logging and Profiling off: nothing below runs, not even the level test (U1) */
    if (lv > pass_lv && !prof_force) return; /* CONLV: one compare; the sinks check their own level in drain_locked */
    if (!writer_th) return; /* no writer: drop the line, never touch SD, socket or stdout on the caller */
    u32 lim = vip ? SLOTS : SLOTS - SLOT_RES;
    /* Queue full: one load and compare, before the descriptor lookup, so a flood costs almost nothing. */
    u32 h = __atomic_load_n(&q_head, __ATOMIC_RELAXED);
    for (;;) {
        if (h - __atomic_load_n(&q_tail, __ATOMIC_RELAXED) >= lim) { /* no acquire: the slot CAS below acquires */
            __atomic_fetch_add(&q_dropped, 1, __ATOMIC_RELAXED);
            return;
        }
        if (__atomic_compare_exchange_n(&q_head, &h, h + 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) break;
    }
    /* The position h is ours. Its slot must be free for this lap (state (lap << 2) | 0). */
    LfDesc* d = &ldesc[((uintptr_t)fmt >> 2) & 511u];
    if (d->fmt != fmt) lf_scan(fmt, d);
    u32 free_st = (h >> 10) << 2;
    Slot* sl = &slots[h & (SLOTS - 1)];
    u32 exp = free_st;
    if (!__atomic_compare_exchange_n(&sl->state, &exp, free_st | 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
        __atomic_fetch_add(&q_dropped, 1, __ATOMIC_RELAXED); /* the writer abandoned this position: the line is dropped */
        return;
    }
    sl->fmt = fmt;
    sl->seq = __atomic_fetch_add(&seq, 1, __ATOMIC_RELAXED);
    sl->th = (u32)(uintptr_t)threadGetCurrent();
    sl->lv = (u8)lv;
    sl->flags = (u8)(prof_force ? 1 : 0);
    sl->slen = 0;
    if (d->ok) {
        sl->nargs = d->nargs;
        sl->nw = d->nw;
        lf_copy(sl->w, sl->str, &sl->slen, d, ap);
    } else {
        sl->flags |= 2; /* refused format: formatted now, the text is in str */
        sl->nargs = sl->nw = 0;
        vsnprintf(sl->str, LF_STR_SIZE, fmt, ap);
        sl->str[LF_STR_SIZE - 1] = 0;
        sl->slen = (uint16_t)strlen(sl->str);
    }
    exp = free_st | 1;
    if (!__atomic_compare_exchange_n(&sl->state, &exp, free_st | 2, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED)) {
        __atomic_fetch_add(&q_dropped, 1, __ATOMIC_RELAXED); /* stalled past the abandon time: the writer took the slot */
        return;
    }
    __atomic_fetch_add(&st_lines, 1, __ATOMIC_RELAXED);
    /* LOG4: no signal per line. pm_log_frame_tick signals the writer once per frame at half full; its 250 ms timeout collects the rest. */
}

/* LOG4: called once per drawn frame (pm_main.c, after pm_gpu_end). Logging off: one load and return. One signal to the writer when the
 * queue reaches half full (edge: the flag clears below a quarter), so a storm costs one wake per fill, not one per line. */
static int half_sig;
void pm_log_frame_tick(void) {
    if (!qon() || !writer_th) return;
    static int storm = -1; /* LOG4 test switch logstorm=<n>: n debug lines per drawn frame (the storm bench) */
    if (storm < 0) { char v[16]; storm = pm_debug_value("logstorm", v, sizeof v) ? atoi(v) : 0; }
    for (int i = 0; i < storm; i++) pm_log_d("storm line %d", i);
    u32 n = __atomic_load_n(&q_head, __ATOMIC_RELAXED) - __atomic_load_n(&q_tail, __ATOMIC_RELAXED);
    if (n >= SLOTS / 2) {
        if (!half_sig) { half_sig = 1; LightEvent_Signal(&ev_wake); }
    } else if (n < SLOTS / 4) half_sig = 0;
}

#define PM_LOG_FN(name, lv) void name(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vlog(lv, fmt, ap); va_end(ap); }
PM_LOG_FN(pm_log_e, 0)
PM_LOG_FN(pm_log_w, 1)
PM_LOG_FN(pm_log_i, 2)
PM_LOG_FN(pm_log_d, 3)
/* PROF4: the prof line. Queued at warn level and written to log.txt in prof mode (the only line that prints there). */
void pm_log_prof_line(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    prof_force = 1; vlog(1, fmt, ap); prof_force = 0;
    va_end(ap);
}
/* PROF4, PROFROW: the lean prof mode and the Profiling row (pm_phase.c prof_row_apply). Mode 0 off, 1 prof (prof lines only, as prof=4),
 * 2 phase set. Each mode sets the words it owns. A word that a mode gives up returns to its state at boot (base_has), so a launch word
 * such as pmu or drawlog comes back when the row leaves prof. base_save runs at the first call, before any change of the words. */
static const char* const row_w[] = { "phase", "sp0c", "opsplit", "pacestat", "logstat", NULL }; /* owned by the phase mode */
static const char* const prof_w[] = { "pmu", "fsplit", "texstat", "audiostat", "drawcensus", "drawlog", NULL }; /* off in prof mode */
#define ROW_N 5
#define PROF_N 6
static char base_has[ROW_N + PROF_N];
static int base_saved;
static void base_save(void) {
    if (base_saved) return;
    base_saved = 1;
    int k = 0;
    for (const char* const* w = row_w; *w; w++) base_has[k++] = pm_debug_has(*w) != 0;
    for (const char* const* w = prof_w; *w; w++) base_has[k++] = pm_debug_has(*w) != 0;
}
/* U2, LOG5: mode 0 off, 1 prof (profiling lines only), 2 phase (the normal lines at the saved live and console levels, never log.txt).
 * Logging is off in both profiling modes. The three levels are saved when profiling starts and restored when it ends: mode 1 sets all three
 * to off (only profiling lines pass), mode 2 sets the disk level off and keeps the saved live and console levels. */
void pm_log_prof_row(int mode) {
    base_save();
    for (const char* const* w = row_w; *w; w++) pm_log_switch(*w, mode == 2);
    int k = ROW_N;
    for (const char* const* w = prof_w; *w; w++, k++) pm_log_switch(*w, mode != 1 && base_has[k]);
    pm_log_switch("prof", mode == 1);
    int was = prof_mode;
    if (mode && !was) { sv_lv[0] = lv_live; sv_lv[1] = lv_disk; sv_lv[2] = lv_con; }
    if (mode) log_file_on = 0;
    prof_mode = mode != 0;
    if (!mode && was) { lv_live = sv_lv[0]; lv_disk = sv_lv[1]; lv_con = sv_lv[2]; }
    if (mode == 1) lv_live = lv_disk = lv_con = -1;
    if (mode == 2) { lv_live = sv_lv[0]; lv_disk = prof_disk ? sv_lv[1] : -1; lv_con = sv_lv[2]; } /* phase lines (census, wcen4) reach log.txt only with prof_disk */
    gate_changed();
}
/* PROF4: the launch word prof=1 (boot_line 1 prints "prof=1 log=off"). U2: it turns logging off for this run. */
void pm_log_prof_apply(int boot_line) {
    char v[16];
    base_save();
    if (!(pm_debug_has("prof") || pm_debug_value("prof", v, sizeof v))) return;
    pm_log_prof_row(1);
    if (boot_line) pm_log_prof_line("prof=1 log=off");
}
/* U3: the bottom screen header. con: "Console: Profiling" while profiling, else "Console: <level or Off>" (Off when logging is off).
 * live: "Live log: On" (a client is connected), "Live log: waiting" (the live log is on with no client), else "Live log: Off". */
static const char* const hdr_lvl[5] = { "Error", "Warn", "Info", "Debug", "Off" };
static int live_state(void) { /* 0 off, 1 waiting, 2 on */
    if (!(prof_mode || (log_file_on && lv_live >= 0))) return 0;
    return client >= 0 ? 2 : 1;
}
void pm_log_header(char* con, int nc, char* live, int nl) {
    if (prof_mode) snprintf(con, (size_t)nc, "Console: Profiling");
    else snprintf(con, (size_t)nc, "Console: %s", hdr_lvl[log_file_on && lv_con >= 0 ? lv_con : 4]);
    int s = live_state();
    snprintf(live, (size_t)nl, "Live log: %s", s == 2 ? "On" : s == 1 ? "waiting" : "Off");
}
/* U3: a key of the header text; the bottom screen redraws its header when it changes. */
int pm_log_header_key(void) {
    return (prof_mode ? 1 << 12 : 0) | (log_file_on ? 1 << 11 : 0) | ((lv_con + 1) & 7) << 8 | live_state() << 4 | (client >= 0 ? 1 : 0);
}
int pm_log_prof_mode(void) { return prof_mode; }
PM_LOG_FN(pm_log, 2) /* an old call is info */
PM_LOG_FN(pm_log_hw, 3 | VIP) /* HWM1: a measurement line at debug level that the queue keeps longest */

/* LOG1 logbench, switch logbench, called once at boot: the producer cost of a kept plain line, a kept line with four
 * arguments, a kept line with two %s, and a dropped line (level test fails), in CPU ticks (svcGetSystemTick, ARM11 clock).
 * Kept lines run in batches of 512 (inside the queue room); the writer drains between batches, outside the timed window. */
#define LB_N 10000
#define LB_BATCH 512
static u64 lb_ticks;
static void lb_drain(void) {
    wake_writer();
    while (__atomic_load_n(&q_head, __ATOMIC_ACQUIRE) != __atomic_load_n(&q_tail, __ATOMIC_ACQUIRE)) svcSleepThread(1000000LL);
}
static void lb_run(int kind) {
    lb_ticks = 0;
    for (int done = 0; done < LB_N;) {
        int k = LB_N - done < LB_BATCH ? LB_N - done : LB_BATCH;
        u64 t0 = svcGetSystemTick();
        for (int i = 0; i < k; i++) {
            if (kind == 0) pm_log_d("logbench plain line");
            else if (kind == 1) pm_log_d("logbench x %d %u %08x %.2f", i, (unsigned)i, (unsigned)i, 1.5 + i);
            else pm_log_d("logbench s %s %s", "abcdefghijklmnop", "qrstuvwxyz012345");
        }
        lb_ticks += svcGetSystemTick() - t0;
        done += k;
        if (kind != 3) lb_drain();
    }
}
void pm_log_bench(void) {
    if (!pm_debug_has("logbench")) return;
    static const char* const names[4] = {"plain", "args", "str2", "dropped"};
    double res[4];
    int save = lv_live, savd = lv_disk;
    lv_live = lv_disk = 3;
    pass_recalc();
    for (int kind = 0; kind < 3; kind++) { lb_run(kind); res[kind] = (double)lb_ticks / LB_N; }
    lv_live = lv_disk = -1; /* the level test fails: the dropped path */
    pass_recalc();
    lb_run(3);
    res[3] = (double)lb_ticks / LB_N;
    lv_live = save;
    lv_disk = savd;
    pass_recalc();
    for (int kind = 0; kind < 4; kind++) pm_log_w("logbench kind %s cycles %.1f", names[kind], res[kind]);
    pm_log_w("logbench: done");
}

/* HW8: a client that only returns EAGAIN (stalled Wi-Fi, a reader that stopped) is closed after 3 s without one successful send.
 * The PC script reconnects and the ring replay fills the gap. Counters go to the logstat line. */
#define LIVE_STALL_TICKS (3ull * 268111856ull)

/* PI6: the file and the console are written by the sink thread (sink_main). The writer only copies bytes into fb and cb,
 * so an SD write or a console draw that takes 1 to 2 s holds up the sink, not the queue. */
static u32 us_since(u64 t0) { return (u32)((svcGetSystemTick() - t0) / (SYSCLOCK_ARM11 / 1000000ull)); }
static unsigned fb_used(void) { return fb_head - fb_tail; }

/* fb_lock held: copy n bytes of s in, the caller checked there is room. */
static void fb_copy_in(const char* s, unsigned n) {
    unsigned off = fb_head & (FB_SIZE - 1);
    unsigned first = FB_SIZE - off;
    if (first > n) first = n;
    memcpy(fb + off, s, first);
    memcpy(fb, s + first, n - first);
    fb_head += n;
}

static char fw_buf[8192]; /* fsink_lock held: one step of the file write */
/* fsink_lock held: write every byte in fb to log.txt. Only the sink thread, the writer (full fb), the stop and the crash path call it. */
static void fb_write_locked(unsigned budget) { /* budget 0: all of fb, else at most budget bytes (LOG4: one SD write per step) */
    unsigned tot = 0;
    for (;;) {
        if (budget && tot >= budget) break;
        LightLock_Lock(&fb_lock);
        unsigned n = fb_used();
        if (n > sizeof fw_buf) n = sizeof fw_buf;
        unsigned off = fb_tail & (FB_SIZE - 1);
        unsigned first = FB_SIZE - off;
        if (first > n) first = n;
        memcpy(fw_buf, fb + off, first);
        memcpy(fw_buf + first, fb, n - first);
        LightLock_Unlock(&fb_lock);
        if (!n) break;
        tot += n;
        if (log_fd >= 0) {
            u64 t0 = svcGetSystemTick();
            unsigned done = 0;
            while (done < n) {
                ssize_t w = write(log_fd, fw_buf + done, n - done);
                if (w <= 0) break;
                done += (unsigned)w;
            }
            if (done) dirty = 1;
            u32 d = us_since(t0);
            if (d > sink_file_max) sink_file_max = d;
        }
        LightLock_Lock(&fb_lock);
        fb_tail += n;
        LightLock_Unlock(&fb_lock);
    }
}

/* Any thread except the crash path: write fb now, then fsync. Stops on a stalled SD card only as long as the write takes. */
static void fb_sync_all(void) {
    LightLock_Lock(&fsink_lock);
    fb_write_locked(0);
    if (log_fd >= 0 && dirty) { fsync(log_fd); dirty = 0; }
    LightLock_Unlock(&fsink_lock);
}

/* Writer only: add lines to fb. Full fb (the SD card is behind by 128 KB): write now, so no line is dropped. */
/* LOG4: a full fb drops the whole batch for log.txt (counted in fb_drop_lines, beat line fbdrop), never waits on fsink_lock. */

static void fb_put(const char* s, unsigned n) {
    LightLock_Lock(&fb_lock);
    if (n <= FB_SIZE - fb_used()) {
        fb_copy_in(s, n);
    } else {
        for (unsigned i = 0; i < n; i++) if (s[i] == '\n') fb_drop_lines++;
    }
    LightLock_Unlock(&fb_lock);
}

/* Writer only: keep the console bytes for the sink thread. Full cb: skipped (log.txt and the socket keep the line). */
static void cb_put(const char* s, unsigned n) {
    LightLock_Lock(&fb_lock);
    if (cb_n + n <= CB_SIZE) { memcpy(cb + cb_n, s, n); cb_n += n; }
    else con_skipped++;
    LightLock_Unlock(&fb_lock);
}

/* LOG3: one buffer per sink. dk: log.txt lines (fb), lv: socket lines (and the ring), cs: console bodies (cb). An empty buffer is skipped. */
static void out_batch(const char* dk, int dn, const char* lv, int ln, const char* cs, int cn) {
    if (dn > 0 && log_fd >= 0) { /* lines queued before the gate went off still reach log.txt */
        u64 t0 = svcGetSystemTick();
        fb_put(dk, (unsigned)dn);
        u32 d = us_since(t0);
        if (d > wr_file_max) wr_file_max = d;
    }
    if (ln > 0) {
        u64 t0 = svcGetSystemTick();
        ring_add(lv, ln);
        if (client >= 0) {
            int off = 0;
            while (off < ln) {
                static int stall_test = -1; /* test switch livestall: every send fails with EAGAIN (a reader that stopped) */
                if (stall_test < 0) stall_test = strstr(switches, "livestall") != NULL;
                int r = stall_test ? (errno = EAGAIN, -1) : send(client, lv + off, (size_t)(ln - off), 0);
                if (r > 0) {
                    off += r;
                    live_ok_tick = 0;
                    continue;
                }
                if (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    close(client);
                    client = -1;
                } else {
                    u64 now = svcGetSystemTick();
                    live_eagain++;
                    if (!live_ok_tick) live_ok_tick = now;
                    else if (now - live_ok_tick > LIVE_STALL_TICKS) {
                        close(client);
                        client = -1;
                        live_ok_tick = 0;
                        live_stall_close++;
                        pm_log_w("live log: client closed, 3 s of only EAGAIN (%u EAGAIN, %u closes)", live_eagain, live_stall_close);
                    }
                }
                break; /* a full socket drops the rest for the live view only, log.txt has it */
            }
        }
        u32 d = us_since(t0);
        if (d > wr_sock_max) wr_sock_max = d;
    }
    /* The console (bottom screen text, slow) is for a reader on the device: the sink thread prints cb at 10 per second. */
    if (cn > 0 && !pm_menu_open) cb_put(cs, (unsigned)cn); /* PT69: the open menu owns the console */
}

/* Sink thread: file and console, every 100 ms. Never runs while the app is suspended (pm_io_quiet, PT68). */
void pm_gpu_hud_flush(void);
static void con_flush(void) {
    static char cw[CB_SIZE];
    unsigned n = 0;
    if (!con_up) return; /* lines wait in cb until the console exists */
    LightLock_Lock(&fb_lock);
    if (pm_menu_open) cb_n = 0;
    else { n = cb_n; memcpy(cw, cb, n); cb_n = 0; }
    LightLock_Unlock(&fb_lock);
    if (n) {
        u64 t0 = svcGetSystemTick();
        LightLock_Lock(&con_lock); /* PT65: the log goes on while the menu is open (it owns rows 3 to 14) */
        fwrite(cw, 1, n, stdout);
        fflush(stdout);
        for (unsigned i = 0; i < n; i++) con_lines += cw[i] == '\n';
        LightLock_Unlock(&con_lock);
        u32 d = us_since(t0);
        if (d > sink_con_max) sink_con_max = d;
    }
    pm_gpu_hud_flush();
}

static void sink_main(void* arg) {
    (void)arg;
    unsigned dtick = 0;
    for (;;) {
        /* LOG4 (U1): logging off, the sink waits with no timeout; pm_log_set_file and pm_log_stop signal ev_sink. */
        if (qon()) svcSleepThread(100000000LL);
        else LightEvent_Wait(&ev_sink);
        if (__atomic_load_n(&writer_stop, __ATOMIC_ACQUIRE) || __atomic_load_n(&crashed, __ATOMIC_ACQUIRE)) return;
        if (pm_io_quiet) continue;
        if (!qon()) { /* lines queued before the gate went off: write them once, then sleep again */
            LightLock_Lock(&fsink_lock);
            if (!pm_io_quiet) fb_write_locked(0);
            LightLock_Unlock(&fsink_lock);
            con_flush();
            continue;
        } /* U3: logging off, one header draw after a change (the thread waits again at once) */
        /* No fsync here: an SD fsync held fsink_lock for 1 to 7 s. fb_sync_all and the crash path still sync.
         * LOG4: one SD write per 250 ms (every third 100 ms step) or 32 KB, whichever comes first. */
        if (++dtick >= 3 || fb_used() >= 32768u) {
            dtick = 0;
            LightLock_Lock(&fsink_lock);
            if (!pm_io_quiet) fb_write_locked(32768u); /* SLEEP2: checked under the lock the gate drain takes */
            LightLock_Unlock(&fsink_lock);
        }
        con_flush();
    }
}

/* Report lines that producers dropped because the queue was full (writer or crash flush). */
static void report_drops(void) {
    u32 d = __atomic_exchange_n(&q_dropped, 0, __ATOMIC_RELAXED);
    if (!d) return;
    q_drop_total += d;
    char m[80];
    int mn = snprintf(m, sizeof(m), "#%u log: %u lines dropped (queue full)\n", __atomic_fetch_add(&seq, 1, __ATOMIC_RELAXED), d);
    /* LOG3: a drop report is an error line: each sink that is on takes it */
    out_batch(m, lv_disk >= 0 ? mn : 0, m, lv_live >= 0 ? mn : 0, m, lv_con >= 0 ? mn : 0);
}

/* CONCOL: console colour per level (error, warn, info, debug). SGR codes libctru maps: 91 bright red, 33 yellow, 37 white, 90 grey.
 * Console bytes only: log.txt, the socket and the ring keep the plain text. Each coloured line resets to 0 before its newline. */
#define CON_SGR_LEN 5 /* ESC [ two digits m */
static const char con_sgr[4][CON_SGR_LEN + 1] = { "\x1b[91m", "\x1b[33m", "\x1b[37m", "\x1b[90m" };
#define CON_RESET "\x1b[0m"

/* PLAN E: the writer's view of the queue. stall_on: the tail position stayed uncommitted since stall_tick. q_abandoned: positions the
 * writer gave up on. beat_due: set every 5 s by writer_beat, the drain writes the beat line (no #seq, no vlog). */
static u32 q_abandoned;
static int stall_on;
static u32 stall_t;
static u64 stall_tick;
static int beat_due;

/* Writer: format a committed slot into out as "#<seq> <body>\n". Returns the length. */
static int line_of(const Slot* sl, char* out, unsigned cap) {
    int n = snprintf(out, cap, "#%u ", sl->seq);
    unsigned o = (unsigned)n;
    if (sl->flags & 2) {
        unsigned l = sl->slen;
        if (o + l > cap - 2) l = cap - 2 - o;
        memcpy(out + o, sl->str, l);
        o += l;
    } else {
        o += lf_format(out + o, cap - o - 1, sl->fmt, sl->w, sl->str);
    }
    if (o == 0 || out[o - 1] != '\n') out[o++] = '\n';
    return (int)o;
}

/* The caller holds drain_lock. */
static void drain_locked(void) {
    static char lb[8192], sb[8192], bb[8192], line[LINE_MAX];
    int ln = 0, sn = 0, bn = 0;
    int dl = lv_disk, ll = lv_live, cl = lv_con; /* LOG3: a sink at -1 takes nothing */
    if (beat_due) {
        beat_due = 0;
        u32 t = q_tail, hd = __atomic_load_n(&q_head, __ATOMIC_ACQUIRE);
        u32 st = __atomic_load_n(&slots[t & (SLOTS - 1)].state, __ATOMIC_ACQUIRE);
        u32 age = stall_on && stall_t == t ? (u32)((svcGetSystemTick() - stall_tick) / (SYSCLOCK_ARM11 / 1000)) : 0;
        int bl = snprintf(line, sizeof line, "beat hb %u head %u tail %u tstate %u tage_ms %u tthread %08x abandoned %u dropped %u file_on %d pass_lv %d sinks disk %u live %u con %u fbdrop %u\n",
                          writer_hb, hd, t, st, age, (u32)(uintptr_t)threadGetCurrent(), q_abandoned, q_drop_total + q_dropped, log_file_on, pass_lv, cnt_disk, cnt_live, cnt_con, fb_drop_lines);
        if (bl > (int)sizeof line - 1) bl = sizeof line - 1;
        out_batch(line, dl >= 2 ? bl : 0, line, ll >= 2 ? bl : 0, line, cl >= 2 ? bl : 0); /* LOG3: the beat is an info line */
    }
    u32 t = q_tail;
    u32 hd = __atomic_load_n(&q_head, __ATOMIC_ACQUIRE);
    /* Bounded: at most SLOTS/4 slots per call. A debug flood refills the queue as fast as it drains, and an unbounded drain kept the
     * writer from returning to accept_client (the console could not connect). The rest waits for the next writer pass. */
    for (int k = 0; k < SLOTS / 4 && t != hd; k++) {
        Slot* sl = &slots[t & (SLOTS - 1)];
        u32 want = (t >> 10) << 2;
        u32 st = __atomic_load_n(&sl->state, __ATOMIC_ACQUIRE);
        if (st != (want | 2) && st != (want | 3)) {
            /* Reserved, not committed. A producer that stalls past ABANDON_TICKS does not block the queue: the position is abandoned,
             * its late commit fails and the line is dropped (counted by the producer). */
            u64 now = svcGetSystemTick();
            if (!stall_on || stall_t != t) { stall_on = 1; stall_t = t; stall_tick = now; break; }
            if (now - stall_tick < ABANDON_TICKS) break;
            u32 e = st;
            if (__atomic_compare_exchange_n(&sl->state, &e, want | 3, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) q_abandoned++;
            continue;
        }
        stall_on = 0;
        if (st == (want | 2)) {
            int n = line_of(sl, line, sizeof line);
            int pad = (sl->flags & 1) != 0, lvl = sl->lv;
            st_bytes += (unsigned)n + 8;
            int skip = sw_lognull && !strstr(line, "CRASH") && !strstr(line, "FATAL") && !strstr(line, "logstat"); /* lognull: writer drops */
            int tf = !skip && (pad ? prof_disk : lvl <= dl); /* U2: a profiling line (pad 1) goes to log.txt only with prof_disk on */
            int tl = !skip && (pad || lvl <= ll), tc = !skip && (pad || lvl <= cl); /* U2: profiling lines go to the live log and the console */
            cnt_disk += (unsigned)tf; cnt_live += (unsigned)tl; cnt_con += (unsigned)tc; /* LOG3: counted per slot, the beat and drop lines are not */
            int cskip = 0, cn = 0;
            if (tc) { /* the console text is the line after "#<seq> " */
                const char* sp = (const char*)memchr(line, ' ', (size_t)n);
                cskip = sp ? (int)(sp - line) + 1 : 0;
                cn = n - cskip;
            }
            if ((tf && ln + n > (int)sizeof(lb)) || (tl && sn + n > (int)sizeof(sb)) || (tc && bn + cn + CON_SGR_LEN + 8 > (int)sizeof(bb))) {
                out_batch(lb, ln, sb, sn, bb, bn);
                ln = sn = bn = 0;
                report_drops(); /* a flood keeps the queue full, so report between batches */
            }
            if (tf) { memcpy(lb + ln, line, (size_t)n); ln += n; }
            if (tl) { memcpy(sb + sn, line, (size_t)n); sn += n; }
            if (tc) {
                int keep = (cn > 0 && line[cskip + cn - 1] == '\n') ? cn - 1 : cn; /* the newline goes after the reset */
                memcpy(bb + bn, con_sgr[sl->lv & 3], CON_SGR_LEN); bn += CON_SGR_LEN;
                memcpy(bb + bn, line + cskip, (size_t)keep); bn += keep;
                memcpy(bb + bn, CON_RESET, 4); bn += 4;
                if (keep < cn) bb[bn++] = '\n';
            }
        }
        __atomic_store_n(&sl->state, want + 4, __ATOMIC_RELEASE); /* free for the next lap */
        t++;
        __atomic_store_n(&q_tail, t, __ATOMIC_RELEASE);
    }
    out_batch(lb, ln, sb, sn, bb, bn);
    report_drops();
}

/* logstat, writer: a beat every 5 s (see the beat line in drain_locked). A stopped hb means a hang. */
static u64 beat_tick;
static void writer_beat(void) {
    u64 now = svcGetSystemTick();
    if (beat_tick && now - beat_tick < 5ull * SYSCLOCK_ARM11) return;
    beat_tick = now;
    beat_due = 1;
}

static void writer_main(void* arg) {
    int id = (int)(intptr_t)arg;
    writer_core = svcGetProcessorID();
    __atomic_store_n(&writer_started, id, __ATOMIC_RELEASE);
    LightEvent_Signal(&ev_started);
    for (;;) {
        /* Wakes every 250 ms, so accept_client and read_cmd run before every bounded drain. LOG4 (U1): logging off, no timeout:
         * the writer sleeps until a signal (pm_log_set_file, pm_io_gate, pm_log_stop). No heartbeat, no accept, no beat. */
        if (qon()) LightEvent_WaitTimeout(&ev_wake, 250000000LL);
        else LightEvent_Wait(&ev_wake);
        if (__atomic_load_n(&active_id, __ATOMIC_ACQUIRE) != id) return; /* a replacement thread took over */
        /* LOG5 fix: lines queued before the queue went off (the logging off line) still drain once: skip only with nothing queued */
        if (!qon() && q_head == q_tail && !io_drain_req && !__atomic_load_n(&writer_stop, __ATOMIC_ACQUIRE) && !__atomic_load_n(&crashed, __ATOMIC_ACQUIRE)) continue;
        writer_hb++;
        writer_beat();
        if (io_drain_req && __atomic_load_n(&writer_on, __ATOMIC_ACQUIRE)) {
            /* SLEEP2: the gate is set before the drain, so the sink cannot start an SD write while this batch syncs. */
            pm_io_quiet = 1;
            LightLock_Lock(&drain_lock);
            accept_client();
            read_cmd(); /* QUIT1 */
            if (!__atomic_load_n(&crashed, __ATOMIC_ACQUIRE)) {
                drain_locked();
            }
            fb_sync_all();
            io_drain_req = 0;
            LightLock_Unlock(&drain_lock);
            LightEvent_Signal(&ev_quiet); /* SLEEP2: io_gate(1) waits for this */
            continue;
        }
        if (!__atomic_load_n(&writer_on, __ATOMIC_ACQUIRE)) continue;
        if (pm_io_quiet) continue; /* PT68: no SD, socket or console call while the app sleeps */
        accept_client();
        read_cmd(); /* QUIT1 */
        LightLock_Lock(&drain_lock);
        if (!__atomic_load_n(&crashed, __ATOMIC_ACQUIRE)) {
            drain_locked();
        }
        LightLock_Unlock(&drain_lock);
        if (__atomic_load_n(&crashed, __ATOMIC_ACQUIRE)) LightEvent_Signal(&ev_flushed);
        if (__atomic_load_n(&writer_stop, __ATOMIC_ACQUIRE)) return;
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

/* CPULIM-UI: app core 1 time limit. pm_cpu_game_value = value the game set last (HUD), pm_cpu_state 0 = ok, -1 = the last call failed. */
volatile int pm_cpu_game_value = 30;
volatile int pm_cpu_state;
static int cpu_game = 80;
static int cpu_now = -1;
static aptHookCookie cpu_cookie;

static Result cpu_apply(int v, const char* why) {
    Result r = APT_SetAppCpuTimeLimit((u32)v);
    pm_log("cpu limit: set %d (%s) result %08lx", v, why, (unsigned long)r);
    if (R_SUCCEEDED(r)) { cpu_now = v; if (v == cpu_game) pm_cpu_game_value = v; pm_cpu_state = 0; }
    else pm_cpu_state = -1;
    return r;
}

/* Hook runs in the thread that calls aptMainLoop (the game thread). Suspend and sleep: 30, so the HOME menu stays responsive. Restore and wake: the game value. */
/* PT68: while the app is suspended or asleep no thread of ours may touch the SD card, the network or the console. The console freezes when the
 * log writer (fsync, soc) or the flash thread runs into a sleeping system module. The hook drains the queue and syncs the file first, then sets
 * the gate. The writer and the flash thread idle while it is set. Lines logged after the gate stay in the queue until the gate opens. */
volatile int pm_io_quiet;
/* SLEEP2: io_gate(1) is synchronous. It asks the writer to drain and sync, then waits (at most 500 ms) for ev_quiet. Returns the
 * drain time in ms, or -1 on the deadline (the gate is closed anyway: the writer may still be in its fsync). */
static int io_gate(int on) {
    if (on) {
        if (__atomic_load_n(&writer_on, __ATOMIC_ACQUIRE)) {
            unsigned long long t0 = svcGetSystemTick();
            LightEvent_Clear(&ev_quiet);
            io_drain_req = 1;
            LightEvent_Signal(&ev_wake);
            if (LightEvent_WaitTimeout(&ev_quiet, 500000000LL) == 0) return (int)((svcGetSystemTick() - t0) / CPU_TICKS_PER_MSEC);
            pm_io_quiet = 1;
            return -1;
        }
        pm_io_quiet = 1;
        return 0;
    }
    pm_io_quiet = 0;
    io_drain_req = 0;
    LightEvent_Signal(&ev_wake);
    return 0;
}

/* Sleep and exit steps that touch the SD card run on a worker thread, so the caller waits at most ms (pm_step_timed).
 * One worker per slot, made on first use and kept. The worker signals done before it clears busy: a new request never sees an old signal. */
typedef struct { void (*fn)(void*); void* arg; LightEvent go, done; volatile int busy, made; Thread th; } PmStep;
static PmStep steps[2];

static void step_main(void* p) {
    PmStep* s = (PmStep*)p;
    for (;;) {
        LightEvent_Wait(&s->go);
        s->fn(s->arg);
        LightEvent_Signal(&s->done);
        __atomic_store_n(&s->busy, 0, __ATOMIC_RELEASE);
    }
}

/* 1 = fn returned in time, 0 = deadline (the worker goes on, the slot stays busy), -1 = not run (slot busy or no thread). */
int pm_step_timed(int slot, void (*fn)(void*), void* arg, unsigned ms) {
    PmStep* s = &steps[slot & 1];
    if (__atomic_load_n(&s->busy, __ATOMIC_ACQUIRE)) return -1;
    if (!s->made) {
        s->made = 1;
        LightEvent_Init(&s->go, RESET_ONESHOT);
        LightEvent_Init(&s->done, RESET_ONESHOT);
        s->th = threadCreate(step_main, s, 16 * 1024, 0x30, -2, false);
    }
    if (!s->th) return -1;
    s->fn = fn;
    s->arg = arg;
    LightEvent_Clear(&s->done);
    __atomic_store_n(&s->busy, 1, __ATOMIC_RELEASE);
    LightEvent_Signal(&s->go);
    return LightEvent_WaitTimeout(&s->done, (s64)ms * 1000000LL) == 0 ? 1 : 0;
}

/* Sleep hook budget: audio join 300 ms, render 200 ms, flash 400 ms, about 0.9 s in all. Wake: audio 300 ms, render 200 ms. */
#define SLEEP_AUDIO_NS 300000000ULL
#define SLEEP_AU2_NS 200000000ULL
#define SLEEP_FLASH_MS 400u
#define SLEEP_IDLE_MS 300u /* SLEEP3: wait for the step slot and a save in flight, after the flash step */
#define WAKE_AUDIO_NS 300000000ULL
#define WAKE_AU2_NS 200000000ULL

static unsigned ms_since(unsigned long long t0) { return (unsigned)((svcGetSystemTick() - t0) / CPU_TICKS_PER_MSEC); }
static void flush_step(void* why) { pm_flash_flush((const char*)why); }

/* SLEEP3: one bounded enter sequence for HOME (suspend) and sleep. Every wait has a deadline. Order: audio join, render sync, flash step,
 * step slot and save idle, io gate (synchronous, drain and fsync done), then the 30 % core 1 limit. No core 1 I/O may run after the limit drops. */
static int step_slot_idle(int slot, unsigned ms) {
    unsigned long long t0 = svcGetSystemTick();
    for (;;) {
        if (!__atomic_load_n(&steps[slot & 1].busy, __ATOMIC_ACQUIRE) && !pm_wd_hold) return 1;
        if (ms_since(t0) >= ms) return 0;
        svcSleepThread(5000000LL);
    }
}

static void quiet_enter(const char* tag, const char* why) {
    unsigned long long t0 = svcGetSystemTick();
    if (pm_audio_join_timed(0, SLEEP_AUDIO_NS) == 0) pm_log("%s: audio join done, %u ms", tag, ms_since(t0));
    else pm_log("%s: audio join timeout, %u ms", tag, ms_since(t0));
    if (pm_au2_sync_timed(3, SLEEP_AU2_NS) == 0) pm_log("%s: render sync done, %u ms", tag, ms_since(t0));
    else pm_log("%s: render sync timeout, %u ms", tag, ms_since(t0));
    unsigned long long tf = svcGetSystemTick();
    int fr = pm_step_timed(PM_STEP_FLASH, flush_step, (void*)why, SLEEP_FLASH_MS);
    if (fr == 1) pm_log("%s: flash flush done, %u ms", tag, ms_since(tf));
    else if (fr == 0) pm_log("%s: flash flush timeout, %u ms", tag, ms_since(tf));
    else pm_log("%s: flash flush skipped, worker busy or not started", tag);
    if (step_slot_idle(PM_STEP_FLASH, SLEEP_IDLE_MS)) pm_log("%s: step slot idle, %u ms", tag, ms_since(t0));
    else pm_log("%s: step slot still busy, %u ms", tag, ms_since(t0));
    int dr = io_gate(1);
    if (dr >= 0) pm_log("%s: io drain done, %d ms", tag, dr);
    else pm_log("%s: io drain timeout, gate closed anyway, %u ms", tag, ms_since(t0));
    cpu_apply(30, why);
    pm_log("%s: gate closed, %u ms", tag, ms_since(t0));
}

static void sleep_enter_hook(void) {
    pm_wd_sleep_tick = (unsigned)(svcGetSystemTick() / CPU_TICKS_PER_MSEC);
    pm_wd_asleep = 1; /* SLP2: the watchdog notes the sleep and does not end the process */
    pm_wd_slept = 1; /* SLP2: the audio and render joins are bounded from now on */
    pm_log("sleep: enter");
    quiet_enter("sleep", "sleep");
}

/* APT wake: the gate opens first, then the audio job and the render queue are joined (bounded). Channels are set up again only when both are idle. */
/* SLP2: the wake line goes straight to crash.txt and is fsynced in a bounded step, so it is on the SD card even when a later wait
 * blocks. The buffer is static: a step that misses its deadline still uses it.
 * SLEEP1: the watchdog notes kept in RAM while asleep follow the wake line, in the same step. */
static char wake_buf[120];
static void wake_line_step(void* unused) {
    (void)unused;
    int fd = open("crash.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd >= 0) {
        write(fd, wake_buf, strlen(wake_buf));
        fsync(fd);
        close(fd);
    }
    pm_wd_flush_notes();
}
static void notes_step(void* unused) {
    (void)unused;
    pm_wd_flush_notes();
}
static void wake_hook(void) {
    unsigned long long t0 = svcGetSystemTick();
    pm_wd_asleep = 0;
    pm_wd_quiet = 0; /* SLEEP1: with the gate open the watchdog writes again */
    pm_wd_wake_frames = 120; /* SLP2: frame begin without the GPU sync wait for these frames (pm_gpu.c) */
    io_gate(0);
    pm_log("sleep: wake");
    unsigned wake_ms = (unsigned)(t0 / CPU_TICKS_PER_MSEC);
    snprintf(wake_buf, sizeof wake_buf, "sleep: wake at tick %u ms, asleep %u ms\n", wake_ms, wake_ms - pm_wd_sleep_tick);
    pm_step_timed(PM_STEP_FLASH, wake_line_step, NULL, 400);
    int a = pm_audio_join_timed(0, WAKE_AUDIO_NS);
    int r = pm_au2_sync_timed(3, WAKE_AU2_NS);
    if (a == 0 && r == 0) {
        pm_dspa_rearm();
        pm_log("sleep: audio resumed, %u ms", ms_since(t0));
    } else pm_log("sleep: audio not resumed, %s timeout, %u ms", a ? "audio join" : "render sync", ms_since(t0));
    cpu_apply(cpu_game, "wake");
}

static void cpu_hook(APT_HookType h, void* param) {
    (void)param;
    switch (h) {
    case APTHOOK_ONSUSPEND: pm_wd_quiet = 1; pm_log("home: suspend"); quiet_enter("home", "HOME menu opens"); break;
    case APTHOOK_ONSLEEP: sleep_enter_hook(); break;
    case APTHOOK_ONRESTORE:
        io_gate(0);
        pm_wd_quiet = 0;
        pm_step_timed(PM_STEP_FLASH, notes_step, NULL, 400); /* SLEEP1: the HOME notes go to crash.txt now, bounded */
        cpu_apply(cpu_game, "HOME menu closed");
        break;
    case APTHOOK_ONWAKEUP: wake_hook(); break;
    case APTHOOK_ONEXIT: pm_audio_drain(3); pm_flash_flush("exit hook"); if (cpu_now != 30) cpu_apply(30, "exit"); break;
    default: break;
    }
}

/* Called once at normal exit (main), after the game thread ended. */
void pm_cpu_limit_exit(void) {
    if (cpu_now != 30 && cpu_now != -1) cpu_apply(30, "exit (main)");
}

/* Start the writer thread. Call once, after pm_log_init. */
void pm_log_start(void) {
    if (writer_on || writer_th) return;
    LightLock_Init(&fb_lock);
    LightLock_Init(&fsink_lock);
    LightEvent_Init(&ev_wake, RESET_ONESHOT);
    LightEvent_Init(&ev_started, RESET_ONESHOT);
    LightEvent_Init(&ev_flushed, RESET_ONESHOT);
    LightEvent_Init(&ev_quiet, RESET_ONESHOT);
    LightEvent_Init(&ev_sink, RESET_ONESHOT);
    LightLock_Init(&drain_lock);
    /* Core 1 (system core): a thread there is allowed only after APT_SetAppCpuTimeLimit (libctru thread.h). */
    /* CPULIM-UI: the game value is 80 (try 79 when 80 fails). Switch cpulimit=<5..80> overrides. HOME menu and sleep set 30, resume sets the game value. */
    {
        int lim = 80;
        char v[16];
        if (pm_debug_value("cpulimit", v, sizeof v)) { lim = atoi(v); if (lim < 5 || lim > 80) lim = 30; }
        cpu_game = lim;
        Result r = cpu_apply(lim, "start");
        if (R_FAILED(r) && lim == 80) {
            cpu_game = 79;
            r = cpu_apply(79, "start retry");
        }
        if (R_FAILED(r)) pm_cpu_state = -1; /* HUD marker: the call failed */
        else pm_log_w("cpu limit: game value %d (APT result %08lx)", pm_cpu_game_value, (unsigned long)r);
        aptHook(&cpu_cookie, cpu_hook, NULL);
    }
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
    sink_th = threadCreate(sink_main, NULL, 32 * 1024, 0x3F, writer_core, false);
    pm_log_d("log: sink thread %s on core %d", sink_th ? "created" : "NOT created", writer_core);
    __atomic_store_n(&writer_on, 1, __ATOMIC_RELEASE);
}

/* HW7: after pm_log_stop. Close the listen socket, socExit, free the socket buffer (ours, at 0x08000000). */
void pm_log_soc_exit(void) {
    if (listen_fd >= 0) { close(listen_fd); listen_fd = -1; }
    if (soc_up) { socExit(); soc_up = 0; pm_soc_free(); }
}

/* Normal exit: stop the writer after it has written everything. */
void pm_log_stop(void) {
    if (!writer_th) return;
    __atomic_store_n(&writer_stop, 1, __ATOMIC_RELEASE);
    LightEvent_Signal(&ev_wake);
    LightEvent_Signal(&ev_sink);
    threadJoin(writer_th, 2000000000ULL);
    __atomic_store_n(&writer_on, 0, __ATOMIC_RELEASE);
    LightLock_Lock(&drain_lock);
    drain_locked(); /* the writer may have stopped on the timeout */
    LightLock_Unlock(&drain_lock);
    fb_sync_all(); /* the sink thread returns on writer_stop; its last bytes go out here */
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
    }
    /* The sink thread is gone or waits for fsink_lock: take it and write fb, then fsync. Kept taken. */
    int fg = LightLock_TryLock(&fsink_lock) == 0;
    if (!fg) {
        svcSleepThread(500000000LL);
        fg = LightLock_TryLock(&fsink_lock) == 0;
    }
    if (fg) {
        fb_write_locked(0);
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

/* LB4: before svcExitProcess. Every crash line is already fsynced. Close the file so the SD card entry is complete. */
void pm_log_crash_close(void) {
    if (log_fd >= 0) { fsync(log_fd); close(log_fd); log_fd = -1; }
}

void pm_crash_line(const char* s, int n) {
    static int crash_fd = -1;
    static int flushed;
    /* SLP2: the line is written and sent before the first crash_flush, which fsyncs. A blocked fsync (SD card waking up) no longer
     * keeps the phase line out of crash.txt. The fsyncs come after the writes. */
    svcOutputDebugString(s, n);
    /* crash.txt keeps every crash (append), so it stays after the next boot. */
    if (crash_fd < 0) crash_fd = open("crash.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (crash_fd >= 0) write(crash_fd, s, (size_t)n);
    if (client >= 0) send(client, s, (size_t)n, MSG_DONTWAIT);
    if (!flushed) {
        flushed = 1;
        crash_flush();
    }
    if (log_fd >= 0) {
        write(log_fd, s, (size_t)n);
        fsync(log_fd);
    }
    if (crash_fd >= 0) fsync(crash_fd);
}
