#include <3ds.h>
#include "pm_port.h"

static unsigned short held;
static unsigned short down;
static signed char stick_x;
static signed char stick_y;

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
    held = b;
    down = edge;
    circlePosition c;
    hidCircleRead(&c);
    stick_x = (signed char)clamp_stick(c.dx);
    stick_y = (signed char)clamp_stick(c.dy);
}

void pm_pad_read(unsigned short* buttons, signed char* x, signed char* y, unsigned short* trigger) {
    if (buttons) *buttons = held;
    if (x) *x = stick_x;
    if (y) *y = stick_y;
    if (trigger) *trigger = down;
}
