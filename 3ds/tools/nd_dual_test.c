/* COIN: host test of nd_dual_keep (nd_dual.h) and the item combine choice after a shaded sprite. Prints "coin ok 4/4" or "coin FAIL <case>". */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"
#include "nd_api.h"

uint32_t f32tof24(float vf) { /* host copy of libctru f32tof24, as 3ds/tools/nd_cmd_test.c */
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

void unpack8(unsigned c, float o[4]) {
    o[0] = (float)((c >> 24) & 255u) / 255.f; o[1] = (float)((c >> 16) & 255u) / 255.f;
    o[2] = (float)((c >> 8) & 255u) / 255.f;  o[3] = (float)(c & 255u) / 255.f;
}
#include "nd_dual.h"

/* 8 = plan with texel-only output, 9 = plan not texel only, else the legacy mode. dual_on is 1 before the combine; force keeps it. */
static int pick(uint32_t cw0, uint32_t cw1, uint32_t omh, int use_t1, int force) {
    static const float tx[4] = { 0.8f, 0.6f, 0.2f, 1.f }, sh[4] = { 0.f, 0.f, 0.f, 1.f }; NdPsoDesc d; NdPsoOut o; uint32_t t[16]; float out[4];
    memset(&d, 0, sizeof d); d.cw0 = cw0; d.cw1 = cw1; d.omh = omh; d.tex_on = 1; d.use_t1 = (uint8_t)use_t1; d.t1_valid = (uint8_t)use_t1; d.comb_set = 1; nd_comb_tst(cw0, cw1, use_t1, 0, t); d.dual_on = (uint8_t)(force || nd_dual_keep(t[0], t[2]));
    nd_pso_build(&d, &o); if (!o.comb.plan_on) return o.comb.leg;
    pm_comb_colours(&o.comb.plan, 0, 0, 0, 0, 0); pm_comb_eval(&o.comb.plan, tx, tx, sh, out);
    for (int i = 0; i < 4; i++) { if (out[i] - tx[i] > 0.002f || tx[i] - out[i] > 0.002f) return 9; } return 8;
}
int main(void) {
    if (pick(0xfcffffffu, 0xfffcf279u, 0u, 0, 0) != 8) { puts("coin FAIL decal"); return 1; }          /* G_CC_DECALRGBA x2, 1 cycle (item_entity.c:956) */
    if (pick(0xfcffffffu, 0xfffd7438u, 1u << 20, 1, 0) != 8) { puts("coin FAIL window5"); return 1; } /* PM_CC_WINDOW_5, G_CC_PASS2, 2 cycle (sprite_shading.c:433) */
    if (!nd_dual_keep(1u, 2u) || nd_dual_keep(15u, 31u)) { puts("coin FAIL keep"); return 1; }        /* PM_CC_50/51 keep it; DECALRGBA (15, 31) ends it */
    if (pick(0xfcffffffu, 0xfffcf279u, 0u, 0, 1) != 2) { puts("coin FAIL cause"); return 1; }          /* stale dual_on: legacy mode 2 */
    puts("coin ok 4/4"); return 0; }
