/* nd_tx_test.c: host test of the texture, rectangle, viewport and colour setters (nd_tx.h) against the gbi macros they replace (TEXUNIT).
 * Each shape records the macro form under the redirect of nd_gbi.h (A) and the setter form (B) in a custom gfx range. The two must agree on
 * (1) the stream words from the mark to the NDC_END word, (2) the unit log of nd_stream_run(mark), (3) the nd_r11_state result.
 * Prints "texunit ok N/N", or "texunit FAIL <name>" for the first shape that differs, and exits 1. Shape arguments: fixed test values;
 * a local variable is a plain int, a pointer is a static array of this file. */
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
#include "nd_ds.h" /* NAT4A: the test state of run_one */
#include "nd_tx.h"

/* the redirect target of the macro form: gMainGfxPos is the top-level packet pointer (nd_gbi.h D9) */
Gfx* gMainGfxPos;
s32 gCurrentDisplayContextIndex;
const NdNDev* const nd_rm_dev[0x3d]; /* nd_rmtab.c: the render mode lists, all NULL here */
int pm_wide;
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
const NdGTab* nd_gtab_find(const void* list) { (void)list; return 0; }
uint32_t nd_mkey_a(const uint32_t* rec) { (void)rec; return 0; }

/* the walker side: nd_stream_run hands each state run to nd_walk_state; the double logs each unit (word pair) */
static uint32_t ulog[512]; static unsigned nulog;
void nd_nr_run(const uint32_t* q) { const unsigned n = (q[-2] >> 16) & 0xffu;
    for (unsigned i = 0; i < n && nulog < 256u; i++) { ulog[2u * nulog] = q[2u * i]; ulog[2u * nulog + 1u] = q[2u * i + 1u]; nulog++; }
}

/* nd_model.c defines nd_r11_state (MDL-5f); nd_recs.c reads it */
int nd_r11_state(uint32_t addr, const uint32_t** nr, uint32_t* cw, int* has_cb, uint32_t* mo, unsigned* nmo);

/* the fixtures of the shapes: a texture image, a viewport */
static uint8_t gTimg[1024];
static Vp gVp;
static Hilite ghl = { { 12, 34, 56, 78 } };

