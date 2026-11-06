/* LOG2: deferred formatting of log lines.
 * The producer (game or audio thread) does not format. It copies the arguments of the call into its queue slot, by kind: word, 64-bit
 * integer (two words), double (two words), string (copied into the slot's string area). The writer thread formats the slot with
 * lf_format. The output is the same as vsnprintf with the same arguments (host test: 3ds/tests/logfmt).
 * Format strings are classified once per call site (lf_scan, a direct-mapped descriptor cache in pm_log.c). A refused format
 * (%n, long double, %j %z %t %q, a format not in the read-only image, more than LF_MAX_ARGS words) goes through vsnprintf in the
 * producer, into the slot's string area (flag bit 1 in the slot). No division, no double arithmetic outside the argument copy. */
#ifndef PM_LOGFMT_H
#define PM_LOGFMT_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LF_MAX_ARGS 16   /* argument words per slot */
#define LF_STR_SIZE 160  /* string area per slot: copied %s values and the vsnprintf text of a refused format */
#define LF_STR_NONE 0xFFFFFFFFu /* a %s argument that did not fit: prints as "" */
#define LF_MAX_SPEC 24   /* longest conversion text (from % to the conversion char) */

#ifndef LF_FMT_OK
/* The format string is in the read-only image of the 3DS build (code and constants at 0x00100000; the heap starts at 0x08000000). */
#define LF_FMT_OK(p) ((uintptr_t)(p) >= 0x00100000u && (uintptr_t)(p) < 0x08000000u)
#endif

enum { LF_WORD = 0, LF_LL = 1, LF_DBL = 2, LF_STR = 3 };

typedef struct {
    int kind;  /* LF_WORD, LF_LL, LF_DBL, LF_STR */
    int len;   /* chars of the conversion text, from '%' through the conversion char */
    int stars; /* '*' in the text: word arguments before the value */
    char conv; /* the conversion char */
} LfConv;

/* One conversion at p (p[0] == '%', p[1] != '%'). Returns its length, 0 when refused. */
static int lf_conv(const char* p, LfConv* c) {
    const char* q = p + 1;
    int lsingle = 0, ll = 0;
    c->stars = 0;
    while (*q && strchr("-+ #0", *q)) q++;
    if (*q == '*') { c->stars++; q++; }
    else while (*q >= '0' && *q <= '9') q++;
    if (*q == '.') {
        q++;
        if (*q == '*') { c->stars++; q++; }
        else while (*q >= '0' && *q <= '9') q++;
    }
    if (*q == 'h') { q++; if (*q == 'h') q++; }
    else if (*q == 'l') { q++; if (*q == 'l') { q++; ll = 1; } else lsingle = 1; }
    else if (*q == 'L' || *q == 'j' || *q == 'z' || *q == 't' || *q == 'q') return 0;
    char cv = *q;
    c->conv = cv;
    switch (cv) {
    case 'd': case 'i': case 'u': case 'o': case 'x': case 'X': case 'p':
        c->kind = ll ? LF_LL : LF_WORD;
        break;
    case 'c':
        if (ll || lsingle) return 0; /* wint_t: refused */
        c->kind = LF_WORD;
        break;
    case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A':
        if (ll) return 0;
        c->kind = LF_DBL;
        break;
    case 's':
        if (ll || lsingle) return 0; /* wchar_t string: refused */
        c->kind = LF_STR;
        break;
    default:
        return 0; /* %n and unknown conversions */
    }
    c->len = (int)(q + 1 - p);
    if (c->len > LF_MAX_SPEC) return 0;
    return c->len;
}

/* The descriptor of one format: argument kinds, 2 bits per argument (bit 2i for argument i). ok 0: the format goes through vsnprintf. */
typedef struct {
    const char* fmt;
    uint32_t kinds;
    uint8_t nargs, nw, ok, pad;
} LfDesc;

/* Scan a format once. Star arguments are word arguments, before their value. */
static void lf_scan(const char* f, LfDesc* d) {
    int n = 0, w = 0;
    d->fmt = f;
    d->kinds = 0;
    d->nargs = d->nw = d->ok = 0;
    if (!LF_FMT_OK(f)) return;
    for (const char* p = f; *p;) {
        if (*p != '%') { p++; continue; }
        if (p[1] == '%') { p += 2; continue; }
        LfConv c;
        int len = lf_conv(p, &c);
        if (!len) return;
        if (n + c.stars + 1 > LF_MAX_ARGS) return;
        for (int s = 0; s < c.stars; s++) {
            d->kinds |= (uint32_t)LF_WORD << (2 * n);
            n++;
            w++;
        }
        int words = (c.kind == LF_LL || c.kind == LF_DBL) ? 2 : 1;
        if (w + words > LF_MAX_ARGS) return;
        d->kinds |= (uint32_t)c.kind << (2 * n);
        n++;
        w += words;
        p += len;
    }
    d->nargs = (uint8_t)n;
    d->nw = (uint8_t)w;
    d->ok = 1;
}

