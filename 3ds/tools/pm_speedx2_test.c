/* SPEEDX2: host test of pm_speedx2 (3ds/src/pm_speedx2.h). Base 4.0 and 8.0; 10 calls on give 8.0 and 16.0; one call off restores 4.0. */
#include <stdio.h>
#include "pm_speedx2.h"

int main(void) {
    PmSpd s = { 0.0f, 0.0f, 0 };
    float walk = 4.0f, run = 8.0f;
    int i;
    for (i = 0; i < 10; i++) pm_speedx2(&s, 1, &walk, &run);
    if (walk != 8.0f) { printf("speedx2 FAIL on walk got %f\n", walk); return 1; }
    if (run != 16.0f) { printf("speedx2 FAIL on run got %f\n", run); return 1; }
    pm_speedx2(&s, 0, &walk, &run);
    if (walk != 4.0f) { printf("speedx2 FAIL off walk got %f\n", walk); return 1; }
    if (run != 8.0f) { printf("speedx2 FAIL off run got %f\n", run); return 1; }
    printf("speedx2 ok\n");
    return 0;
}
