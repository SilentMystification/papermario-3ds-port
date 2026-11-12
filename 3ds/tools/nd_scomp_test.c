/* nd_scomp_test.c: host test of the state compiler key (S6T, nd_scomp.h). Two runs of a def that differ only in an inherited field the def
 * reads get different entry ids; two runs that differ only in a field the def sets get the same id. Also the def analysis (value units,
 * refusal). Built with -DND_SCOMP_HOST (the pure part of nd_scomp.c). */
#include <stdio.h>
#include <string.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_op.h"
#include "nd_scomp.h"

static int fails;
static void check(const char* what, int ok) {
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) fails++;
}
#define OM_A(sft, len) ((uint32_t)(sft) | (uint32_t)(len) << 8)
/* cam_main-like: viewport (value), clear geometry, texture off, cycle type, scissor (value), combine SHADE, render mode, matrix (value) */
static const uint32_t cam_u[] = {
    NDO_W0(NDO_MOVEMEM, G_MV_VIEWPORT), 0,
    NDO_W0(NDO_SCISSOR, 0), 0,
    NDO_W0(NDO_GEOM, ~(uint32_t)(G_SHADE | G_LIGHTING)), 0,
    NDO_W0(NDO_TEXTURE, 0), 0,
    NDO_W0(NDO_OMH, OM_A(G_MDSFT_CYCLETYPE, 2)), G_CYC_1CYCLE,
    ND_NOP_A, 0,
    NDO_W0(NDO_COMB, 0xfffe7e), 0xfffffbfdu,
    NDO_W0(NDO_OML, OM_A(3, 29)), 0x0f0a4000u,
    NDO_W0(NDO_MW, NDO_MW_NONE << 16), 0,
    NDO_W0(NDO_MTX, 0x07), 0,
};
/* status_icons-like: scissor (value), cycle type, texture on, alpha compare; no combine */
static const uint32_t ico_u[] = {
    NDO_W0(NDO_SCISSOR, 0), 0,
    NDO_W0(NDO_OMH, OM_A(G_MDSFT_CYCLETYPE, 2)), G_CYC_1CYCLE,
    NDO_W0(NDO_OML, OM_A(0, 2)), 0,
    NDO_W0(NDO_TEXTURE, 1), 0xffffffffu,
};
static const uint32_t bad_u[] = {
    NDO_W0(NDO_OMH, OM_A(G_MDSFT_CYCLETYPE, 2)), G_CYC_1CYCLE,
    NDO_W0(NDO_VTX, 0), 0,
};
/* SCOMP0: primitive colour before the first static unit (a value unit, ordered before the static writes) */
static const uint32_t pre_u[] = {
    NDO_W0(NDO_PRIM, 0), 0x11223344u,
    NDO_W0(NDO_OMH, OM_A(G_MDSFT_CYCLETYPE, 2)), G_CYC_1CYCLE,
};
/* SCOMP0: primitive colour between two static units: refused */
static const uint32_t mid_u[] = {
    NDO_W0(NDO_OMH, OM_A(G_MDSFT_CYCLETYPE, 2)), G_CYC_1CYCLE,
    NDO_W0(NDO_PRIM, 0), 0x11223344u,
    NDO_W0(NDO_OML, OM_A(0, 2)), 0,
};
/* S6VORD: a scissor (class 2 value) between two static units: refused */
static const uint32_t mid2_u[] = {
    NDO_W0(NDO_OMH, OM_A(G_MDSFT_CYCLETYPE, 2)), G_CYC_1CYCLE,
    NDO_W0(NDO_SCISSOR, 0), 0,
    NDO_W0(NDO_OML, OM_A(0, 2)), 0,
};
/* S6VORD: MW none is a no-op (dropped); the matrix after it is a value unit before the first static */
static const uint32_t mwn_u[] = {
    NDO_W0(NDO_MW, NDO_MW_NONE << 16), 0,
    NDO_W0(NDO_MTX, 0x07), 0,
    NDO_W0(NDO_OMH, OM_A(G_MDSFT_CYCLETYPE, 2)), G_CYC_1CYCLE,
};
/* S6SYNC: the render mode list form {OML, OMH cycle, GEOM, GEOM}: one sync unit (the cycle), index 1 */
static const uint32_t rml_u[] = {
    NDO_W0(NDO_OML, OM_A(0, 2)), 0,
    NDO_W0(NDO_OMH, OM_A(G_MDSFT_CYCLETYPE, 2)), G_CYC_1CYCLE,
    NDO_W0(NDO_GEOM, ~(uint32_t)G_CULL_BOTH), 0,
    NDO_W0(NDO_GEOM, ~(uint32_t)0), G_ZBUFFER,
};
/* SCOMP0: texture setters after the last static unit: value units, after the combine */
static const uint32_t tex_u[] = {
    NDO_W0(NDO_COMB, 0xfffe7e), 0xfffffbfdu,
    NDO_W0(NDO_TIMG, 0), 0,
    NDO_W0(NDO_TILE, 0), 0,
};
static void base_in(NdCombIn* in) {
    memset(in, 0, sizeof *in);
    in->w0 = 0xfc121824u; in->w1 = 0xff33ffffu; in->omode_h = 0x00102cf0u; in->prim = 0xff0000ffu; in->env = 0x00ff00ffu; in->prim_lo8 = 0x40u;
    in->key_c = 1; in->key_s = 2; in->conv_k4 = 3;
    for (unsigned i = 0; i < 16u; i++) in->tst[i] = i;
    in->tex_on = 1; in->use_t1 = 0; in->comb_set = 1;
}
static unsigned id_of(NdSTab* t, const NdSDef* d, const NdCombIn* in, uint32_t vprim, int* miss) {
    uint32_t k[ND_SC_KEYW] = { 0 };
    nd_scomp_key(d, in, vprim, k);
    return nd_scomp_find(t, d->id, k, miss);
}
int main(void) {
    static NdSTab t;
    NdSDef cam = { cam_u, sizeof cam_u / 8u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 } };
    NdSDef ico = { ico_u, sizeof ico_u / 8u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 } };
    NdSDef bad = { bad_u, sizeof bad_u / 8u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 } };
    NdSDef pre = { pre_u, sizeof pre_u / 8u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 } };
    NdSDef mid = { mid_u, sizeof mid_u / 8u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 } };
    NdSDef tex = { tex_u, sizeof tex_u / 8u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 } };
    NdSDef mid2 = { mid2_u, sizeof mid2_u / 8u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 } };
    NdSDef mwn = { mwn_u, sizeof mwn_u / 8u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 } };
    NdSDef rml = { rml_u, sizeof rml_u / 8u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 } };
    NdCombIn a, b;
    int m1, m2;
    unsigned i1, i2;
    check("cam def compiles", nd_scomp_analyse(&cam) == 1);
    check("cam value units 3 (viewport, scissor, matrix), 2 before the first static", cam.nv == 3u && cam.pre == 2u && cam.vmask == (1u | 1u << 1 | 1u << 9));
    check("cam folds: cycle mask, render mode mask, geometry clear, combine, texture",
          cam.omh_m == (3u << G_MDSFT_CYCLETYPE) && cam.oml_m == 0xfffffff8u && cam.has_comb && cam.has_tex && cam.has_geom &&
          cam.geom_c == (~(uint32_t)(G_SHADE | G_LIGHTING) & 0xffffffu) && cam.cw0 == 0xfcfffe7eu);
    check("ico def compiles", nd_scomp_analyse(&ico) == 1 && ico.nv == 1u && ico.pre == 1u && !ico.has_comb);
    check("a def with a vertex load is refused", nd_scomp_analyse(&bad) == 0);
    check("prim before the first static compiles", nd_scomp_analyse(&pre) == 1 && pre.nv == 1u && pre.pre == 1u);
    check("prim between static units is refused", nd_scomp_analyse(&mid) == 0);
    check("texture units after the last static compile", nd_scomp_analyse(&tex) == 1 && tex.nv == 2u && tex.pre == 0u);
    check("scissor between static units is refused", nd_scomp_analyse(&mid2) == 0);
    check("MW none is a no-op: the matrix is a pre value", nd_scomp_analyse(&mwn) == 1 && mwn.nv == 1u && mwn.pre == 1u);
    check("RM list form: one sync unit at index 1", nd_scomp_analyse(&rml) == 1 && rml.nsync == 1u && rml.sync_k[0] == 1u);
    check("cam sync units: texture, cycle type, combine", cam.nsync == 3u);
    /* cam: the combine words are set by the def, prim is inherited and read by the decision */
    base_in(&a); i1 = id_of(&t, &cam, &a, 0, &m1);
    check("first run of a key is a miss", m1 == 1);
    b = a; i2 = id_of(&t, &cam, &b, 0, &m2);
    check("same inputs: same id, a hit", i2 == i1 && m2 == 0);
    b = a; b.prim = 0x12345678u; i2 = id_of(&t, &cam, &b, 0, &m2);
    check("cam: inherited prim (read) differs: other id", i2 != i1 && m2 == 1);
    b = a; b.env ^= 0x100u; i2 = id_of(&t, &cam, &b, 0, &m2);
    check("cam: inherited env (read) differs: other id", i2 != i1);
    b = a; i2 = id_of(&t, &cam, &b, 1, &m2);
    check("cam: inherited vprim (read) differs: other id", i2 != i1);
    b = a; b.w0 ^= 0x0f0000u; b.w1 ^= 0x1u; b.tst[3] = 99; b.use_t1 = 1; b.comb_set = 0; b.box_folded = 1; i2 = id_of(&t, &cam, &b, 0, &m2);
    check("cam: combine words and fields (set by the def) differ: same id", i2 == i1 && m2 == 0);
    b = a; b.omode_h = (b.omode_h & ~(3u << G_MDSFT_CYCLETYPE)) | (1u << G_MDSFT_CYCLETYPE); i2 = id_of(&t, &cam, &b, 0, &m2);
    check("cam: cycle type (set by the def) differs: same id", i2 == i1 && m2 == 0);
    b = a; b.tex_on = 0; i2 = id_of(&t, &cam, &b, 0, &m2);
    check("cam: tex_on (set by the def) differs: same id", i2 == i1 && m2 == 0);
    /* ico: no combine unit, so the combine words are inherited and read */
    base_in(&a); i1 = id_of(&t, &ico, &a, 0, &m1);
    check("ico: other def, other id", m1 == 1);
    b = a; b.w0 ^= 0x0f0000u; i2 = id_of(&t, &ico, &b, 0, &m2);
    check("ico: inherited combine word (read) differs: other id", i2 != i1 && m2 == 1);
    b = a; b.tst[5] = 77; i2 = id_of(&t, &ico, &b, 0, &m2);
    check("ico: inherited combine field (read) differs: other id", i2 != i1);
    b = a; b.tex_on = 0; i2 = id_of(&t, &ico, &b, 0, &m2);
    check("ico: tex_on (set by the def) differs: same id", i2 == i1 && m2 == 0);
    b = a; b.omode_h ^= 3u << G_MDSFT_CYCLETYPE; i2 = id_of(&t, &ico, &b, 0, &m2);
    check("ico: cycle type (set by the def) differs: same id", i2 == i1 && m2 == 0);
    printf(fails ? "SCOMP TEST FAIL %d\n" : "SCOMP TEST PASS\n", fails);
    return fails ? 1 : 0;
}
