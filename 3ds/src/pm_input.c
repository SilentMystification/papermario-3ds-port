#include <3ds.h>
#include "pm_port.h"

unsigned pm_pad_scans;
static unsigned short held;
static unsigned short down;
static unsigned short press_seen; /* presses since the last read: a tap shorter than one game frame still counts as held once */
static unsigned tap_injected, tap_seen;
static signed char stick_x;
static signed char stick_y;
static int rand_on, rand_x, rand_y; /* padrand switch */

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
    if (k & KEY_L) b |= 0x2000;      /* Z */
    if (k & KEY_START) b |= 0x1000;
    if (k & KEY_DUP) b |= 0x0800;
    if (k & KEY_DDOWN) b |= 0x0400;
    if (k & KEY_DLEFT) b |= 0x0200;
    if (k & KEY_DRIGHT) b |= 0x0100;
    if (k & KEY_ZL) b |= 0x0020;     /* L */
    if (k & KEY_R) b |= 0x0010;      /* R */
    if (k & KEY_CSTICK_UP) b |= 0x0008;
    if (k & KEY_CSTICK_DOWN) b |= 0x0004;
    if (k & KEY_CSTICK_LEFT) b |= 0x0002;
    if (k & KEY_CSTICK_RIGHT) b |= 0x0001;
    unsigned short edge = 0;
    if (d & KEY_A) edge |= 0x8000;
    if (d & KEY_B) edge |= 0x4000;
    if (d & KEY_L) edge |= 0x2000;
    if (d & KEY_START) edge |= 0x1000;
    if (d & KEY_DUP) edge |= 0x0800;
    if (d & KEY_DDOWN) edge |= 0x0400;
    if (d & KEY_DLEFT) edge |= 0x0200;
    if (d & KEY_DRIGHT) edge |= 0x0100;
    if (d & KEY_ZL) edge |= 0x0020;
    if (d & KEY_R) edge |= 0x0010;
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
