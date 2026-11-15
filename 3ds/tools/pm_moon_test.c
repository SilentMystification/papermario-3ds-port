#include <stdio.h>
#include "pm_moon.h"
int main(void) {
    static const int c[][3] = { {1,17,0}, {1,18,1}, {2,5,0}, {2,40,1}, {0,1,1}, {-1,1,1} }; /* boots, timeInAir, expected */
    for (unsigned i = 0; i < sizeof c / sizeof c[0]; i++)
        if (pm_moon_allowed(c[i][0], c[i][1]) != c[i][2]) { printf("moon FAIL case %u\n", i); return 1; }
    if (PM_MOON_AIR_TIME < PM_MOON_WIN_END || PM_MOON_AIR_TIME > 100) { printf("moon FAIL airtime\n"); return 1; }
    { static const float s[][3] = { {70.f, 4.f, 4.f}, {35.f, 4.f, 2.f}, {140.f, 4.f, 4.f} }; /* mag, run, expected */
      for (unsigned i = 0; i < 3; i++) { float d = pm_moon_air_speed(s[i][0], s[i][1]) - s[i][2]; if (d > 1e-4f || d < -1e-4f) { printf("moon FAIL speed %u\n", i); return 1; } } }
    printf("moon ok 10/10\n"); return 0;
}
