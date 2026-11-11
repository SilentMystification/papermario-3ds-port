/* nd_rec.c: the frame stream (DIRECT, plan_direct.md sections 2 and 3). Record side: the game's producers append native commands to the
 * stream of the display context being built (D9: no task list; the game writes no list word outside a list for later).
 * Walker side: nd_stream_run replays one task stream (core 0, pm_frame_walk).
 * Storage: per display context a chain of 32 KB heap chunks, allocated at first use and kept (no per-frame malloc, hw rule 7). The chain of
 * a context is reset when the game starts building that context again (gCurrentDisplayContextIndex changed since the last record), so a
 * stream lives exactly as long as its context (the depth-1 frame push of nd_wq.c keeps it safe). */
#include <stdlib.h>
#include <string.h>
#include "ultra64.h"
#include "variables.h"
#include "pm_port.h"
#include "nd.h"
#include "nd_cmd.h"
#include "nd_op.h"
#include "gbi_custom.h"
#include "nd_api.h"

extern s32 gCurrentDisplayContextIndex;
void nd_walk_rec_model(const Gfx* q);           /* nd_walk.c: a model record, as the walker's G_PM_REC arm */
void nd_walk_spr(const Gfx* q);                 /* nd_walk.c: a sprite record and its geometry words */
void nd_walk_ent(const Gfx* q, unsigned words); /* nd_walk.c: an entity record (and its G_DL) */
void nd_texrect_run(const uint32_t* p);
int is_host(u32 p);                             /* nd_walk.c: p is in host memory */         /* nd_tex_set.c: a texture rectangle */
void nd_walk_state(const uint32_t* q, unsigned n); /* nd_walk.c: a run of state ops */

#define CH_WORDS (32768u / 4u)
#define CH_MAX 16u
typedef struct { uint32_t* ch[CH_MAX]; unsigned nch, cur; } NdChain;
static NdChain chain[2];
static int rec_ctx = -1;   /* the context of the last record (-1: none yet) */
static uint32_t* wp;       /* write position */
static uint32_t* wend;     /* end of the current chunk, less room for one NDC_JUMP */
static uint32_t* task_head; /* first command of the open task (NULL: no task open) */
static unsigned hw_units;  /* PROF: the most 4-byte words of one context stream (the high-water mark) */
static unsigned ndc_n[NDC_N]; /* PROF: commands recorded per op (relaxed counters: the logic writes, the prof report reads) */
static unsigned ndc_sw;      /* PROF: state ops recorded (NDC_STATE units) */
static uint32_t* st_hdr;     /* D4b-3: the header of the open NDC_STATE run (rec_take clears it: any other record closes the run) */
static uint32_t* st_end;

static __attribute__((cold, noinline)) uint32_t* ch_get(NdChain* c, unsigned i) {
    if (i >= CH_MAX) { pm_log_e("ndc: stream over %u chunks", CH_MAX); pm_fatal_stop("ndc: frame stream full"); }
    if (i >= c->nch) {
        c->ch[i] = (uint32_t*)malloc(CH_WORDS * 4u);
        if (!c->ch[i]) pm_fatal_stop("ndc: no memory for a frame stream chunk");
        c->nch = i + 1u;
    }
    return c->ch[i];
}
static void ch_enter(NdChain* c, unsigned i) {
    c->cur = i;
    wp = ch_get(c, i);
    wend = wp + CH_WORDS - 2u;
}
/* HC4b (plan_hotcode 5.2): the recorder shadow. The recorder drops a typed op that the walker would skip (its old same-test true): per field
 * the value of the last op it wrote and a valid bit. Ops whose skip reads only the walker state they set (othermode L, fog, blend, geometry,
 * light colour) drop while the field is valid; ops whose skip also reads sc_ok and the combine input hash (othermode H, prim, env, texture,
 * combine) drop only while no word op came between (a draw, tile, load or matrix word may change those inputs: sh_sc) and sc_ok is known 1
 * (sc1: after an op that runs or skips with a sync). Every other command, a range mark and a task or context start reset it all (sh_all): the
 * walker state after them is not the recorded one (a model hit or miss, a native draw, a range replayed elsewhere). Core 1 data only. */
