/* GENKEY: the read trace of a generator or a host test (LD_PRELOAD shim, built in the host image pm3ds-host:1, 32 and 64 bit).
 * Every successful open of a file (open, open64, openat, openat64, __open_2, __openat_2, fopen, fopen64, freopen, freopen64)
 * whose real path starts with RT_ROOT (default /work/) appends one line "<exe>\t<path below RT_ROOT>" to the file RT_LOG.
 * A directory open is not logged: the key of a traced step holds the full path list of its input tar (3ds/tools/readkey.sh),
 * which also covers every failed probe (a new file anywhere in the tar is a new key). glibc-internal opens do not pass the PLT,
 * so each public entry point is wrapped on its own. Self-test: 3ds/tests/host.d/readtrace.sh (each reader kind of the steps). */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void rt_log(const char* path, int fd_ok)
{
    const char* log = getenv("RT_LOG");
    const char* root = getenv("RT_ROOT");
    char rp[PATH_MAX], exe[PATH_MAX], line[2 * PATH_MAX + 4];
    struct stat st;
    size_t rl;
    int saved = errno, n, lfd;
    static int (*ropen)(const char*, int, ...);
    if (!fd_ok || !log || !path) goto out;
    if (!root) root = "/work/";
    if (!realpath(path, rp)) goto out;
    rl = strlen(root);
    if (strncmp(rp, root, rl) != 0) {
        /* HSDEP: RT_ROOT2 (a shared /tmp folder made outside the traced unit) is logged as "@<path below it>" */
        const char* r2 = getenv("RT_ROOT2");
        if (!r2 || strncmp(rp, r2, strlen(r2)) != 0) goto out;
        if (stat(rp, &st) != 0 || !S_ISREG(st.st_mode)) goto out;
        n = (int)readlink("/proc/self/exe", exe, sizeof exe - 1);
        exe[n > 0 ? n : 0] = 0;
        n = snprintf(line, sizeof line, "%s\t@%s\n", strrchr(exe, '/') ? strrchr(exe, '/') + 1 : exe, rp + strlen(r2));
        goto wr;
    }
    if (stat(rp, &st) != 0 || !S_ISREG(st.st_mode)) goto out;
    n = (int)readlink("/proc/self/exe", exe, sizeof exe - 1);
    exe[n > 0 ? n : 0] = 0;
    n = snprintf(line, sizeof line, "%s\t%s\n", strrchr(exe, '/') ? strrchr(exe, '/') + 1 : exe, rp + rl);
wr:
    if (n <= 0 || n >= (int)sizeof line) goto out;
    if (!ropen) ropen = (int (*)(const char*, int, ...))dlsym(RTLD_NEXT, "open");
    lfd = ropen(log, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644);
    if (lfd >= 0) { if (write(lfd, line, (size_t)n) < 0) { } close(lfd); }
out:
    errno = saved;
}

#define MODE_ARG(fl) mode_t mode = 0; if ((fl) & (O_CREAT | O_TMPFILE)) { va_list ap; va_start(ap, fl); mode = va_arg(ap, mode_t); va_end(ap); }

#define WRAP_OPEN(name)                                                                     \
    int name(const char* p, int fl, ...)                                                    \
    {                                                                                       \
        static int (*f)(const char*, int, ...);                                             \
        int r;                                                                              \
        MODE_ARG(fl)                                                                        \
        if (!f) f = (int (*)(const char*, int, ...))dlsym(RTLD_NEXT, #name);                \
        r = f(p, fl, mode);                                                                 \
        rt_log(p, r >= 0);                                                                  \
        return r;                                                                           \
    }
WRAP_OPEN(open)
WRAP_OPEN(open64)

static void at_log(int dfd, const char* p, int ok)
{
    char buf[PATH_MAX], dp[64];
    ssize_t n;
    if (!ok || !p) return;
    if (p[0] == '/' || dfd == AT_FDCWD) { rt_log(p, 1); return; }
    snprintf(dp, sizeof dp, "/proc/self/fd/%d", dfd);
    n = readlink(dp, buf, sizeof buf - 2);
    if (n <= 0 || (size_t)n + 2 + strlen(p) >= sizeof buf) return;
    buf[n] = '/'; strcpy(buf + n + 1, p);
    rt_log(buf, 1);
}

#define WRAP_OPENAT(name)                                                                   \
    int name(int d, const char* p, int fl, ...)                                             \
    {                                                                                       \
        static int (*f)(int, const char*, int, ...);                                        \
        int r;                                                                              \
        MODE_ARG(fl)                                                                        \
        if (!f) f = (int (*)(int, const char*, int, ...))dlsym(RTLD_NEXT, #name);           \
        r = f(d, p, fl, mode);                                                              \
        at_log(d, p, r >= 0);                                                               \
        return r;                                                                           \
    }
WRAP_OPENAT(openat)
WRAP_OPENAT(openat64)

int __open_2(const char* p, int fl)
{
    static int (*f)(const char*, int);
    int r;
    if (!f) f = (int (*)(const char*, int))dlsym(RTLD_NEXT, "__open_2");
    r = f(p, fl);
    rt_log(p, r >= 0);
    return r;
}
int __open64_2(const char* p, int fl)
{
    static int (*f)(const char*, int);
    int r;
    if (!f) f = (int (*)(const char*, int))dlsym(RTLD_NEXT, "__open64_2");
    r = f(p, fl);
    rt_log(p, r >= 0);
    return r;
}
int __openat_2(int d, const char* p, int fl)
{
    static int (*f)(int, const char*, int);
    int r;
    if (!f) f = (int (*)(int, const char*, int))dlsym(RTLD_NEXT, "__openat_2");
    r = f(d, p, fl);
    at_log(d, p, r >= 0);
    return r;
}
int __openat64_2(int d, const char* p, int fl)
{
    static int (*f)(int, const char*, int);
    int r;
    if (!f) f = (int (*)(int, const char*, int))dlsym(RTLD_NEXT, "__openat64_2");
    r = f(d, p, fl);
    at_log(d, p, r >= 0);
    return r;
}

#define WRAP_FOPEN(name)                                                                    \
    FILE* name(const char* p, const char* m)                                                \
    {                                                                                       \
        static FILE* (*f)(const char*, const char*);                                        \
        FILE* r;                                                                            \
        if (!f) f = (FILE * (*)(const char*, const char*)) dlsym(RTLD_NEXT, #name);         \
        r = f(p, m);                                                                        \
        rt_log(p, r != NULL);                                                               \
        return r;                                                                           \
    }
WRAP_FOPEN(fopen)
WRAP_FOPEN(fopen64)

#define WRAP_FREOPEN(name)                                                                  \
    FILE* name(const char* p, const char* m, FILE* s)                                       \
    {                                                                                       \
        static FILE* (*f)(const char*, const char*, FILE*);                                 \
        FILE* r;                                                                            \
        if (!f) f = (FILE * (*)(const char*, const char*, FILE*)) dlsym(RTLD_NEXT, #name);  \
        r = f(p, m, s);                                                                     \
        rt_log(p, r != NULL);                                                               \
        return r;                                                                           \
    }
WRAP_FREOPEN(freopen)
WRAP_FREOPEN(freopen64)
