/* nd_st_test.c: host test of the state calls (nd_st.h) against the macros they replace (STUNIT). Each shape records the macro form under the
 * redirect of nd_gbi.h (A) and the setter form (B) in a custom gfx range. The two must agree on (1) the stream words from the mark to the
 * NDC_END word, (2) the unit log of nd_stream_run(mark), (3) the nd_r11_state result (return, cw, has_cb, nmo, mo). Prints "stunit ok 18/18",
 * or "stunit FAIL <name>" for the first shape that differs, and exits 1. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ultra64.h"
#include "variables.h"
#include "pm_port.h"
#include "gbi_custom.h"
#include "nd.h"
#include "nd_api.h"
#include "nd_gtab.h"
#include "nd_cmd.h"
#include "nd_gbi.h"
#include "nd_st.h"

/* the redirect target of the macro form: gMainGfxPos is the top-level packet pointer (nd_gbi.h D9) */
Gfx* gMainGfxPos;
s32 gCurrentDisplayContextIndex;
Gfx* ModelRenderModes[128]; /* nd_rec.c: the render mode table of src/model.c (61 lists), all NULL here */
int is_host(u32 p) { (void)p; return 0; }
char __end__[1];
u8 gMapShapeData[1];
void pm_log(const char* fmt, ...) { (void)fmt; }
void pm_log_e(const char* fmt, ...) { (void)fmt; }
void pm_log_evt(const char* fmt, ...) { (void)fmt; }
void pm_fatal_stop(const char* msg) { (void)msg; }
int pm_debug_has(const char* name) { (void)name; return 0; } /* the switches are off in the host test (nd_model.c PM5) */
unsigned pm_dbg_gen;
int pm_interp_repl;
const float (*pm_interp_find(const void* m))[4] { (void)m; return 0; }
uint32_t f32tof24(float f) { (void)f; return 0; } /* nd_tmpl.c: libctru, not used here */
void nd_walk_rec_model(const Gfx* q) { (void)q; }
void nd_walk_spr(const Gfx* q) { (void)q; }
void nd_walk_ent(const Gfx* q, unsigned words) { (void)q; (void)words; }
void nd_texrect_run(const uint32_t* p) { (void)p; }
void nd_parts_run(const uint32_t* rec) { (void)rec; }
void nd_parts_run_v(const uint32_t* rec) { (void)rec; }
void nd_lstcen(const void* l, unsigned kind) { (void)l; (void)kind; }
int nd_lc_frame = -1;
const NdGTab* nd_gtab_find(const void* list) { (void)list; return 0; }
uint32_t nd_mkey_a(const uint32_t* rec) { (void)rec; return 0; }

/* the walker side: nd_stream_run hands each state run to nd_walk_state; the double logs each unit (word pair) */
static uint32_t ulog[512]; static unsigned nulog;
void nd_walk_state(const uint32_t* q, unsigned n) {
    for (unsigned i = 0; i < n && nulog < 256u; i++) { ulog[2u * nulog] = q[2u * i]; ulog[2u * nulog + 1u] = q[2u * i + 1u]; nulog++; }
}

/* nd_model.c defines nd_r11_state (MDL-5f); nd_recs.c reads it */
int nd_r11_state(uint32_t addr, const uint32_t** nr, uint32_t* cw, int* has_cb, uint32_t* mo, unsigned* nmo);

