/* F03 host check: gcc -I3ds/include 3ds/tools/test_zenc.c -o zenc && ./zenc */
#include <stdio.h>
#include <stdlib.h>
#include "pm_depth.h"
int main(void) {
    static const int sh[8] = { 6, 5, 4, 3, 2, 1, 0, 0 };
    static const int bi[8] = { 0x00000, 0x20000, 0x30000, 0x38000, 0x3C000, 0x3E000, 0x3F000, 0x3F800 };
    int d, worst = 0;
    for (d = 0; d <= 32704; d++) {
        unsigned short w = pm_depth_encode(d);
        int dec = (((w & 0x1FFC) >> 2) << sh[w >> 13]) + bi[w >> 13];
        int err = abs((dec >> 3) - d);
        if (err > worst) worst = err;
    }
    printf("worst error %d\n", worst);
    return worst > 8;
}