static struct {
    uint32_t omh_v, omh_k, oml_v, oml_k;  /* othermode value and the bits it holds */
    uint32_t geom[2], prim[2], tex[2], comb[2], env, fog, blend, lc[9];
    uint16_t lc_ok;                       /* light colour k valid (bit k) */
    uint8_t geom_ok, prim_ok, tex_ok, comb_ok, env_ok, fog_ok, blend_ok, sc1;
} sh;
static void sh_all(void) { memset(&sh, 0, sizeof sh); }
static void sh_sc(void) { sh.omh_k = 0; sh.prim_ok = sh.tex_ok = sh.comb_ok = sh.env_ok = sh.sc1 = 0; }
static int sh_unit(uint32_t* v, uint8_t* ok, const uint32_t* t) {
    if (*ok && v[0] == t[0] && v[1] == t[1]) return 1;
    v[0] = t[0]; v[1] = t[1]; *ok = 1;
    return 0;
}
/* 1: drop the typed op t (the walker would skip it); else the shadow takes it. */
static int sh_drop(const uint32_t* t) {
    const uint32_t a = t[0] & 0xffffffu, b = t[1];
    switch (t[0] >> 24) {
    case NDO_OMH: {
        const uint32_t m = nd_op_om_mask(a), sft = a & 0xffu, len = (a >> 8) & 0xffu;
        if (sh.sc1 && !(m & ~sh.omh_k) && !((b ^ sh.omh_v) & m)) return 1;
        sh.omh_v = (sh.omh_v & ~m) | (b & m); sh.omh_k |= m;
        if (sft <= G_MDSFT_CYCLETYPE && sft + len > G_MDSFT_CYCLETYPE) sh.sc1 = 1; /* the setter syncs the combine */
        else sh.tex_ok = sh.comb_ok = 0; /* the combine input hash may differ from the last sync's */
        return 0;
    }
    case NDO_OML: {
        const uint32_t m = nd_op_om_mask(a);
        if (!(m & ~sh.oml_k) && !((b ^ sh.oml_v) & m)) return 1;
        sh.oml_v = (sh.oml_v & ~m) | (b & m); sh.oml_k |= m;
        return 0;
    }
    case NDO_GEOM: return sh_unit(sh.geom, &sh.geom_ok, t);
    case NDO_PRIM: { const int d = sh_unit(sh.prim, &sh.prim_ok, t); sh.sc1 = 1; return d; }
    case NDO_TEXTURE: { const int d = sh_unit(sh.tex, &sh.tex_ok, t); sh.sc1 = 1; return d; }
    case NDO_COMB: { const int d = sh_unit(sh.comb, &sh.comb_ok, t); sh.sc1 = 1; return d; }
    case NDO_ENV:
        sh.sc1 = 1;
        if (sh.env_ok && sh.env == b) return 1;
        sh.env = b; sh.env_ok = 1;
        return 0;
    case NDO_FOG:
        if (sh.fog_ok && sh.fog == b) return 1;
        sh.fog = b; sh.fog_ok = 1;
        return 0;
    case NDO_BLEND:
        if (sh.blend_ok && sh.blend == b) return 1;
        sh.blend = b; sh.blend_ok = 1;
        return 0;
    case NDO_MW:
        if ((a >> 16) == NDO_MW_LIGHTCOL) {
            const unsigned k = a & 0xffffu;
            if ((sh.lc_ok >> k & 1u) && !((sh.lc[k] ^ b) & 0xffffff00u)) return 1;
            sh.lc[k] = b; sh.lc_ok |= (uint16_t)(1u << k);
        }
        return 0;
    case NDO_RARE: sh.sc1 = 1; return 0;
    case NDO_TIMG: case NDO_TILE: case NDO_TILESIZE: case NDO_LOADBLOCK: case NDO_LOADTILE: case NDO_LOADTLUT:
        sh_sc(); return 0; /* HC4c: a tile, load or image op may change the combine inputs (td[1].valid, dual_on) */
    case NDO_VTX: case NDO_TRI1: case NDO_TRI2: case NDO_MTX: case NDO_POPMTX:
        sh_sc(); return 0; /* HC4d: a draw or matrix op keeps the walker fields but may change the combine inputs (box_fold_sync, dual_bind) */
    case NDO_MOVEMEM: sh_all(); return 0; /* a light or viewport: the walker state after it is not followed */
    case NDO_CALL: case NDO_RECT: case NDO_MISC: sh_all(); return 0; /* HC4d2: a list call, rectangle, image or marker (as the word ops before) */
    default: return 0;
    }
}
/* A typed no-op unit (nd_op.h): a sync, G_SPNOOP, G_NOOP or a RDPHALF word. */
#define IS_NOP(w0) ((w0) == NDO_W0(NDO_MISC, NDO_MISC_NOP << 16))
static int t4_sw = -1; /* debug switch t4chk: a dropped op is written after NDO_DROP (nd_walk.c t4_check) */

