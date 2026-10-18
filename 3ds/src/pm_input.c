#include <3ds.h>
#include "pm_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

unsigned pm_pad_scans;
static unsigned short held;
static unsigned short down;
static unsigned short press_seen; /* presses since the last read: a tap shorter than one game frame still counts as held once */
static unsigned tap_injected, tap_seen;
static signed char stick_x;
static signed char stick_y;
static int rand_on, rand_x, rand_y; /* padrand switch */

/* padscript switch (slice W01): a scripted pad. `padscript=A,wait30,right*60,right+A*20` or `padscript=@file`
 * (file in the SD app folder, tokens split by comma, space or newline). A token is `wait<n>` (nothing pressed) or buttons joined
 * with +, then an optional *<n> scan count (default 1). Buttons: A B Z START L R dup ddown dleft dright, stick: up down left right.
 * The unit is a pad scan: the game reads the pad on every second scan, so 2 scans are 1 game frame (30 Hz). With `warp` the script
 * starts on the first frame of the scene (pm_padscript_start), without it on the first scan. After the last token the real pad is back. */
#define PS_MAX 450
static struct { unsigned short b; signed char x, y; unsigned short n; } ps[PS_MAX];
static int ps_n, ps_i, ps_run, ps_wait_start;
static unsigned ps_left;
static unsigned short ps_prev;

static int ps_button(const char* t, unsigned short* b, signed char* x, signed char* y) {
    static const struct { const char* n; unsigned short bit; } bt[] = {
        { "A", 0x8000 }, { "B", 0x4000 }, { "Z", 0x2000 }, { "START", 0x1000 }, { "L", 0x0020 }, { "R", 0x0010 },
        { "cdown", 0x0004 }, { "dup", 0x0800 }, { "ddown", 0x0400 }, { "dleft", 0x0200 }, { "dright", 0x0100 },
    };
    for (unsigned i = 0; i < sizeof(bt) / sizeof(bt[0]); i++) if (strcasecmp(t, bt[i].n) == 0) { *b |= bt[i].bit; return 1; }
    if (strcasecmp(t, "up") == 0) { *y = 80; return 1; }
    if (strcasecmp(t, "down") == 0) { *y = -80; return 1; }
    if (strcasecmp(t, "left") == 0) { *x = -80; return 1; }
    if (strcasecmp(t, "right") == 0) { *x = 80; return 1; }
    return 0;
}

static void ps_parse(char* s) {
    char* save = NULL;
    const char* sep = ", \r\n\t";
    for (char* tok = strtok_r(s, sep, &save); tok && ps_n < PS_MAX; tok = strtok_r(NULL, sep, &save)) {
        unsigned n = 1;
        char* star = strchr(tok, '*');
        if (star) { *star = 0; n = (unsigned)atoi(star + 1); }
        unsigned short b = 0;
        signed char x = 0, y = 0;
        if (strncasecmp(tok, "wait", 4) == 0) {
            n = (unsigned)atoi(tok + 4);
        } else {
            char* sv2 = NULL;
            for (char* p = strtok_r(tok, "+", &sv2); p; p = strtok_r(NULL, "+", &sv2))
                if (!ps_button(p, &b, &x, &y)) pm_log("padscript: unknown button '%s'", p);
        }
        if (n == 0) n = 1;
        ps[ps_n].b = b; ps[ps_n].x = x; ps[ps_n].y = y; ps[ps_n].n = (unsigned short)n;
        ps_n++;
    }
}

static void ps_load(void) {
    static char buf[3072]; /* slice S19d: was 1024, a battle script needs about 2.5 KB */
    char v[200]; /* was 40: a long inline script was cut after 39 characters */
    if (!pm_debug_value("padscript", v, sizeof v)) return;
    if (v[0] == '@') {
        FILE* f = fopen(v + 1, "rb");
        size_t n = f ? fread(buf, 1, sizeof(buf) - 1, f) : 0;
        if (f) fclose(f);
        buf[n] = 0;
        if (!f) pm_log("padscript: cannot open %s", v + 1);
    } else {
        strncpy(buf, v, sizeof buf - 1);
    }
    ps_parse(buf);
    ps_i = 0;
    ps_left = ps_n ? ps[0].n : 0;
    {
        char t[8];
        ps_wait_start = pm_debug_value("warp", t, sizeof t) || pm_debug_value("preset", t, sizeof t) || pm_debug_value("story", t, sizeof t);
    }
    ps_run = ps_n > 0 && !ps_wait_start;
    pm_log("padscript: %d steps, starts %s", ps_n, ps_wait_start ? "in the scene" : "now");
}

void pm_padscript_start(void) {
    if (ps_n && !ps_run && ps_wait_start && ps_i == 0) { ps_run = 1; pm_log("padscript: start at scan %u", pm_pad_scans); }
}

static int clamp_stick(int v) {
    v = v * 80 / 154;
    if (v > 80) v = 80;
    if (v < -80) v = -80;
    return v;
}

