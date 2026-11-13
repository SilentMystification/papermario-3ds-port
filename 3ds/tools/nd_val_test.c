/* nd_val_test.c: host test of the value calls (nd_val.h) and the matrix calls (nd_xf.h) against the macros they replace (HUNIT). Each
 * shape records the macro form under the redirect of nd_gbi.h (A) and the native form (B) in a custom gfx range. The two must agree on
 * (1) the stream words from the mark to the NDC_END word, (2) the unit log of nd_stream_run(mark), (3) the nd_r11_state result (return,
 * cw, has_cb, nmo, mo). Prints "hunit ok 7/7", or "hunit FAIL <shape>" for the first shape that differs, and exits 1. */
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
#include "nd_val.h"
#include "nd_xf.h"

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

static const uint32_t* mptr = (const uint32_t*)0x00123450;

/* the macro form (A) and the native form (B) of each shape */
static void mac_push(unsigned p) { gSPMatrix(gMainGfxPos++, (void*)mptr, p); }
static void nat_push(unsigned p) { nd_xf_push((const void*)mptr, p); }
static void mac_pop(void) { gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW); }
static void nat_pop(void) { nd_xf_pop(G_MTX_MODELVIEW); }
static void mac_prim(void) { gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 128, 0); gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 128, 0); }
static void nat_prim(void) { nd_val_prim(0, 0, 0, 0, 128, 0); nd_val_prim(0, 0, 0, 0, 128, 0); }
static void mac_env(void) { gDPSetEnvColor(gMainGfxPos++, 10, 20, 30, 0); }
static void nat_env(void) { nd_val_env(10, 20, 30, 0); }
static void mac_seg(void) { gSPSegment(gMainGfxPos++, 0x09, 0x00123450); }
static void nat_seg(void) { nd_val_seg(0x09, 0x00123450); }
static void mac_tile(void) { gDPSetTileSize(gMainGfxPos++, 1, 5, 7, 5 + 124, 7 + 508); }
static void nat_tile(void) { nd_val_tile_size(1, 5, 7, 5 + 124, 7 + 508); }
static void mac_psync(void) { gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 128, 0); gDPPipeSync(gMainGfxPos++); gDPSetEnvColor(gMainGfxPos++, 10, 20, 30, 0); }
static void nat_psync(void) { nd_val_prim(0, 0, 0, 0, 128, 0); nd_val_env(10, 20, 30, 0); }

/* one run of a shape: the words of the range, its unit log, and the r11 result */
typedef struct {
    uint32_t w[256]; unsigned nw;
    uint32_t u[512]; unsigned nu;
    int r; uint32_t cw[2]; int hcb; uint32_t mo[8]; unsigned nmo;
} Res;

static void run_one(void (*fn)(void*), void* arg, Res* out) {
    memset(out, 0, sizeof *out);
    gMainGfxPos = nd_rec_task();
    const uint32_t* mk = nd_rec_mark();
    fn(arg);
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

/* the shape runners: the macro form and the native form take the same argument */
static void f_mac_push(void* a) { mac_push(*(unsigned*)a); }
static void f_nat_push(void* a) { nat_push(*(unsigned*)a); }
static void f_mac_pop(void* a) { (void)a; mac_pop(); }
static void f_nat_pop(void* a) { (void)a; nat_pop(); }
static void f_mac_prim(void* a) { (void)a; mac_prim(); }
static void f_nat_prim(void* a) { (void)a; nat_prim(); }
static void f_mac_env(void* a) { (void)a; mac_env(); }
static void f_nat_env(void* a) { (void)a; nat_env(); }
static void f_mac_seg(void* a) { (void)a; mac_seg(); }
static void f_nat_seg(void* a) { (void)a; nat_seg(); }
static void f_mac_tile(void* a) { (void)a; mac_tile(); }
static void f_nat_tile(void* a) { (void)a; nat_tile(); }
static void f_mac_psync(void* a) { (void)a; mac_psync(); }
static void f_nat_psync(void* a) { (void)a; nat_psync(); }

static int shape(const char* name, void (*fa)(void*), void (*fb)(void*), unsigned n) {
    static Res a, b;
    for (unsigned i = 0; i < n; i++) {
        unsigned arg = i;
        run_one(fa, &arg, &a);
        run_one(fb, &arg, &b);
        if (!same(&a, &b)) { printf("hunit FAIL %s\n", name); return 0; }
    }
    return 1;
}

int main(void) {
    if (!shape("push", f_mac_push, f_nat_push, 8u)) return 1;
    if (!shape("pop", f_mac_pop, f_nat_pop, 1u)) return 1;
    if (!shape("prim", f_mac_prim, f_nat_prim, 1u)) return 1;
    if (!shape("env", f_mac_env, f_nat_env, 1u)) return 1;
    if (!shape("seg", f_mac_seg, f_nat_seg, 1u)) return 1;
    if (!shape("tilesize", f_mac_tile, f_nat_tile, 1u)) return 1;
    if (!shape("pipesync", f_mac_psync, f_nat_psync, 1u)) return 1;
    printf("hunit ok 7/7\n");
    return 0;
}