/* HC4b (W3): the render mode table. Entry i holds the typed ops of ModelRenderModes[i] (src/model.c, 61 static lists) without the syncs,
 * then NDO_END. Built by the recorder at the first record of index i; the frame push publishes it to core 0; never written again. */
extern Gfx* ModelRenderModes[];
#define ND_RM_LISTS 0x3du /* RENDER_MODE_IDX_00 to RENDER_MODE_IDX_3C */
uint32_t nd_rm_tab[ND_RM_N][2u * ND_RM_OPS];
uint8_t nd_rm_n[ND_RM_N];
static uint8_t rm_built[ND_RM_N];
__attribute__((cold, noinline)) void nd_rm_build(unsigned i) {
    if (i >= ND_RM_N) return;
    unsigned n = 0;
    const Gfx* l = i < ND_RM_LISTS ? ModelRenderModes[i] : NULL;
    for (unsigned k = 0; l && n + 1u < ND_RM_OPS; k++) {
        const uint32_t w0 = l[k].words.w0, w1 = l[k].words.w1;
        if ((w0 >> 24) == NDO_END) break;
        if (IS_NOP(w0)) continue; /* a typed sync or no-op */
        nd_rm_tab[i][2u * n] = w0; nd_rm_tab[i][2u * n + 1u] = w1; /* HC4e4: ModelRenderModes is typed (gs forms) */
        n++;
    }
    nd_rm_tab[i][2u * n] = NDO_W0(NDO_END, 0);
    nd_rm_tab[i][2u * n + 1u] = 0;
    nd_rm_n[i] = (uint8_t)n;
    rm_built[i] = 1;
}

/* The context of the build: a new one resets its chain. */
static void rec_ctx_check(void) {
    int x = (int)(gCurrentDisplayContextIndex & 1);
    if (x == rec_ctx) return;
    rec_ctx = x;
    ch_enter(&chain[x], 0);
    task_head = NULL;
    st_hdr = NULL;
    sh_all();
}
/* Room for n words (header and payload): else the chunk ends with NDC_JUMP to the next one. */
static int st_take; /* HC4b: rec_take of a state op (the shadow stays) */
static uint32_t* rec_take(unsigned n) {
    st_hdr = NULL; /* any record closes the open state run (nd_rec_state reopens its own) */
    if (!st_take) sh_all(); /* HC4b: any other command resets the shadow */
    if (wp + n > wend) {
        NdChain* c = &chain[rec_ctx];
        uint32_t* j = wp;
        ch_enter(c, c->cur + 1u);
        j[0] = NDC_HDR(NDC_JUMP, 0, 0);
        j[1] = (uint32_t)(uintptr_t)wp;
    }
    uint32_t* p = wp;
    wp += n;
    {
        unsigned used = chain[rec_ctx].cur * CH_WORDS + (unsigned)(wp - chain[rec_ctx].ch[chain[rec_ctx].cur]);
        if (used > hw_units) hw_units = used;
    }
    return p;
}
static void task_open(void) {
    rec_ctx_check();
    if (!task_head) { task_head = wp; sh_all(); }
}
/* D9: the game writes no task list. At the top level gMainGfxPos names nd_rec_top: each redirected gbi macro takes its packet back
 * (nd_gx_w), so the pointer is back at nd_rec_top before every record. A pointer past it is a word that no redirect recorded: it is
 * logged (cold, 4 lines) and dropped. */