/* the macro form (A) and the setter form (B) of each shape: the fixtures are the root call sites (draw_img_util.c:21, :26, :27) */
static void m_rm(void) { gDPSetRenderMode(gMainGfxPos++, G_RM_TEX_EDGE, G_RM_TEX_EDGE2); }
static void n_rm(void) { nd_pso_rm(G_RM_TEX_EDGE, G_RM_TEX_EDGE2); }
static void m_cyc(void) { gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE); }
static void n_cyc(void) { nd_pso_cyc(G_CYC_1CYCLE); }
static void m_filt(void) { gDPSetTextureFilter(gMainGfxPos++, G_TF_BILERP); }
static void n_filt(void) { nd_pso_filt(G_TF_BILERP); }
static void m_persp(void) { gDPSetTexturePersp(gMainGfxPos++, G_TP_PERSP); }
static void n_persp(void) { nd_pso_persp(G_TP_PERSP); }
static void m_lut(void) { gDPSetTextureLUT(gMainGfxPos++, G_TT_RGBA16); }
static void n_lut(void) { nd_pso_lut(G_TT_RGBA16); }
static void m_dither(void) { gDPSetColorDither(gMainGfxPos++, G_CD_DISABLE); }
static void n_dither(void) { nd_pso_dither(G_CD_DISABLE); }
static void m_alcmp(void) { gDPSetAlphaCompare(gMainGfxPos++, G_AC_THRESHOLD); }
static void n_alcmp(void) { nd_pso_alcmp(G_AC_THRESHOLD); }
static void m_aldith(void) { gDPSetAlphaDither(gMainGfxPos++, G_AD_NOISE); }
static void n_aldith(void) { nd_pso_aldith(G_AD_NOISE); }
static void m_lod(void) { gDPSetTextureLOD(gMainGfxPos++, G_TL_TILE); }
static void n_lod(void) { nd_pso_lod(G_TL_TILE); }
static void m_detail(void) { gDPSetTextureDetail(gMainGfxPos++, G_TD_CLAMP); }
static void n_detail(void) { nd_pso_detail(G_TD_CLAMP); }
static void m_conv(void) { gDPSetTextureConvert(gMainGfxPos++, G_TC_FILT); }
static void n_conv(void) { nd_pso_conv(G_TC_FILT); }
static void m_combkey(void) { gDPSetCombineKey(gMainGfxPos++, G_CK_NONE); }
static void n_combkey(void) { nd_pso_combkey(G_CK_NONE); }
static void m_cc(void) { gDPSetCombineMode(gMainGfxPos++, G_CC_DECALRGBA, G_CC_DECALRGBA); }
static void n_cc(void) { nd_pso_cc(G_CC_DECALRGBA, G_CC_DECALRGBA); }
static void m_som(void) { gSPSetOtherMode(gMainGfxPos++, G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, 0x00100000); }
static void n_som(void) { nd_pso_som(G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, 0x00100000); }
static void m_geom_set(void) { gSPSetGeometryMode(gMainGfxPos++, G_ZBUFFER); }
static void n_geom_set(void) { nd_pso_geom_set(G_ZBUFFER); }
static void m_geom_clr(void) { gSPClearGeometryMode(gMainGfxPos++, G_SHADE); }
static void n_geom_clr(void) { nd_pso_geom_clr(G_SHADE); }
static void m_tex(void) { gSPTexture(gMainGfxPos++, 0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON); }
static void n_tex(void) { nd_tex_on(0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON); }
static void m_scis(void) { gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, 10.0F, 20.0F, 300.0F, 200.0F); }
static void n_scis(void) { nd_pso_scis(G_SC_NON_INTERLACE, 10.0F, 20.0F, 300.0F, 200.0F); }

/* one run of a shape: the words of the range, its unit log, and the r11 result */
typedef struct {
    uint32_t w[256]; unsigned nw;
    uint32_t u[512]; unsigned nu;
    int r; uint32_t cw[2]; int hcb; uint32_t mo[8]; unsigned nmo;
} Res;

static void run_one(void (*fn)(void), Res* out) {
    memset(out, 0, sizeof *out);
    gMainGfxPos = nd_rec_task();
    const uint32_t* mk = nd_rec_mark();
    fn();
    nd_rec_end();
    const uint32_t* en = nd_rec_pos();
    out->nw = (unsigned)(en - mk);
    if (out->nw > 256u) out->nw = 256u;
    memcpy(out->w, mk, out->nw * sizeof(uint32_t));
    nulog = 0;
    nd_stream_run(mk);
    out->nu = nulog * 2u;
    memcpy(out->u, ulog, out->nu * sizeof(uint32_t));
    const uint32_t* nr = 0;
    out->r = nd_r11_state((uint32_t)(uintptr_t)mk, &nr, out->cw, &out->hcb, out->mo, &out->nmo);
}

static int same(const Res* a, const Res* b) {
    return a->nw == b->nw && memcmp(a->w, b->w, a->nw * sizeof(uint32_t)) == 0
        && a->nu == b->nu && memcmp(a->u, b->u, a->nu * sizeof(uint32_t)) == 0
        && a->r == b->r && memcmp(a->cw, b->cw, sizeof a->cw) == 0 && a->hcb == b->hcb
        && a->nmo == b->nmo && memcmp(a->mo, b->mo, sizeof a->mo) == 0;
}

typedef struct { const char* name; void (*mac)(void); void (*nat)(void); } Shape;
static const Shape shapes[] = {
    { "rm", m_rm, n_rm }, { "cyc", m_cyc, n_cyc }, { "filt", m_filt, n_filt }, { "persp", m_persp, n_persp },
    { "lut", m_lut, n_lut }, { "dither", m_dither, n_dither }, { "alcmp", m_alcmp, n_alcmp }, { "aldith", m_aldith, n_aldith },
    { "lod", m_lod, n_lod }, { "detail", m_detail, n_detail }, { "conv", m_conv, n_conv }, { "combkey", m_combkey, n_combkey },
    { "cc", m_cc, n_cc }, { "som", m_som, n_som }, { "geom_set", m_geom_set, n_geom_set }, { "geom_clr", m_geom_clr, n_geom_clr },
    { "tex", m_tex, n_tex }, { "scis", m_scis, n_scis },
};

int main(void) {
    static Res a, b;
    unsigned n = (unsigned)(sizeof shapes / sizeof shapes[0]), ok = 0;
    for (unsigned i = 0; i < n; i++) {
        run_one(shapes[i].mac, &a);
        run_one(shapes[i].nat, &b);
        if (!same(&a, &b)) { printf("stunit FAIL %s\n", shapes[i].name); return 1; }
        ok++;
    }
    printf("stunit ok %u/%u\n", ok, n);
    return 0;
}
