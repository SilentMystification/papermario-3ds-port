# ws_MOON6 code blocks (exact text, LF line ends)
## Block A: 3ds/src/pm_moon.h (replace the whole file)
```c
/* MOON6: the game's midair window (src/7BB60.c:1095-1097: peakJumpTime >= 6, timeInAir < 18, A pressed) has priority. It opens at the apex.
 * Before the apex (peakJumpTime 0) the moon jump waits: a moon rise there would move the apex. A press in an open window is the game's (spin or
 * tornado jump, PS_FLAG_FLYING), so the cheat does not see it. The moon hold does not lower timeInAir: the window closes at 18 as in a normal jump. */
#pragma once
#define PM_MOON_WIN_END 18  /* the window is timeInAir < 18 */
#define PM_MOON_AIR_TIME 18 /* the moon hold caps timeInAir here (src/77480.c:614 respawns above 100) */
static inline int pm_moon_allowed(int bootsLevel, int timeInAir, int peakJumpTime) { return bootsLevel < 1 || peakJumpTime > 0 || timeInAir >= PM_MOON_WIN_END; }
static inline int pm_moon_air_cap(int timeInAir) { return timeInAir > PM_MOON_AIR_TIME ? PM_MOON_AIR_TIME : timeInAir; }
#define PM_MOON_STICK_MAX 70.0f /* player_input_to_move_vector clamps the stick magnitude here (src/7B440.c:147) */
/* MOON5: the air speed of a moon jump: the run speed times the stick share */
static inline float pm_moon_air_speed(float mag, float run) { if (mag > PM_MOON_STICK_MAX) mag = PM_MOON_STICK_MAX; return run * mag * (1.0f / PM_MOON_STICK_MAX); }
```
## Block B: 3ds/tools/pm_moon_test.c (replace the whole file)
```c
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
```
## Block C: 3ds/tests/host.d/moon.sh (replace the whole file)
```sh
#!/bin/sh
# MOON6: host test of pm_moon.h (gate, flat-ground jump arc, timeInAir cap, stick speed). Each seeded fault must print its exact line.
mkdir -p /tmp/moon_seed
seed() {
    sed "$1" 3ds/src/pm_moon.h > /tmp/moon_seed/pm_moon.h
    gcc -m32 -Wall -I /tmp/moon_seed -o /tmp/moon_seed/t 3ds/tools/pm_moon_test.c || exit 1
    got=$(/tmp/moon_seed/t)
    if [ "$got" != "$2" ]; then echo "moon seed FAIL: [$1] got [$got]"; exit 1; fi
}
seed 's/ || peakJumpTime > 0//' 'moon FAIL arc boots 1 first 0 land 15'
seed 's/timeInAir >= PM_MOON_WIN_END/1/' 'moon FAIL arc boots 1 first -1 land 0'
seed 's/? PM_MOON_AIR_TIME : timeInAir/? timeInAir : timeInAir/' 'moon FAIL cap'
seed 's/if (mag > PM_MOON_STICK_MAX) mag = PM_MOON_STICK_MAX; //' 'moon FAIL speed 2'
gcc -m32 -Wall -I 3ds/src -o /tmp/moon_t 3ds/tools/pm_moon_test.c || exit 1
/tmp/moon_t || exit 1
```
FACT (oracle, docker pm3ds-host:1 on a scratch copy of blocks A to C): `moon ok 16/16 (arc first 8)`, rc 0. A wrong expected text fails with rc 1.