Gfx nd_rec_top[8];
static __attribute__((cold, noinline)) void stray_word(void) {
    static int n;
    const Gfx* g = nd_rec_top;
    if (n++ < 4) pm_log_e("ndc: %d list words outside the stream f%u: %08x %08x", (int)(gMainGfxPos - nd_rec_top), pm_frame_count(),
                          (unsigned)g->words.w0, (unsigned)g->words.w1);
    gMainGfxPos = nd_rec_top;
}
static void rec_open(void) {
    task_open();
    if (__builtin_expect(gMainGfxPos != nd_rec_top, 0)) stray_word();
}
Gfx* nd_rec_task(void) { return nd_rec_top; }
static __attribute__((cold, noinline)) void model_bad(unsigned words) {
    pm_log_e("ndc: model record of %u words (most %u)", words, G_PM_REC_WORDS + ND_REC_AUX_MAX);
    pm_fatal_stop("ndc: model record too long");
}
/* HC4c (W4): the payload holds typed ops: the panner tile sizes (R9, R10) and the R13 words. R13 w0 is the count of typed units.
 * HC4e4: the producer macros (nd_gbi.h) type every unit, so the payload is copied as it is.
 * R13NOP: the R13 syncs (typed NDO_MISC_NOP units) are dropped here; the count q[26] is the kept units. */
void nd_rec_model(const void* rec, unsigned words) {
    if (words > G_PM_REC_WORDS + ND_REC_AUX_MAX) model_bad(words);
    { const unsigned rmi = (((const uint32_t*)rec)[0] >> 16) & 0xffu; if (rmi < ND_RM_N && !rm_built[rmi]) nd_rm_build(rmi); } /* HC4b: W3 */
    const uint32_t* r = (const uint32_t*)rec; unsigned keep = 0;
    for (unsigned k = G_PM_REC_WORDS; k < words; k++) if (!IS_NOP(r[2 * k])) keep++; /* R13NOP: a typed sync is not an R13 op */
    rec_open(); uint32_t* p = rec_take(2u + 2u * (G_PM_REC_WORDS + keep)); ndc_n[NDC_MODEL]++;
    memcpy(p + 2, rec, G_PM_REC_WORDS * 8u); uint32_t* q = p + 2; uint32_t* o = q + 2 * G_PM_REC_WORDS;
    for (unsigned k = G_PM_REC_WORDS; k < words; k++) if (!IS_NOP(r[2 * k])) { o[0] = r[2 * k]; o[1] = r[2 * k + 1]; o += 2; }
    q[26] = keep; p[0] = NDC_HDR(NDC_MODEL, G_PM_REC_WORDS + keep, 0);
    p[1] = nd_mkey_a(q); /* HC6 step 2: key A of the model cache (the record part), hashed here on core 1 */
}
void nd_rec_spr(const void* rec, const void* geo) {
    rec_open();
    uint32_t* p = rec_take(2u + 2u * (G_PM_SPR_WORDS + 2u));
    ndc_n[NDC_SPR]++;
    p[0] = NDC_HDR(NDC_SPR, G_PM_SPR_WORDS + 2u, 0);
    p[1] = 0;
    memcpy(p + 2, rec, G_PM_SPR_WORDS * 8u);
    memcpy(p + 2 + 2 * G_PM_SPR_WORDS, geo, 16u);
}
/* D3: the draw record of a static list (gbi_custom.h gPmEntDrawRecord) and the G_DL of that list. form: record word 2 w0 (1: the animator form, log). */
void nd_rec_ent_draw(const void* list, unsigned segid, unsigned varray, unsigned form) {
    Gfx r[G_PM_ENT_WORDS + 1];
    gPmEntDrawRecord(r, list, segid, varray);
    r[2].words.w0 = form;
    r[G_PM_ENT_WORDS].words.w0 = ND_OP_CALL_W0; /* HC4d2: the list call as a typed op (W6) */
    r[G_PM_ENT_WORDS].words.w1 = (uint32_t)(uintptr_t)list;
    rec_open();
    uint32_t* p = rec_take(2u + 2u * (G_PM_ENT_WORDS + 1u));
    ndc_n[NDC_ENT]++;
    p[0] = NDC_HDR(NDC_ENT, G_PM_ENT_WORDS + 1u, 0);
    p[1] = 0;
    memcpy(p + 2, r, sizeof r);
}
void nd_rec_ent(const void* rec) {
    rec_open();
    uint32_t* p = rec_take(2u + 2u * G_PM_ENT_WORDS);
    ndc_n[NDC_ENT]++;
    p[0] = NDC_HDR(NDC_ENT, G_PM_ENT_WORDS, 0);
    p[1] = 0;
    memcpy(p + 2, rec, G_PM_ENT_WORDS * 8u);
}
void nd_rec_texrect(uint32_t w0, uint32_t w1, uint32_t hs, uint32_t hd) {
    rec_open();
    uint32_t* p = rec_take(6);
    ndc_n[NDC_TEXRECT]++;
    p[0] = NDC_HDR(NDC_TEXRECT, 2, 0);
    p[1] = 0;
    p[2] = w0;
    p[3] = w1;
    p[4] = hs;
    p[5] = hd;
}
/* NATIVEAPI: a native draw record (nd_api.h). units 1 to 255. */
static __attribute__((cold, noinline)) void napi_bad(unsigned units) {
    pm_log_e("ndc: native draw record of %u units (1 to 255)", units);
    pm_fatal_stop("ndc: bad native draw record");
}
void nd_rec_napi(NdNapiRun run, const void* rec, unsigned units) {
    if (units == 0u || units > 255u) napi_bad(units);
    rec_open();
    uint32_t* p = rec_take(2u + 2u * units);
    ndc_n[NDC_NAPI]++;
    p[0] = NDC_HDR(NDC_NAPI, units, 0);
    p[1] = (uint32_t)(uintptr_t)run;
    memcpy(p + 2, rec, units * 8u);
}
/* D4b-3: a state op (HC4a: typed by the recorder) extends the open NDC_STATE run when nothing was recorded after it (no span, no other command) and the chunk has room;
 * else it opens a new run. HC4b: a typed op the shadow drops is not written (t4chk: written after NDO_DROP). */
