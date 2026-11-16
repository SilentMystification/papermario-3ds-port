#include <stdio.h>
#include "pm_moon.h"
/* MOON6: a flat-ground jump with A held, step by step as the game runs it: the apex check (src/7E9D0.c:243-248), timeInAir++ (src/world/action/jump.c:74,139),
 * the jump and fall integrator (src/7BB60.c:202-270, 448-461; values src/world/actions.c:45-46), the landing (y <= 0). It returns the first timeInAir at which the
 * moon jump may act, 0 when it never may before the landing, -1 when it may act before the apex (the window would move). land: timeInAir at the landing. */
static int arc_first(int boots, int* land) {
    static const float J[4] = { 15.7566404343f, -7.38624f, 3.44693994522f, -0.75f };
    static const float F[4] = { 0.154342994094f, -0.350080013275f, -0.182262003422f, 0.0115200001746f };
    float g[4], y = 0.0f, last = 0.0f;
    int tia = 0, peak = 0, fall = 0, k;
    for (k = 0; k < 4; k++) g[k] = J[k];
    for (;;) {
        if (tia != 0) { if (g[0] <= 0.0f && last > 0.0f) peak = tia; last = g[0]; }
        tia++;
        g[2] += g[3]; g[1] += g[2]; g[0] += g[1];
        if (!fall && g[0] <= 0.0f) { for (k = 0; k < 4; k++) g[k] = F[k]; g[2] += g[3]; g[1] += g[2]; g[0] += g[1]; fall = 1; }
        y += g[0];
        if (y <= 0.0f || tia > 60) { *land = tia; return 0; }
        if (pm_moon_allowed(boots, tia, peak)) return peak > 0 || boots < 1 ? tia : -1;
    }
}
int main(void) {
    static const int c[][4] = { {1,17,0,0}, {1,18,0,1}, {2,5,0,0}, {2,40,0,1}, {0,1,0,1}, {-1,1,0,1}, {1,8,7,1}, {2,3,2,1} }; /* boots, timeInAir, peakJumpTime, expected */
    int land = 0, first;
    for (int b = 1; b <= 2; b++) {
        first = arc_first(b, &land);
        if (first <= 0) { printf("moon FAIL arc boots %d first %d land %d\n", b, first, land); return 1; }
    }
    if (arc_first(0, &land) != 1) { printf("moon FAIL arc boots 0\n"); return 1; }
    for (unsigned i = 0; i < sizeof c / sizeof c[0]; i++)
        if (pm_moon_allowed(c[i][0], c[i][1], c[i][2]) != c[i][3]) { printf("moon FAIL case %u\n", i); return 1; }
    if (pm_moon_air_cap(5) != 5 || pm_moon_air_cap(19) != PM_MOON_AIR_TIME) { printf("moon FAIL cap\n"); return 1; }
    if (PM_MOON_AIR_TIME < PM_MOON_WIN_END || PM_MOON_AIR_TIME > 100) { printf("moon FAIL airtime\n"); return 1; }
    { static const float s[][3] = { {70.f, 4.f, 4.f}, {35.f, 4.f, 2.f}, {140.f, 4.f, 4.f} }; /* mag, run, expected */
      for (unsigned i = 0; i < 3; i++) { float d = pm_moon_air_speed(s[i][0], s[i][1]) - s[i][2]; if (d > 1e-4f || d < -1e-4f) { printf("moon FAIL speed %u\n", i); return 1; } } }
    first = arc_first(1, &land);
    printf("moon ok 16/16 (arc first %d)\n", first); return 0;
}
