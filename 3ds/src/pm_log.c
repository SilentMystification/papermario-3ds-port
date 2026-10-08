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
    log_fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
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

void pm_log(const char* fmt, ...) {
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

void pm_crash_line(const char* s, int n) {
    svcOutputDebugString(s, n);
    if (log_fd >= 0) {
        write(log_fd, s, (size_t)n);
        fsync(log_fd);
    }
    if (client >= 0) send(client, s, (size_t)n, MSG_DONTWAIT);
}