static void st_put(uint32_t w0, uint32_t w1) {
    uint32_t* h = st_hdr;
    st_take = 1;
    if (h && wp == st_end && ((h[0] >> 16) & 0xffu) < 255u && wp + 2u <= wend) {
        h[0] += 1u << 16;
        uint32_t* p = rec_take(2);
        st_take = 0;
        p[0] = w0;
        p[1] = w1;
        st_hdr = h;
        st_end = wp;
        return;
    }
    uint32_t* p = rec_take(4);
    st_take = 0;
    ndc_n[NDC_STATE]++;
    p[0] = NDC_HDR(NDC_STATE, 1, 0);
    p[1] = 0;
    p[2] = w0;
    p[3] = w1;
    st_hdr = p;
    st_end = wp;
}
/* HC4d2: a typed op into the open state run (the shadow decides a drop). */
static void rec_op_t(const uint32_t t[2]) {
    rec_open();
    ndc_sw++;
    if (sh_drop(t)) {
        if (t4_sw < 0) t4_sw = pm_debug_has("t4chk") ? 1 : 0;
        if (!t4_sw) return;
        st_put(NDO_W0(NDO_DROP, 0), 0);
    }
    st_put(t[0], t[1]);
}
static __attribute__((cold, noinline)) void word_bad(uint32_t w0) {
    static int k;
    if (k++ < 4) pm_log_e("ndc: untyped word %08x not recorded", (unsigned)w0);
}
/* HC4a: typed on the recorder (nd_op.h). HC4e4: an untyped word is not recorded. */
void nd_rec_state(uint32_t w0, uint32_t w1) {
    const uint32_t t[2] = { w0, w1 };
    if (__builtin_expect(!nd_op_typed(w0 >> 24), 0)) { word_bad(w0); return; }
    rec_op_t(t);
}
/* D4b-3, D5: the nd_gbi.h redirect. The packet gMainGfxPos++ is taken back and the op recorded; the RDP syncs are dropped (the walker skips
 * them). A texture rectangle (its op word and the two RDPHALF words, written by one macro) is held until its last word and recorded as
 * NDC_TEXRECT. Any other packet (a local list) and the branch form of G_DL keep the word; G_ENDDL at the top level is dropped (NDC_END ends a
 * task and a custom gfx range). */
/* DIRECT D11a: a push G_DL whose target is a static state list (in the program image, not a map shape list; to its G_ENDDL at most 64 ops,
 * each a state op of the executor: no list, vertex, triangle, rectangle, image or record op) is recorded in line: G_PM_LST and the list's
 * ops without the RDP syncs. The replay ran such a list on the op executor (walk_call: ent_gdl refuses a list without triangles, dlc_try a
 * list outside the map shapes; nd_ops_run with the sync state kept); G_PM_LST carries its list start (gv_epoch, ent_skipgen). The list is
 * read here on the recorder side, the replay reads no list. Any other target keeps its G_DL. */