void pm_pad_scan(void) {
    hidScanInput();
    u32 k = hidKeysHeld();
    u32 d = hidKeysDown();
    unsigned short b = 0;
    if (k & KEY_A) b |= 0x8000;
    if (k & KEY_B) b |= 0x4000;
    if (k & (KEY_L | KEY_R)) b |= 0x2000; /* Z on L and R (C4E) */
    if (k & KEY_START) b |= 0x1000;
    if (k & (KEY_DUP | KEY_X)) b |= 0x0008;       /* D-pad is N64 C (C4E) */
    if (k & (KEY_DDOWN | KEY_Y)) b |= 0x0004;
    if (k & KEY_DLEFT) b |= 0x0002;
    if (k & KEY_DRIGHT) b |= 0x0001;
    if (k & KEY_ZL) b |= 0x0020;     /* L, New 3DS only */
    if (k & KEY_ZR) b |= 0x0010;     /* R, New 3DS only */
    if (k & KEY_CSTICK_UP) b |= 0x0008;
    if (k & KEY_CSTICK_DOWN) b |= 0x0004;
    if (k & KEY_CSTICK_LEFT) b |= 0x0002;
    if (k & KEY_CSTICK_RIGHT) b |= 0x0001;
    unsigned short edge = 0;
    if (d & KEY_A) edge |= 0x8000;
    if (d & KEY_B) edge |= 0x4000;
    if (d & (KEY_L | KEY_R)) edge |= 0x2000;
    if (d & KEY_START) edge |= 0x1000;
    if (d & (KEY_DUP | KEY_X)) edge |= 0x0008;
    if (d & (KEY_DDOWN | KEY_Y)) edge |= 0x0004;
    if (d & KEY_DLEFT) edge |= 0x0002;
    if (d & KEY_DRIGHT) edge |= 0x0001;
    if (d & KEY_ZL) edge |= 0x0020;
    if (d & KEY_ZR) edge |= 0x0010;
    if (d & KEY_CSTICK_UP) edge |= 0x0008;
    if (d & KEY_CSTICK_DOWN) edge |= 0x0004;
    if (d & KEY_CSTICK_LEFT) edge |= 0x0002;
    if (d & KEY_CSTICK_RIGHT) edge |= 0x0001;
    /* autoa switch: press A on two of every four scans, so Azahar runs leave the menus fast.
     * The old pattern was 1 scan in 240. */
    {
        if (pm_debug_has("autoa")) {
            if (pm_pad_scans % 4 < 2) {
                b |= 0x8000;
                edge |= 0x8000;
            }
        }
        /* taptest switch: a press that lasts one scan, once per 60 scans. Half of them land on a scan the game never reads. */
        if (pm_debug_has("taptest") && pm_pad_scans % 60 == 0) {
            b |= 0x8000;
            edge |= 0x8000;
            tap_injected++;
        }
        /* padrand switch (slice F01): a seeded random walk. The stick holds one direction for 20 to 60
         * scans, B and START and Z tap now and then. With autoa this jumps, bumps walls and walks into loading zones. */
        if (pm_debug_has("padrand")) {
            static unsigned rng = 12345, left;
            if (left == 0) {
                rng = rng * 1103515245u + 12345u;
                unsigned r = rng >> 8;
                left = 20 + r % 41;
                int dir = (r >> 6) % 9;
                rand_x = dir == 8 ? 0 : (int)(70 * __builtin_cosf(dir * 0.7853982f));
                rand_y = dir == 8 ? 0 : (int)(70 * __builtin_sinf(dir * 0.7853982f));
            }
            left--;
            rand_on = 1;
            if (pm_pad_scans % 97 < 3) { b |= 0x4000; edge |= 0x4000; }
            if (pm_pad_scans % 701 < 3) { b |= 0x1000; edge |= 0x1000; }
            if (pm_pad_scans % 211 < 3) { b |= 0x2000; edge |= 0x2000; }
        }
        {
            static int ps_init;
            if (!ps_init) { ps_init = 1; ps_load(); }
            if (ps_run) {
                b |= ps[ps_i].b;
                edge |= ps[ps_i].b & ~ps_prev;
                ps_prev = ps[ps_i].b;
                rand_x = ps[ps_i].x;
                rand_y = ps[ps_i].y;
                rand_on = 1;
                if (--ps_left == 0) {
                    if (++ps_i >= ps_n) { ps_run = 0; rand_on = 0; ps_prev = 0; pm_log("padscript: done at scan %u", pm_pad_scans); }
                    else ps_left = ps[ps_i].n;
                }
            }
        }
        /* faulttest switch (slice F01): one read of address 0x10 at scan 300, to check the fault detector */
        if (pm_debug_has("faulttest") && pm_pad_scans == 300) {
            pm_log("faulttest read 0x10");
            pm_log("faulttest value %u", *(volatile unsigned*)0x10);
        }
        pm_pad_scans++;
    }
    held = b;
    /* The game reads the pad every second retrace, the pad is scanned every retrace.
     * Keep every press edge until the game reads it. */
    down |= edge;
    press_seen |= edge;
    circlePosition c;
    hidCircleRead(&c);
    stick_x = (signed char)clamp_stick(c.dx);
    stick_y = (signed char)clamp_stick(c.dy);
    if (rand_on) {
        stick_x = (signed char)rand_x;
        stick_y = (signed char)rand_y;
    }
}

void pm_pad_read(unsigned short* buttons, signed char* x, signed char* y, unsigned short* trigger) {
    if (buttons) *buttons = held | press_seen;
    if (x) *x = stick_x;
    if (y) *y = stick_y;
    if (trigger) *trigger = down;
    if ((down & 0x8000) && pm_debug_has("taptest")) {
        tap_seen++;
        if (tap_seen % 5 == 0) pm_log("taptest injected %u seen %u", tap_injected, tap_seen);
    }
    down = 0;
    press_seen = 0;
}
