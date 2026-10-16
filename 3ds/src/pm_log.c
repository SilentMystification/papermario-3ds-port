#include <3ds.h>
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
static int st_on;

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

void pm_log_poll(void) { accept_client(); }

int pm_debug_has(const char* word) {
    if (!word || !switches[0]) return 0;
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

const char* pm_debug_text(void) { return switches; }

void pm_log_set_switches(const char* s) {
    switches[0] = 0;
    if (s) strncat(switches, s, sizeof(switches) - 1);
}

void pm_log_init(int live) {
    /* Keep the last run: a crash trace in log.txt must survive the next boot. */
    unlink("log.old");
    int rot = rename("log.txt", "log.old");
    int rot_err = errno;
    log_fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (rot != 0) pm_log("log rotate failed errno %d (no previous log.txt on the first boot)", rot_err);
    unsigned soc_bytes = 0;
    u32* soc = (u32*)pm_soc_buffer(&soc_bytes);
    if (!live || !soc || soc_bytes < 0x1000 || socInit(soc, soc_bytes) != 0) {
        if (live) pm_log("live log: off");
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
        pm_log("live log: bind 17492 failed\n");
        return;
    }
    fcntl(listen_fd, F_SETFL, O_NONBLOCK);
    pm_log("live log: port 17492\n");
}

static void stat_site(const char* fmt) {
    int i;
    for (i = 0; i < st_nsites; i++) if (st_fmt[i] == fmt) { st_cnt[i]++; return; }
    if (st_nsites < SITES) { st_fmt[st_nsites] = fmt; st_cnt[st_nsites++] = 1; }
}

/* Called by the frame loop every 300 drawn frames when the logstat switch is on. */
void pm_log_stat(float avg_ms, unsigned frames, float secs) {
    unsigned dl = st_lines - st_prev_lines, db = st_bytes - st_prev_bytes;
    st_prev_lines = st_lines;
    st_prev_bytes = st_bytes;
    pm_log("logstat frames %u work %.2f ms wall %.2f ms lines %u (%.1f/frame %.0f/s) bytes %u total %u", frames, avg_ms, secs * 1000.f / (float)frames, dl,
           (float)dl / (float)frames, (float)dl / secs, db, st_lines);
}

void pm_log_top(void) {
    for (int k = 0; k < 8; k++) {
        int best = -1;
        for (int i = 0; i < st_nsites; i++) if (st_cnt[i] && (best < 0 || st_cnt[i] > st_cnt[best])) best = i;
        if (best < 0) return;
        pm_log("logtop %u x \"%.60s\"", st_cnt[best], st_fmt[best]);
        st_cnt[best] = 0;
    }
}

void pm_log(const char* fmt, ...) {
    if (st_on == 0) st_on = pm_debug_has("logstat") ? 1 : 2;
    if (st_on == 1 && fmt[0] != 'l') stat_site(fmt);
    char body[240];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(body, sizeof(body), fmt, ap);
    va_end(ap);
    size_t bl = strlen(body);
    if (bl == 0 || body[bl - 1] != '\n') {
        if (bl + 1 < sizeof(body)) {
            body[bl++] = '\n';
            body[bl] = 0;
        }
    }
    char line[280];
    int n = snprintf(line, sizeof(line), "#%u %s", seq++, body);
    if (n < 0) return;
    st_lines++;
    st_bytes += (unsigned)n;
    /* lognull: measurement switch, drops every line that is not an error. */
    if (pm_debug_has("lognull") && !strstr(body, "CRASH") && !strstr(body, "FATAL") && !strstr(body, "logstat")) return;
    if (log_fd >= 0) {
        if (write(log_fd, line, (size_t)n) > 0) fsync(log_fd);
    }
    ring_add(line, n);
    if (client >= 0 && send(client, line, (size_t)n, 0) < 0) {
        close(client);
        client = -1;
    }
    fwrite(body, 1, bl, stdout);
    fflush(stdout);
}

void pm_assert_fail(const char* statement) {
    static const char* seen[8];
    static int nseen;
    int i;
    if (!statement) statement = "assert";
    for (i = 0; i < nseen; i++) if (seen[i] == statement) return;
    if (nseen < 8) seen[nseen++] = statement;
    pm_log("assert %s", statement);
}

void pm_crash_line(const char* s, int n) {
    static int crash_fd = -1;
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