extern char __end__[];
extern u8 gMapShapeData[];
/* The class of a typed list word for rec_inline: 0 not a state op, 1 a state op, 2 a sync (dropped), 3 the list end. */
static unsigned st_kind(uint32_t w0) {
    const unsigned op = w0 >> 24;
    if (!nd_op_typed(op)) return 0u; /* HC4e4: no raw word in a typed list */
    if (op == NDO_END) return 3u;
    if (op == NDO_MISC) return (w0 & 0xffffffu) == (NDO_MISC_NOP << 16) ? 2u : 0u;
    return (op <= NDO_COMB || nd_op_tex(op) || (op >= NDO_MTX && op <= NDO_MOVEMEM)) ? 1u : 0u;
}
static int rec_inline(uint32_t a) {
    if (a < 0x00100000u || a >= (uint32_t)(uintptr_t)__end__ || (a & 7u) || a - (uint32_t)(uintptr_t)gMapShapeData < 0x40000u) return 0;
    const Gfx* l = (const Gfx*)(uintptr_t)a;
    unsigned n = 0;
    for (;; n++) {
        if (n == 64u) return 0;
        const unsigned k = st_kind(l[n].words.w0);
        if (k == 3u) break;
        if (!k) return 0;
    }
    { static const uint32_t lst[2] = { NDO_W0(NDO_MISC, NDO_MISC_LST << 16), 0 }; rec_op_t(lst); }
    for (unsigned i = 0; i < n; i++) if (st_kind(l[i].words.w0) == 1u) nd_rec_state(l[i].words.w0, l[i].words.w1);
    return 1;
}
static uint32_t tr_w[3];
static unsigned tr_n; /* the words of the texture rectangle being recorded */
static __attribute__((cold, noinline)) void tr_bad(uint32_t w0) {
    pm_log_e("ndc: texture rectangle word %u followed by %08x", tr_n, (unsigned)w0);
    pm_fatal_stop("ndc: broken texture rectangle");
}
/* NAPI0 census (switch napicen): recorded typed ops per caller (ra) since the last 300 frame line */
typedef struct { uint32_t ra, n; } napi_e_t;
static napi_e_t* napi_tab;
static unsigned napi_next = 300u;
static __attribute__((cold, noinline)) void napi_dump(unsigned f) {
    unsigned total = 0;
    for (unsigned i = 0; i < 512u; i++) total += napi_tab[i].n;
    for (unsigned r = 0; r < 40u; r++) {
        unsigned b = 512u, bn = 0;
        for (unsigned i = 0; i < 512u; i++) if (napi_tab[i].n > bn) { bn = napi_tab[i].n; b = i; }
        if (b == 512u) break;
        pm_log("napicen f%u ra %08x n %u", f, (unsigned)napi_tab[b].ra, bn / 300u);
        napi_tab[b].n = 0;
    }
    pm_log_evt("napicen f%u done total %u", f, total / 300u);
    memset(napi_tab, 0, 512u * sizeof(napi_e_t));
}
static __attribute__((cold, noinline)) void napi_note(uintptr_t ra) {
    if (!napi_tab) napi_tab = (napi_e_t*)calloc(512u, sizeof(napi_e_t));
    if (!napi_tab) return;
    const unsigned f = pm_frame_count();
    if (f >= napi_next) { napi_next += 300u; napi_dump(f); }
    const uint32_t k = (uint32_t)ra;
    unsigned i = (k >> 2) & 511u;
    for (unsigned p = 0; p < 512u; p++, i = (i + 1u) & 511u) {
        if (napi_tab[i].ra == k) { napi_tab[i].n++; return; }
        if (napi_tab[i].ra == 0u) { napi_tab[i].ra = k; napi_tab[i].n = 1u; return; }
    }
}
void nd_gx_w(Gfx* g, uint32_t w0, uint32_t w1) {
    if (g != gMainGfxPos - 1) { g->words.w0 = w0; g->words.w1 = w1; return; }
    if (__builtin_expect(PMDL("napicen"), 0)) napi_note((uintptr_t)__builtin_return_address(0));
    const unsigned op = w0 >> 24;
    if (op == NDO_CALL && (w0 & 1u)) { g->words.w0 = w0; g->words.w1 = w1; return; } /* a branch: a unit of the list */
    gMainGfxPos = g;
    if (op == NDO_END) return; /* D7: the end of a task list or of a custom gfx builder (NDC_END closes both) */
    if (__builtin_expect(tr_n != 0, 0)) {
        if (tr_n == 1 && IS_NOP(w0)) { tr_w[2] = w1; tr_n = 2; return; } /* RDPHALF_1: s, t */
        if (tr_n == 2 && IS_NOP(w0)) { tr_n = 0; nd_rec_texrect(tr_w[0], tr_w[1], tr_w[2], w1); return; } /* RDPHALF_2: dsdx, dtdy */
        tr_bad(w0);
    }
    if (op == NDO_RECT && (w1 & NDO_RECT_TEX)) { /* the words nd_texrect_run reads: G_TEXRECT with the a field, w1 without the flag */
        tr_w[0] = ((uint32_t)G_TEXRECT << 24) | (w0 & 0xffffffu); tr_w[1] = w1 & ~NDO_RECT_TEX; tr_n = 1; return;
    }
    if (IS_NOP(w0)) return; /* the syncs are dropped (the walker skips them) */
    if (op == NDO_CALL && rec_inline(w1)) return;
    nd_rec_state(w0, w1);
}
const uint32_t* nd_rec_pos(void) { return wp; }
const uint32_t* nd_rec_mark(void) {
    rec_open();
    st_hdr = NULL; /* a range bound closes the state run: the range starts or ends at a command header */
    sh_all(); /* HC4b: a range replays elsewhere */
    return wp;
}
void nd_rec_call(const uint32_t* a, const uint32_t* b) {
    rec_open();
    uint32_t* p = rec_take(4);
    ndc_n[NDC_CALL]++;
    p[0] = NDC_HDR(NDC_CALL, 1, 0);
    p[1] = (uint32_t)(uintptr_t)a;
    p[2] = (uint32_t)(uintptr_t)b;
    p[3] = 0;
}
static __attribute__((cold, noinline)) void data_bad(unsigned bytes) {
    pm_log_e("ndc: inline data of %u bytes (most %u)", bytes, (CH_WORDS - 4u) * 4u);
    pm_fatal_stop("ndc: inline data too long");
}
void* nd_rec_data(unsigned bytes) {
    const unsigned units = (bytes + 7u) >> 3;
    if (2u + 2u * units > CH_WORDS - 4u) data_bad(bytes);
    rec_open();
    uint32_t* p = rec_take(2u + 2u * units);
    p[0] = NDC_HDR(NDC_JUMP, 0, 0);
    p[1] = (uint32_t)(uintptr_t)(p + 2u + 2u * units);
    return p + 2;
}
void nd_sub_open(NdSub* s) {
    rec_open();
    s->br = rec_take(2);
    s->br[0] = NDC_HDR(NDC_JUMP, 0, 0);
    s->br[1] = 0;
    s->a = nd_rec_mark();
}
void nd_sub_close(NdSub* s) {
    s->b = nd_rec_mark();
    s->br[1] = (uint32_t)(uintptr_t)s->b;
}
void nd_sub_call(const NdSub* s) { nd_rec_call(s->a, s->b); }
/* D10a: the end of a custom gfx range (build_custom_gfx): the replay of a range (nd_range_run) stops at its NDC_END. */
void nd_rec_end(void) {
    rec_open();
    uint32_t* e = rec_take(2);
    e[0] = NDC_HDR(NDC_END, 0, 0);
    e[1] = 0;
}
/* D10a: the ops of a static list (a custom gfx list that a map script set, SetCustomGfx) recorded as commands, as the walker would run the
 * list: the syncs dropped, a texture rectangle as NDC_TEXRECT, a branch followed, a push G_DL kept as a state op (walk_call at replay), the
 * list ends at its G_ENDDL. A producer record in a static list is an error (logged, the list ends). */