/* the macro form (A) and the setter form (B) of each shape. The syncs and G_NOOP, G_ENDDL have no setter output (the empty form). */
static void m_img(void) { gDPSetTextureImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 64, gTimg); }
static void n_img(void) { nd_tex_img(G_IM_FMT_RGBA, G_IM_SIZ_16b, 64, gTimg); }
static void m_tile(void) { gDPSetTile(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0x40, 0x100, G_TX_RENDERTILE, 3, G_TX_WRAP, 4, 1, G_TX_CLAMP, 2, 0); }
static void n_tile(void) { nd_tex_tile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0x40, 0x100, G_TX_RENDERTILE, 3, G_TX_WRAP, 4, 1, G_TX_CLAMP, 2, 0); }
static void m_ldtile(void) { gDPLoadTile(gMainGfxPos++, G_TX_LOADTILE, 0, 0, 127, 15); }
static void n_ldtile(void) { nd_tex_ldtile(G_TX_LOADTILE, 0, 0, 127, 15); }
static void m_tsz(void) { gDPSetTileSize(gMainGfxPos++, G_TX_RENDERTILE, 0, 0, 127 << 2, 15 << 2); }
static void n_tsz(void) { nd_tex_tsz(G_TX_RENDERTILE, 0, 0, 127 << 2, 15 << 2); }
static void m_hilite(void) { gDPSetHilite1Tile(gMainGfxPos++, G_TX_RENDERTILE, &ghl, 64, 64); gDPSetHilite2Tile(gMainGfxPos++, G_TX_MIRROR, &ghl, 64, 64); }
static void n_hilite(void) { nd_tex_hilite1(G_TX_RENDERTILE, &ghl, 64, 64); nd_tex_hilite2(G_TX_MIRROR, &ghl, 64, 64); }
static void m_ldblk(void) { gDPLoadBlock(gMainGfxPos++, G_TX_LOADTILE, 0, 0, 255, 0x80); }
static void n_ldblk(void) { nd_tex_ldblk(G_TX_LOADTILE, 0, 0, 255, 0x80); }
static void m_ldtlut(void) { gDPLoadTLUTCmd(gMainGfxPos++, G_TX_LOADTILE, 15); }
static void n_ldtlut(void) { nd_tex_ldtlut(G_TX_LOADTILE, 15); }
static void m_fill(void) { gDPSetFillColor(gMainGfxPos++, 0x12345678); }
static void n_fill(void) { nd_val_fill(0x12345678); }
static void m_fog(void) { gDPSetFogColor(gMainGfxPos++, 10, 20, 30, 40); }
static void n_fog(void) { nd_val_fog(10, 20, 30, 40); }
static void m_blend(void) { gDPSetBlendColor(gMainGfxPos++, 1, 2, 3, 4); }
static void n_blend(void) { nd_val_blend(1, 2, 3, 4); }
static void m_viewport(void) { gSPViewport(gMainGfxPos++, &gVp); }
static void n_viewport(void) { nd_xf_viewport(&gVp); }
static void m_fogpos(void) { gSPFogPosition(gMainGfxPos++, 900, 1000); }
static void n_fogpos(void) { nd_pso_fogpos(900, 1000); }
static void m_pipe(void) { gDPPipelineMode(gMainGfxPos++, G_PM_NPRIMITIVE); }
static void n_pipe(void) { nd_pso_pipe(G_PM_NPRIMITIVE); }
static void m_geom_load(void) { gSPLoadGeometryMode(gMainGfxPos++, G_ZBUFFER | G_SHADE); }
static void n_geom_load(void) { nd_pso_geom_load(G_ZBUFFER | G_SHADE); }
static void m_keyr(void) { gDPSetKeyR(gMainGfxPos++, 1, 2, 3); }
static void n_keyr(void) { nd_pso_keyr(1, 2, 3); }
static void m_keygb(void) { gDPSetKeyGB(gMainGfxPos++, 1, 2, 3, 4, 5, 6); }
static void n_keygb(void) { nd_pso_keygb(1, 2, 3, 4, 5, 6); }
static void m_scisf(void) { gDPSetScissorFrac(gMainGfxPos++, G_SC_NON_INTERLACE, 10, 20, 300, 200); }
static void n_scisf(void) { nd_pso_scisf(G_SC_NON_INTERLACE, 10, 20, 300, 200); }
static void m_rfill(void) { gDPFillRectangle(gMainGfxPos++, 0, 0, 319, 239); }
static void n_rfill(void) { nd_rect_fill(0, 0, 319, 239); }
static void m_rsfill(void) { gDPScisFillRectangle(gMainGfxPos++, 0, 0, 319, 239); }
static void n_rsfill(void) { nd_rect_sfill(0, 0, 319, 239); }
static void m_rtex(void) { gSPTextureRectangle(gMainGfxPos++, 10, 20, 300, 200, 0, 0x8000, 0x8000, 0x400, 0x400); }
static void n_rtex(void) { nd_rect_tex(10, 20, 300, 200, 0, 0x8000, 0x8000, 0x400, 0x400); }
static void m_rscis(void) { gSPScisTextureRectangle(gMainGfxPos++, 10, 20, 300, 200, 0, 0x8000, 0x8000, 0x400, 0x400); }
static void n_rscis(void) { nd_rect_scis(10, 20, 300, 200, 0, 0x8000, 0x8000, 0x400, 0x400); }
static void m_ltile(void) { gDPLoadTextureTile(gMainGfxPos++, gTimg, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, 0, 31, 31, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void n_ltile(void) { nd_tex_load_tile(gTimg, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, 0, 31, 31, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void m_ltile4(void) { gDPLoadTextureTile_4b(gMainGfxPos++, gTimg, G_IM_FMT_CI, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void n_ltile4(void) { nd_tex_load_tile_4b(gTimg, G_IM_FMT_CI, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void m_mtile(void) { gDPLoadMultiTile(gMainGfxPos++, gTimg, 0x100, 2, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, 0, 31, 31, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void n_mtile(void) { nd_tex_load_mtile(gTimg, 0x100, 2, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, 0, 31, 31, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void m_mtile4(void) { gDPLoadMultiTile_4b(gMainGfxPos++, gTimg, 0x100, 2, G_IM_FMT_CI, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void n_mtile4(void) { nd_tex_load_mtile_4b(gTimg, 0x100, 2, G_IM_FMT_CI, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void m_lblk(void) { gDPLoadTextureBlock(gMainGfxPos++, gTimg, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void n_lblk(void) { nd_tex_load_block(gTimg, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void m_lblk4(void) { gDPLoadTextureBlock_4b(gMainGfxPos++, gTimg, G_IM_FMT_CI, 32, 32, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void n_lblk4(void) { nd_tex_load_block_4b(gTimg, G_IM_FMT_CI, 32, 32, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void m_tlut16(void) { gDPLoadTLUT_pal16(gMainGfxPos++, 3, gTimg); }
static void n_tlut16(void) { nd_tex_load_tlut16(3, gTimg); }
static void m_tlut256(void) { gDPLoadTLUT_pal256(gMainGfxPos++, gTimg); }
static void n_tlut256(void) { nd_tex_load_tlut256(gTimg); }
static void m_smtile2(void) { gDPScrollMultiTile2_4b(gMainGfxPos++, gTimg, G_IM_FMT_CI, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD, 2, 3); }
static void n_smtile2(void) { nd_tex_scroll_mtile2_4b(gTimg, G_IM_FMT_CI, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD, 2, 3); }
static void m_stile4(void) { gDPScrollTextureTile_4b(gMainGfxPos++, gTimg, G_IM_FMT_CI, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD, 2, 3); }
static void n_stile4(void) { nd_tex_scroll_tile_4b(gTimg, G_IM_FMT_CI, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD, 2, 3); }
static void m_sblk4(void) { gDPScrollTextureBlock_4b(gMainGfxPos++, gTimg, G_IM_FMT_CI, 32, 32, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD, 2, 3); }
static void n_sblk4(void) { nd_tex_scroll_block_4b(gTimg, G_IM_FMT_CI, 32, 32, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD, 2, 3); }
static void m_loadsync(void) { gDPLoadSync(gMainGfxPos++); }
static void n_empty(void) { }
static void m_tilesync(void) { gDPTileSync(gMainGfxPos++); }
static void m_noop(void) { gDPNoOp(gMainGfxPos++); }
static void m_endlist(void) { gSPEndDisplayList(gMainGfxPos++); }

/* one run of a shape: the words of the range, its unit log, and the r11 result */
typedef struct {
    uint32_t w[256]; unsigned nw;
    uint32_t u[512]; unsigned nu;
    int r; uint32_t cw[2]; int hcb; uint32_t mo[8]; unsigned nmo;
} Res;

static void run_one(void (*fn)(void), Res* out) {
    memset(out, 0, sizeof *out);
    { static NdDs d0; static int have; if (!have) { d0 = nd_ds; have = 1; } nd_ds = d0; } /* NAT4A: each run starts from the same core 1 state */
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
    { "img", m_img, n_img }, { "tile", m_tile, n_tile }, { "ldtile", m_ldtile, n_ldtile }, { "tsz", m_tsz, n_tsz },
    { "ldblk", m_ldblk, n_ldblk }, { "ldtlut", m_ldtlut, n_ldtlut }, { "fill", m_fill, n_fill }, { "fog", m_fog, n_fog },
    { "blend", m_blend, n_blend }, { "viewport", m_viewport, n_viewport }, { "fogpos", m_fogpos, n_fogpos }, { "pipe", m_pipe, n_pipe },
    { "geom_load", m_geom_load, n_geom_load }, { "keyr", m_keyr, n_keyr }, { "keygb", m_keygb, n_keygb }, { "scisf", m_scisf, n_scisf },
    { "rfill", m_rfill, n_rfill }, { "rsfill", m_rsfill, n_rsfill }, { "rtex", m_rtex, n_rtex }, { "rscis", m_rscis, n_rscis },
    { "ltile", m_ltile, n_ltile }, { "ltile4", m_ltile4, n_ltile4 }, { "mtile", m_mtile, n_mtile }, { "mtile4", m_mtile4, n_mtile4 },
    { "lblk", m_lblk, n_lblk }, { "lblk4", m_lblk4, n_lblk4 }, { "tlut16", m_tlut16, n_tlut16 }, { "tlut256", m_tlut256, n_tlut256 },
    { "smtile2", m_smtile2, n_smtile2 }, { "stile4", m_stile4, n_stile4 }, { "sblk4", m_sblk4, n_sblk4 },
    { "loadsync", m_loadsync, n_empty }, { "tilesync", m_tilesync, n_empty }, { "noop", m_noop, n_empty }, { "endlist", m_endlist, n_empty },
    { "hilite", m_hilite, n_hilite },
};

int main(void) {
    static Res a, b;
    unsigned n = (unsigned)(sizeof shapes / sizeof shapes[0]), ok = 0;
    for (unsigned i = 0; i < n; i++) {
        run_one(shapes[i].mac, &a);
        run_one(shapes[i].nat, &b);
        if (!same(&a, &b)) { printf("texunit FAIL %s\n", shapes[i].name); return 1; }
        ok++;
    }
    printf("texunit ok %u/%u\n", ok, n);
    return 0;
}
