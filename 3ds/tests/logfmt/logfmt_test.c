/* LOG2 host test: deferred formatting (pm_logfmt.h) against vsnprintf, byte for byte.
 * Build and run (repo root, Git Bash): sh 3ds/tests/logfmt/run.sh. Cases come from logfmt_cases.inc (gen_logfmt.py). */
#define LF_FMT_OK(p) 1 /* host: string constants are not in the 3DS image range; the range test is for the device */
#include "pm_logfmt.h"
#include <stdio.h>
#include <stdlib.h>

#define REPS 40
static uint32_t rs = 2463534242u;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static uint64_t rnd64(void) { return ((uint64_t)rnd() << 32) | rnd(); }
static int RND_SMALL(void) { return (int)(rnd() % 14u) - 3; } /* -3..10: widths, precisions (negative precision = omitted) */
static const char* pool(int i) {
    static const char* const p[] = {"", "a", "abc", "hello world", "crash.txt", "0123456789abcdef", "Mario & Luigi", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"};
    return p[(unsigned)i % (sizeof p / sizeof p[0])];
}

static int nok, nfail;
static void check(int id, const char* fmt, ...) {
    LfDesc d;
    uint32_t w[LF_MAX_ARGS];
    char str[LF_STR_SIZE], got[1024], exp[1024];
    uint16_t slen = 0;
    va_list ap;
    lf_scan(fmt, &d);
    if (!d.ok) { printf("case %d: scan refused: %s\n", id, fmt); nfail++; return; }
    va_start(ap, fmt);
    lf_copy(w, str, &slen, &d, ap);
    va_end(ap);
    lf_format(got, sizeof got, fmt, w, str);
    va_start(ap, fmt);
    vsnprintf(exp, sizeof exp, fmt, ap);
    va_end(ap);
    if (strcmp(got, exp) != 0) {
        printf("FAIL case %d\n  fmt: %s\n  exp: %s\n  got: %s\n", id, fmt, exp, got);
        nfail++;
    } else nok++;
}

#include "logfmt_cases.inc"

/* Refused formats take the vsnprintf fallback in the producer. */
static void refusals(void) {
    static const char* const bad[] = {"v=%n", "%Lf", "%jd", "%zu", "%tx", "%qd", "%lc", "%ls", "%y", "end %", "%lf%n"};
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        LfDesc d;
        lf_scan(bad[i], &d);
        if (d.ok) { printf("FAIL refusal not refused: %s\n", bad[i]); nfail++; }
        else nok++;
    }
    /* more than 16 argument words */
    LfDesc d;
    lf_scan("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &d);
    if (d.ok) { printf("FAIL 17 arguments accepted\n"); nfail++; } else nok++;
    lf_scan("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &d);
    if (!d.ok) { printf("FAIL 16 arguments refused\n"); nfail++; } else nok++;
}


/* Hand-written cases for the paths the tree's corpus does not reach: stars (width and precision, negative precision), 64-bit,
 * doubles with flags, %p, %c, %.*s, %s of NULL. A line whose strings exceed LF_STR_SIZE is cut (not tested for equality). */
static void extra(void) {
    for (int r = 0; r < REPS; r++) {
        check(900, "%*d|%-*.*f|%.*s|%lld", RND_SMALL(), (int)rnd(), RND_SMALL(), RND_SMALL(), (double)(int)rnd() / 3.0,
              RND_SMALL(), pool((int)rnd()), (long long)rnd64());
        check(904, "%llu|%p|%c|%5.2e|%hhd|%#x", (unsigned long long)rnd64(), (void*)(uintptr_t)rnd(), (int)('a' + rnd() % 26),
              (double)(int)rnd() / 1e5, (int)(char)rnd(), (unsigned)rnd());
        check(901, "%*s|%-8s|%.3s|%s|%%|%ld %lu %lx", RND_SMALL(), pool((int)rnd()), pool((int)rnd()), pool((int)rnd()), (const char*)NULL,
              (long)(int)rnd(), (unsigned long)rnd(), (unsigned long)rnd());
        check(902, "%.*f %.*lf %lld %g %G", RND_SMALL(), (double)(int)rnd() / 9.0, RND_SMALL(), (double)(int)rnd() / 11.0,
              (long long)rnd64() >> 20, (double)(int)rnd() / 3.0, 1.0e-7);
    }
}

int main(void) {
    extra();
    for (int i = 0; i < NCASES; i++) {
        cases[i]();
    }
    refusals();
    printf("logfmt: %d ok, %d failed (%d formats, %d reps each)\n", nok, nfail, NCASES, REPS);
    return nfail ? 1 : 0;
}
