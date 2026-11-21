#include <stdio.h>
#include "pm_stamp.h"
/* MTX1 host test: pm_stamp_due over the modular reader counts of nd_wq.c. Each case: done, s, due. */
static const unsigned cs[6][3] = { {5u, 5u, 1u}, {5u, 6u, 0u}, {6u, 5u, 1u}, {0u, 0xffffffffu, 1u}, {0xffffffffu, 0u, 0u}, {2u, 0xfffffffeu, 1u} };
int main(void) {
    for (unsigned i = 0; i < 6u; i++) {
        if (pm_stamp_due(cs[i][0], cs[i][1]) != (int)cs[i][2]) { printf("stamp FAIL %u\n", i); return 1; }
    }
    printf("stamp ok\n");
    return 0;
}