static __attribute__((cold, noinline)) void list_bad(const Gfx* g, unsigned n) {
    static int k;
    if (k++ < 4) pm_log_e("ndc: static list op %08x at %p after %u words not recorded", (unsigned)g->words.w0, (const void*)g, n);
}
void nd_rec_list(const void* l) {
    const Gfx* g = (const Gfx*)l;
    if (__builtin_expect(nd_lc_frame != -1, 0)) nd_lstcen(l, 5u);
    for (unsigned n = 0; n < 1024u; n++, g++) {
        const u32 u0 = g->words.w0, u1 = g->words.w1;
        u32 w0 = u0, w1 = u1;
        unsigned op = w0 >> 24;
        if (!nd_op_typed(op)) { list_bad(g, n); return; } /* HC4e4: a static list is typed at compile time */
        if (op == NDO_END) op = G_ENDDL; /* the raw op the cases below know */
        else if (IS_NOP(w0)) op = G_SPNOOP;
        else if (op == NDO_RECT && (w1 & NDO_RECT_TEX)) { w0 = ((u32)G_TEXRECT << 24) | (w0 & 0xffffffu); w1 &= ~NDO_RECT_TEX; op = G_TEXRECT; }
        else if (op == NDO_CALL) { w0 = ((u32)G_DL << 24) | ((w0 & 1u) ? (u32)G_DL_NOPUSH << 16 : 0u); op = G_DL; }
        switch (op) {
        case G_ENDDL: return;
        case G_RDPPIPESYNC: case G_RDPTILESYNC: case G_RDPLOADSYNC: case G_RDPFULLSYNC: case G_SPNOOP: break;
        case G_TEXRECT: case G_TEXRECTFLIP: nd_rec_texrect(w0, w1, g[1].words.w1, g[2].words.w1); g += 2; n += 2; break;
        case G_DL:
            if (((w0 >> 16) & 0xffu) == G_DL_NOPUSH) { /* a branch: host addresses only (a segment base is walker state) */
                if (!is_host(w1)) { list_bad(g, n); return; }
                g = (const Gfx*)(uintptr_t)w1 - 1;
                break;
            }
            if (!rec_inline(w1)) nd_rec_state(u0, u1);
            break;
        default: nd_rec_state(u0, u1); break;
        }
    }
    list_bad(g, 1024u);
}
const uint32_t* nd_rec_close(void) {
    rec_open();
    uint32_t* e = rec_take(2);
    e[0] = NDC_HDR(NDC_END, 0, 0);
    e[1] = 0;
    const uint32_t* h = task_head;
    task_head = NULL;
    return h;
}
/* PROF: the commands per op since the last take, and the stream high-water mark in KB (pm_phase.c ndc line). */
void nd_rec_cnt_take(unsigned o[NDC_N], unsigned* sw, unsigned* kb) {
    static unsigned last[NDC_N], last_sw;
    for (unsigned i = 0; i < NDC_N; i++) { unsigned v = ndc_n[i]; o[i] = v - last[i]; last[i] = v; }
    { unsigned v = ndc_sw; *sw = v - last_sw; last_sw = v; }
    *kb = (hw_units * 4u + 1023u) >> 10;
}