/* Producer: copy the arguments of ap into the slot words w and the string area str (the string area fill is *slen).
 * Only for a format with ok 1. The caller consumed nothing else from ap. */
static void lf_copy(uint32_t* w, char* str, uint16_t* slen, const LfDesc* d, va_list ap) {
    unsigned k = 0, sl = *slen;
    for (unsigned i = 0; i < d->nargs; i++) {
        unsigned kind = (d->kinds >> (2 * i)) & 3u;
        if (kind == LF_WORD) {
            w[k++] = (uint32_t)va_arg(ap, unsigned);
        } else if (kind == LF_LL) {
            unsigned long long v = va_arg(ap, unsigned long long);
            w[k++] = (uint32_t)v;
            w[k++] = (uint32_t)(v >> 32);
        } else if (kind == LF_DBL) {
            double x = va_arg(ap, double);
            uint64_t v;
            memcpy(&v, &x, 8);
            w[k++] = (uint32_t)v;
            w[k++] = (uint32_t)(v >> 32);
        } else {
            const char* s = va_arg(ap, const char*);
            unsigned room = LF_STR_SIZE - sl;
            if (!s) s = "(null)";
            if (room == 0) { w[k++] = LF_STR_NONE; continue; }
            unsigned n = (unsigned)strlen(s); /* newlib strlen: word at a time; the copy is cut at room - 1 chars below */
            if (n > room - 1) n = room - 1;   /* the NUL fits */
            w[k++] = sl;
            memcpy(str + sl, s, n);
            str[sl + n] = 0;
            sl += n + 1;
        }
    }
    *slen = (uint16_t)sl;
}

/* Writer: format the slot arguments w and str with the format f into out (cap bytes, NUL ended). Returns the chars written.
 * Each conversion goes through snprintf with its own spec text (star values inserted) and the typed argument, so the text is the
 * text vsnprintf makes. A precision star with a negative value drops the '.' (C rule: negative precision is as if omitted). */
static unsigned lf_format(char* out, unsigned cap, const char* f, const uint32_t* w, const char* str) {
    unsigned o = 0, wi = 0;
    if (!cap) return 0;
    for (const char* p = f; *p;) {
        if (*p != '%') { if (o + 1 < cap) out[o++] = *p; p++; continue; }
        if (p[1] == '%') { if (o + 1 < cap) out[o++] = '%'; p += 2; continue; }
        LfConv c;
        int len = lf_conv(p, &c);
        if (!len) break; /* lf_scan accepted the format: not reached */
        char sp[LF_MAX_SPEC + 16 * 2 + 4];
        int sn = 0;
        for (int i = 0; i < len; i++) {
            char ch = p[i];
            if (ch == '*') {
                int v = (int)w[wi++];
                if (i > 0 && p[i - 1] == '.' && v < 0) { sn--; continue; }
                sn += snprintf(sp + sn, sizeof sp - (unsigned)sn, "%d", v);
                continue;
            }
            sp[sn++] = ch;
        }
        sp[sn] = 0;
        unsigned room = cap - o;
        int r = 0;
        switch (c.kind) {
        case LF_WORD:
            if (c.conv == 'p') r = snprintf(out + o, room, sp, (void*)(uintptr_t)w[wi]);
            else if (c.conv == 'u' || c.conv == 'o' || c.conv == 'x' || c.conv == 'X') r = snprintf(out + o, room, sp, (unsigned)w[wi]);
            else r = snprintf(out + o, room, sp, (int)w[wi]);
            wi++;
            break;
        case LF_LL: {
            uint64_t v = (uint64_t)w[wi] | ((uint64_t)w[wi + 1] << 32);
            wi += 2;
            if (c.conv == 'd' || c.conv == 'i') r = snprintf(out + o, room, sp, (long long)v);
            else r = snprintf(out + o, room, sp, (unsigned long long)v);
            break;
        }
        case LF_DBL: {
            uint64_t b = (uint64_t)w[wi] | ((uint64_t)w[wi + 1] << 32);
            double x;
            wi += 2;
            memcpy(&x, &b, 8);
            r = snprintf(out + o, room, sp, x);
            break;
        }
        default: {
            uint32_t off = w[wi++];
            const char* s = off < LF_STR_SIZE ? str + off : "";
            r = snprintf(out + o, room, sp, s);
            break;
        }
        }
        if (r < 0) r = 0;
        if ((unsigned)r < room) o += (unsigned)r;
        else o = cap - 1; /* cut: the NUL is at cap - 1 */
        p += len;
    }
    out[o] = 0;
    return o;
}

#endif
