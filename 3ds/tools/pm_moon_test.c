#include <stdio.h>
#include "pm_moon.h"
int main(void) {
    static const int c[][3] = { {1,17,0}, {1,18,1}, {2,5,0}, {2,40,1}, {0,1,1}, {-1,1,1} }; /* boots, timeInAir, expected */
    for (unsigned i = 0; i < sizeof c / sizeof c[0]; i++)
        if (pm_moon_allowed(c[i][0], c[i][1]) != c[i][2]) { printf("moon FAIL case %u\n", i); return 1; }
    if (PM_MOON_AIR_TIME < PM_MOON_WIN_END || PM_MOON_AIR_TIME > 100) { printf("moon FAIL airtime\n"); return 1; }
    printf("moon ok 7/7\n"); return 0;
}