static __attribute__((cold, noinline)) void run_bad(uint32_t h, const uint32_t* p) {
    static int n;
    if (n++ < 4) pm_log_e("ndc: bad command %08x at %p", (unsigned)h, (const void*)p);
}
static void stream_run(const uint32_t* p, const uint32_t* stop) {
    for (;;) {
        __builtin_prefetch(p + 16); /* HC9B: the stream 64 B ahead (no hardware prefetcher on the ARM11) */
        if (p == stop) return;
        const uint32_t h = p[0];
        switch (h >> 24) {
        case NDC_END: return;
        case NDC_JUMP: p = (const uint32_t*)(uintptr_t)p[1]; continue;
        case NDC_MODEL: nd_walk_rec_model((const Gfx*)(p + 2)); break;
        case NDC_SPR: nd_walk_spr((const Gfx*)(p + 2)); break;
        case NDC_ENT: nd_walk_ent((const Gfx*)(p + 2), (h >> 16) & 0xffu); break;
        case NDC_TEXRECT: nd_texrect_run(p); break;
        case NDC_STATE: nd_walk_state(p + 2, (h >> 16) & 0xffu); break;
        case NDC_NAPI: ((NdNapiRun)(uintptr_t)p[1])(p + 2); break;
        case NDC_CALL: stream_run((const uint32_t*)(uintptr_t)p[1], (const uint32_t*)(uintptr_t)p[2]); break;
        default: run_bad(h, p); return;
        }
        p += 2u + 2u * ((h >> 16) & 0xffu);
    }
}
void nd_stream_run(const uint32_t* p) { stream_run(p, NULL); }
