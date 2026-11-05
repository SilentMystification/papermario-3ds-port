/* nd_model_test.c: host test of the S3N-1 compile tables in nd_model.c: the render mode list (nd_mode_compile, B5 and B6) and the texture handle
 * descriptor (nd_tex_handle_set, B4). Synthetic lists. Exit 1 on a failure. */
#include <stdio.h>
#include <string.h>
#include "nd.h"
#include "nd_ops.h"

int pm_interp_repl;
const float (*pm_interp_find(const void* m))[4] { (void)m; return 0; }
uint32_t f32tof24(float f) { (void)f; return 0; } /* nd_tmpl.c: libctru, not used here */

static int fails;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL %s\n", msg); fails++; } } while (0)

/* SETOTHERMODE_H over bits [len, sft]: field = 32 - sft - len */
static void oth(unsigned* p, unsigned op, unsigned sft, unsigned len, unsigned v) {
    unsigned field = 32u - sft - len;
    p[0] = (op << 24) | (field << 8) | (len - 1u);
    p[1] = v;
}

int main(void) {
    unsigned L[3][2];
    NdMode m;
    NdRender want;

    /* 1: cycle type (om_h bits 20, 21 = 2 cycle) and the whole om_l written: ok, render state equal to nd_render_state */
    oth(L[0], OP_SETOTHERMODE_H, 20, 2, 1u << 20);
    oth(L[1], OP_SETOTHERMODE_L, 0, 32, 0x00FF1C33u | 0x10000000u);
    oth(L[2], OP_ENDDL, 0, 1, 0); L[2][0] = OP_ENDDL << 24; L[2][1] = 0;
    CHECK(nd_mode_compile(&m, &L[0][0]) == 1 && m.ok == 1, "mode 1 ok");
    nd_render_state(1u << 20, 0x00FF1C33u | 0x10000000u, 0, &want);
    CHECK(memcmp(&m.r, &want, sizeof want) == 0, "mode 1 render state equal");

    /* 2: cycle type not written (B5): fallback */
    unsigned L2[2][2];
    oth(L2[0], OP_SETOTHERMODE_L, 0, 32, 0x33u); /* om_l only, om_h never written */
    L2[1][0] = OP_ENDDL << 24; L2[1][1] = 0;
    CHECK(nd_mode_compile(&m, &L2[0][0]) == 0 && m.ok == 0, "B5: cycle type not written is fallback");

    /* 3: a geometry mode op in the list (B6): fallback */
    unsigned L3[3][2];
    L3[0][0] = OP_GEOMETRYMODE << 24; L3[0][1] = 0;
    oth(L3[1], OP_SETOTHERMODE_H, 20, 2, 0); /* not reached: the list stops at the geometry op */
    L3[2][0] = OP_ENDDL << 24; L3[2][1] = 0;
    CHECK(nd_mode_compile(&m, &L3[0][0]) == 0 && m.ok == 0, "B6: geometry op is fallback");

    /* 4: texture handle descriptor: common case set, found by index and list pointer */
    static const unsigned gfx_a[2] = { 0, 0 }, gfx_b[2] = { 1, 1 };
    static const unsigned raster[4] = { 0, 0, 0, 0 }, pal[8] = { 0 };
    nd_tex_handle_set(3, gfx_a, raster, 0, 32, 32, 0, 2, 2, 2, 5, 5, 0, 0);
    const NdTexDesc* d = nd_tex_handle_desc(3, gfx_a);
    CHECK(d != NULL && d->w == 32 && d->h == 32 && d->masks == 5 && d->maskt == 5 && d->siz == 2 && d->stride == 32 && d->pal_n == 0, "B4 common case set");
    CHECK(nd_tex_handle_desc(3, gfx_b) == NULL, "B4 other list pointer is not the handle");
    CHECK(nd_tex_handle_desc(4, gfx_a) == NULL, "B4 unset index is NULL");

    /* 5: CI4 with a palette: pal_n 16. Several tiles (extra 1) or an aux raster: fallback */
    nd_tex_handle_set(5, gfx_a, raster, pal, 16, 16, 0, 0, 0, 0, 4, 4, 0, 0);
    d = nd_tex_handle_desc(5, gfx_a);
    CHECK(d != NULL && d->pal_n == 16 && d->pal == (const void*)pal, "B4 CI4 palette count 16");
    nd_tex_handle_set(6, gfx_a, raster, 0, 16, 16, 0, 2, 0, 0, 4, 4, 1, 0);
    CHECK(nd_tex_handle_desc(6, gfx_a) == NULL, "B4 extra tiles is fallback");
    nd_tex_handle_set(7, gfx_a, raster, 0, 16, 16, 0, 2, 0, 0, 4, 4, 0, 1);
    CHECK(nd_tex_handle_desc(7, gfx_a) == NULL, "B4 aux raster is fallback");

    printf("nd_model: %s\n", fails ? "FAIL" : "PASS");
    return fails != 0;
}
