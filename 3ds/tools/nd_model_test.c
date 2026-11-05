/* nd_model_test.c: host test of the S3N-1 compile tables in nd_model.c: the render mode list (nd_mode_compile, B5 and B6) and the texture handle
 * descriptor (nd_tex_handle_set, B4). Synthetic lists. Exit 1 on a failure. */
#include <stdio.h>
#include <string.h>
#include "nd.h"
#include "nd_ops.h"
#include "PR/mbi.h"
#include "PR/gbi.h"

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

    /* 2: cycle type not written (B8): compiles, the draw merges the rest */
    unsigned L2[2][2];
    oth(L2[0], OP_SETOTHERMODE_L, 0, 32, 0x33u); /* om_l only, om_h never written */
    L2[1][0] = OP_ENDDL << 24; L2[1][1] = 0;
    CHECK(nd_mode_compile(&m, &L2[0][0]) == 1 && m.ok == 1 && m.wh == 0 && m.wl == 0xFFFFFFFFu, "B8: partial list compiles, written masks kept");

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

    /* 6 (S3N-3a): one model record, 1CYC textured SURFACE_OPA: the key and the state words nd_raw_native would set (combine 8, B7) */
    {
        Gfx cg;
        unsigned L4[3][2], rec[28], rr[28];
        NdMode mode;
        NdUnit u0 = { 0x00200020u, 0x2206u, 0u, 0x0304b340u, 0x3u, 0u };
        NdGvP gv;
        NdMCtx c, c2;
        NdDraw dr;
        NdMState st;
        float db = 0.001f;
        oth(L4[0], OP_SETOTHERMODE_H, 20, 2, 0u);
        oth(L4[1], OP_SETOTHERMODE_L, 0, 32, 0x00FF1C33u | 0x10000000u);
        L4[2][0] = OP_ENDDL << 24; L4[2][1] = 0;
        CHECK(nd_mode_compile(&mode, &L4[0][0]) == 1, "3a: 1CYC mode compiles");
        gDPSetCombineMode(&cg, G_CC_MODULATERGBA, G_CC_MODULATERGBA);
        memset(rec, 0, sizeof rec); /* R3 flags 0: no fog, no prim, no env */
        memset(&gv, 0, sizeof gv);
        gv.uvso[0] = 0.5f; gv.uvso[1] = 0.25f; gv.uvso[2] = -1.f; gv.uvso[3] = 2.f;
        gv.cm[3] = 1.f; gv.ca[3] = 1.f; gv.zm[0] = 1.f;
        memset(&c, 0, sizeof c);
        c.mode = &mode; c.u0 = &u0; c.gv = &gv; c.has_comb = 1; c.comb_w0 = cg.words.w0; c.comb_w1 = cg.words.w1;
        c.z_on = 1; c.cull = 0; c.draw_3d = 1; c.dview = 1; c.rgb_scale = 1; c.r_mix = 0; c.r_memblend = 0; c.r_replace = 0;
        c.prim = 0xffffffffu; c.cconst = 0; c.mixcol = 0;
        c.known_h = 0x00FFFFFFu; c.known_l = 0xFFFFFFFFu; c.base_oh = 0; c.base_ol = 0; /* B8: nd_st has values for every read bit */
        memcpy(&c.dbias, &db, 4);
        memset(&dr, 0, sizeof dr);
        CHECK(nd_model_state(rec, &c, &dr, &st) == 1, "3a: 1CYC textured record draws natively");
        CHECK(st.k.combine == 8 && st.k.raw == 1 && st.k.pass == 0, "3a: key combine 8, raw, pass 0");
        CHECK(st.k.tex == 1 && st.k.tex1 == 0 && st.k.i8_0 == 0 && st.k.two == 0 && st.k.t1only == 0, "3a: key texture unit 0 only, 1CYC");
        CHECK(st.k.w0 == cg.words.w0 && st.k.w1 == cg.words.w1, "3a: key carries the combiner words");
        CHECK(st.k.z == 1 && st.k.zcmp == mode.r.zcmp && st.k.zupd == mode.r.zupd && st.k.r_blend == mode.r.blend, "3a: key depth and blend from the render mode");
        CHECK(st.k.draw_3d == 1 && st.k.rect_2d == 0 && st.k.off_active == 0 && st.k.off_i8 == 0, "3a: key draw flags");
        CHECK(dr.tmpl == st.tmpl && dr.tmpl >= 0, "3a: draw template id");
        CHECK(memcmp(&dr.u[0], &u0, sizeof u0) == 0, "3a: unit 0 words from the texture block");
        CHECK(dr.uvso[0] == 0.5f && dr.uvso[1] == 0.25f && dr.uvso[2] == -1.f && dr.uvso[3] == 2.f, "3a: uvso from gv");
        CHECK(dr.zmix[0] == 1.f && dr.zmix[1] == 0.f, "3a: zmix from gv");
        /* fallbacks */
        memcpy(rr, rec, sizeof rr); rr[26] = 1;
        CHECK(nd_model_state(rr, &c, &dr, &st) == 0, "3a: R13 aux words are fallback");
        c2 = c; c2.has_comb = 0;
        CHECK(nd_model_state(rec, &c2, &dr, &st) == 0, "3a: no combine entry is fallback");
        c2 = c; c2.mode = NULL;
        CHECK(nd_model_state(rec, &c2, &dr, &st) == 0, "3a: no render mode is fallback");
        c2 = c; gv.fxon = 1;
        CHECK(nd_model_state(rec, &c2, &dr, &st) == 0, "3a: lit or fog is fallback");
        gv.fxon = 0;
        memcpy(rr, rec, sizeof rr); rr[22] = 1;
        CHECK(nd_model_state(rr, &c, &dr, &st) == 0, "3a: custom gfx pre is fallback");
        { /* B8: a list that writes only om_l bits 16 to 23: the read bits 28 to 31 have no value (nd_st unknown) */
            unsigned L5[3][2];
            NdMode pm;
            oth(L5[0], OP_SETOTHERMODE_L, 16, 8, 0x40u << 16);
            L5[1][0] = OP_ENDDL << 24; L5[1][1] = 0;
            CHECK(nd_mode_compile(&pm, &L5[0][0]) == 1, "B8: partial list compiles");
            c2 = c; c2.mode = &pm; c2.known_l = 0x00FFFFFFu;
            CHECK(nd_model_state(rec, &c2, &dr, &st) == -1, "B8: read other mode bit with no value is nost");
            c2.known_l = 0xFFFFFFFFu;
            CHECK(nd_model_state(rec, &c2, &dr, &st) == 1, "B8: the same list with the bits known draws");
        }
    }

    printf("nd_model: %s\n", fails ? "FAIL" : "PASS");
    return fails != 0;
}
