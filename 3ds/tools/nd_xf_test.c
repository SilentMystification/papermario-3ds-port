/* nd_xf_test.c: host test of nd_xf_push and nd_xf_pop (NXF1 row). Each call must record the same two words nd_op_xlate gives for the
 * gSPMatrix or gSPPopMatrix word of the same arguments. Prints "nxf ok 9/9" or "nxf FAIL <case>" and exits 1 on a failure. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ultra64.h"
#include "nd_op.h"
#include "nd_api.h"
#include "nd_xf.h"

uint32_t f32tof24(float vf) {
    union { float f; uint32_t i; } v;
    v.f = vf;
    uint32_t sign = v.i >> 31, mant = (v.i & 0x7FFFFFu) >> 7;
    int exp = (int)((v.i >> 23) & 0xFF);
    if (exp == 0) return sign << 23;
    exp = exp - 127 + 63;
    if (exp < 0) return sign << 23;
    if (exp > 0x7F) return sign << 23 | 0x7Fu << 16;
    return mant | (uint32_t)exp << 16 | sign << 23;
}

/* test doubles: the recorder keeps the units of the last call; the walker is not run here */
static uint32_t last[2]; static unsigned nlast;
void nd_rec_napi(NdNapiRun run, const void* rec, unsigned units) {
    (void)run;
    memcpy(last, rec, units * 8u);
    nlast = units * 2u;
}
void nd_walk_state(const uint32_t* q, unsigned n) { (void)q; (void)n; }

static int fails;
static void check(const char* name, const uint32_t want[2]) {
    if (nlast != 2u || last[0] != want[0] || last[1] != want[1]) {
        printf("nxf FAIL %s\n", name);
        fails++;
    }
}

int main(void) {
    static const unsigned ps[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    void* m = (void*)0x00123450;
    char name[32];
    unsigned i;
    for (i = 0; i < 8; i++) {
        Gfx g;
        uint32_t want[2];
        gSPMatrix(&g, m, ps[i]);
        nd_op_xlate(g.words.w0, g.words.w1, want);
        nd_xf_push(m, ps[i]);
        snprintf(name, sizeof name, "push p=%u", ps[i]);
        check(name, want);
    }
    {
        Gfx g;
        uint32_t want[2];
        gSPPopMatrix(&g, G_MTX_MODELVIEW);
        nd_op_xlate(g.words.w0, g.words.w1, want);
        nd_xf_pop(G_MTX_MODELVIEW);
        check("pop", want);
    }
    if (fails) return 1;
    printf("nxf ok 9/9\n");
    return 0;
}
