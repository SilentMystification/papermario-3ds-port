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
#include "nd_gtab.h"
#include "nd_lkey.h"
#include "nd_rmdef.h"
#include "nd_ds.h"
#include "nd_msnap.h"
#include "nd_fs.h"

extern s32 gCurrentDisplayContextIndex;
void nd_walk_rec_model(const NdW2* q);           /* nd_walk.c: a model record, as the walker's G_PM_REC arm */
void nd_walk_spr(const NdW2* q);                 /* nd_walk.c: a sprite record and its geometry words */
void nd_walk_ent(const NdW2* q, unsigned words); /* nd_walk.c: an entity record (and its G_DL) */
void nd_texrect_run(const uint32_t* p);
int is_host(u32 p);                             /* nd_walk.c: p is in host memory */         /* nd_tex_set.c: a texture rectangle */

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
static unsigned ndc_sw;      /* PROF: state ops recorded (native record run units) */
static uint32_t* st_hdr;     /* D4b-3: the header of the open native record run (rec_take clears it: any other record closes the run) */
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
/* HWFIX: chunks 0 and 1 of both chains at boot (pm_main.c, before the logic thread). On the O3DS the commit is full after a map load, so a chunk taken in game can fail (ch_get stops the game). */
void nd_rec_reserve(void) {
    for (unsigned k = 0; k < 2u; k++)
        for (unsigned i = chain[k].nch; i < 2u; i++) {
            chain[k].ch[i] = (uint32_t*)malloc(CH_WORDS * 4u);
            if (!chain[k].ch[i]) { pm_log_e("ndc: no memory for stream chunk %u of context %u at boot", i, k); return; }
            chain[k].nch = i + 1u;
        }
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
    case ND_FS_OMH: {
        const uint32_t m = nd_op_om_mask(a), sft = a & 0xffu, len = (a >> 8) & 0xffu;
        if (sh.sc1 && !(m & ~sh.omh_k) && !((b ^ sh.omh_v) & m)) return 1;
        sh.omh_v = (sh.omh_v & ~m) | (b & m); sh.omh_k |= m;
        if (sft <= G_MDSFT_CYCLETYPE && sft + len > G_MDSFT_CYCLETYPE) sh.sc1 = 1; /* the setter syncs the combine */
        else sh.tex_ok = sh.comb_ok = 0; /* the combine input hash may differ from the last sync's */
        return 0;
    }
    case ND_FS_OML: {
        const uint32_t m = nd_op_om_mask(a);
        if (!(m & ~sh.oml_k) && !((b ^ sh.oml_v) & m)) return 1;
        sh.oml_v = (sh.oml_v & ~m) | (b & m); sh.oml_k |= m;
        return 0;
    }
    case ND_FS_GEOM: return sh_unit(sh.geom, &sh.geom_ok, t);
    case ND_FS_PRIM: { const int d = sh_unit(sh.prim, &sh.prim_ok, t); sh.sc1 = 1; return d; }
    case ND_FS_TEXTURE: { const int d = sh_unit(sh.tex, &sh.tex_ok, t); sh.sc1 = 1; return d; }
    case ND_FS_COMB: { const int d = sh_unit(sh.comb, &sh.comb_ok, t); sh.sc1 = 1; return d; }
    case ND_FS_ENV:
        sh.sc1 = 1;
        if (sh.env_ok && sh.env == b) return 1;
        sh.env = b; sh.env_ok = 1;
        return 0;
    case ND_FS_FOG:
        if (sh.fog_ok && sh.fog == b) return 1;
        sh.fog = b; sh.fog_ok = 1;
        return 0;
    case ND_FS_BLEND:
        if (sh.blend_ok && sh.blend == b) return 1;
        sh.blend = b; sh.blend_ok = 1;
        return 0;
    case ND_FS_MW:
        if ((a >> 16) == ND_FS_MW_LIGHTCOL) {
            const unsigned k = a & 0xffffu;
            if ((sh.lc_ok >> k & 1u) && !((sh.lc[k] ^ b) & 0xffffff00u)) return 1;
            sh.lc[k] = b; sh.lc_ok |= (uint16_t)(1u << k);
        }
        return 0;
    case ND_FS_RARE: sh.sc1 = 1; return 0;
    case ND_FS_TIMG: case ND_FS_TILE: case ND_FS_TILESIZE: case ND_FS_LOADBLOCK: case ND_FS_LOADTILE: case ND_FS_LOADTLUT:
        sh_sc(); return 0; /* HC4c: a tile, load or image op may change the combine inputs (td[1].valid, dual_on) */
    case ND_FS_VTX: case ND_FS_TRI1: case ND_FS_TRI2: case ND_FS_MTX: case ND_FS_POPMTX:
        sh_sc(); return 0; /* HC4d: a draw or matrix op keeps the walker fields but may change the combine inputs (box_fold_sync, dual_bind) */
    case ND_FS_MOVEMEM: sh_all(); return 0; /* a light or viewport: the walker state after it is not followed */
    case ND_FS_CALL: case ND_FS_RECT: case ND_FS_MISC: sh_all(); return 0; /* HC4d2: a list call, rectangle, image or marker (as the word ops before) */
    default: return 0;
    }
}
/* A typed no-op unit (nd_op.h): a sync, G_SPNOOP, G_NOOP or a RDPHALF word. */
#define IS_NOP(w0) ((w0) == ND_FS_W0(ND_FS_MISC, ND_FS_MISC_NOP << 16))

/* HC4b (W3): the render mode table. Entry i holds the typed ops of nd_rm_dev[i] (nd_rmtab.c, 61 device entries) without the syncs,
 * then ND_FS_END. Built by the recorder at the first record of index i; the frame push publishes it to core 0; never written again. */
uint32_t nd_rm_tab[ND_RM_N][2u * ND_RM_OPS];
uint8_t nd_rm_n[ND_RM_N];
static uint8_t rm_built[ND_RM_N];
__attribute__((cold, noinline)) void nd_rm_build(unsigned i) {
    if (i >= ND_RM_N) return;
    unsigned n = 0, nl = 0;
    const NdNDev* e = i < ND_RM_LISTS ? nd_rm_dev[i] : NULL; /* MODELRM: the render mode lists are device entries (nd_rmtab.c) */
    const uint32_t* lu = e ? nd_lkey_units(e, e, 4096u, &nl) : NULL;
    for (unsigned k = 0; k < nl && n + 1u < ND_RM_OPS; k++) {
        const uint32_t w0 = lu[2u * k], w1 = lu[2u * k + 1u];
        if ((w0 >> 24) == ND_FS_END) break;
        if (IS_NOP(w0)) continue; /* a typed sync or no-op */
        nd_rm_tab[i][2u * n] = w0; nd_rm_tab[i][2u * n + 1u] = w1; /* HC4e4: the device units are typed (gs forms) */
        n++;
    }
    nd_rm_tab[i][2u * n] = ND_FS_W0(ND_FS_END, 0);
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
    nd_ds_lg_reset();
}
/* Room for n words (header and payload): else the chunk ends with NDC_JUMP to the next one. */
static int st_take; /* HC4b: rec_take of a state op (the shadow stays) */
static uint32_t* rec_take(unsigned n) {
    st_hdr = NULL; /* any record closes the open state run (nd_rec_nr reopens its own) */
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
void nd_snap_task(void); /* NAT2J: nd_snap.c */
static void task_open(void) {
    rec_ctx_check();
    if (!task_head) { task_head = wp; sh_all(); nd_snap_task(); } /* NAT2J */
}
static void rec_open(void) { task_open(); } /* DEL3C1: no top-level packet on the device */
static __attribute__((cold, noinline)) void model_bad(unsigned words) {
    pm_log_e("ndc: model record of %u words (most %u)", words, G_PM_REC_WORDS + ND_REC_AUX_MAX);
    pm_fatal_stop("ndc: model record too long");
}
/* HC4c (W4): the payload holds typed ops: the panner tile sizes (R9, R10) and the R13 words. R13 w0 is the count of typed units.
 * HC4e4: the producer macros (nd_gbi.h) type every unit, so the payload is copied as it is.
 * R13NOP: the R13 syncs (typed ND_FS_MISC_NOP units) are dropped here; the count q[26] is the kept units. */
/* NST2b: one typed unit into the core 1 state (nd_ds_apply), its ND_FS_CALL list (depth 1) and the NST3 ep rule. The model record
 * (nd_rec_model) and the stream units (nd_rec_nr) both call it. */
void nd_ds_unit(NdDs* s, uint32_t w0, uint32_t w1) {
    nd_ds_apply(s, w0, w1); /* NST1: the core 1 draw state follows the units (core 0 does not read it) */
}
void nd_msnap_rec(uint32_t* p, unsigned units); /* NREC1A: nd_msnap.c, weak below */
void nd_rec_model(const void* rec, unsigned words) {
    if (words > G_PM_REC_WORDS + ND_REC_AUX_MAX) model_bad(words);
    { const unsigned rmi = (((const uint32_t*)rec)[0] >> 16) & 0xffu; if (rmi < ND_RM_N && !rm_built[rmi]) nd_rm_build(rmi); } /* HC4b: W3 */
    const uint32_t* r = (const uint32_t*)rec; unsigned keep = 0;
    for (unsigned k = G_PM_REC_WORDS; k < words; k++) if (!IS_NOP(r[2 * k])) keep++; /* R13NOP: a typed sync is not an R13 op */
    rec_open(); uint32_t* p = rec_take(2u + 2u * (G_PM_REC_WORDS + keep) + 2u + 2u * ND_MSNAP_U + 4u); /* NREC1B: room for the jump and the model snap */ ndc_n[NDC_MODEL]++;
    memcpy(p + 2, rec, G_PM_REC_WORDS * 8u); uint32_t* q = p + 2; uint32_t* o = q + 2 * G_PM_REC_WORDS;
    for (unsigned k = G_PM_REC_WORDS; k < words; k++) if (!IS_NOP(r[2 * k])) { o[0] = r[2 * k]; o[1] = r[2 * k + 1]; o += 2; }
    q[26] = keep; p[0] = NDC_HDR(NDC_MODEL, G_PM_REC_WORDS + keep, 0);
    { uint32_t* j = p + 2u + 2u * (G_PM_REC_WORDS + keep); j[0] = NDC_HDR(NDC_JUMP, 0, 0); j[1] = (uint32_t)(uintptr_t)(j + 2u + 2u * ND_MSNAP_U + 4u); } /* NREC1B; NAT5B: the 2 alt ranges follow the snap */
    nd_msnap_pre(&nd_ds); /* NAT2C: the state before the record */
    { const unsigned i = (q[0] >> 16) & 0xffu; nd_ds_model(&nd_ds, q, i < ND_RM_N ? nd_rm_tab[i] : NULL, i < ND_RM_N ? nd_rm_n[i] : 0u, nd_ds_unit, p + 4u + 2u * (G_PM_REC_WORDS + keep) + 2u * ND_MSNAP_U); } /* NST2b: the model record feeds the core 1 state */ nd_msnap_rec(p, G_PM_REC_WORDS + keep); /* NREC1A: the model snap before the post range */ if (q[23]) nd_ds_range(&nd_ds, (const uint32_t*)(uintptr_t)q[23], NULL); /* NREC1A: custom gfx post, moved from nd_ds_model */
    p[1] = nd_mkey_a(q); /* HC6 step 2: key A of the model cache (the record part), hashed here on core 1 */
}
void nd_snap_mesh(void); /* NDR4: strong in nd_snap.c */
void nd_rec_spr(const void* rec, const void* geo) {
    nd_snap_mesh(); /* NDR4: the snap of the sprite draw */
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
    NdW2 r[G_PM_ENT_WORDS + 1];
    gPmEntDrawRecord(r, list, segid, varray);
    r[2].words.w0 = form;
    r[G_PM_ENT_WORDS].words.w0 = ND_OP_CALL_W0; /* HC4d2: the list call as a typed op (W6) */
    r[G_PM_ENT_WORDS].words.w1 = (uint32_t)(uintptr_t)list;
    rec_open();
    uint32_t* p = rec_take(2u + 2u * (G_PM_ENT_WORDS + 2u));
    ndc_n[NDC_ENT]++;
    p[0] = NDC_HDR(NDC_ENT, G_PM_ENT_WORDS + 2u, 0);
    p[1] = 0;
    memcpy(p + 2, r, sizeof r);
    uint32_t* const x = p + 2u + 2u * (G_PM_ENT_WORDS + 1u); uint32_t ab[2]; x[0] = x[1] = 0u; uint32_t* const br = nd_alt_open(); nd_alt_raw_on = 2; nd_alt_et_begin(); /* NAT5A */
    nd_lsc_begin(); nd_ds_list(&nd_ds, list, 0);
    nd_alt_raw_on = 0; nd_alt_close(br, ab); x[1] = (uint32_t)(uintptr_t)nd_alt_et_end(ab[1]); /* NAT5A */
    nd_lsc_end_at(p + 1); /* NST2C: the entity's list state on core 1 (core 0 runs it at nd_eg_run or walk_call) NREC2: the list snap table in header word 1 */
}
void nd_rec_ent(const void* rec) {
    rec_open();
    uint32_t* p = rec_take(2u + 2u * G_PM_ENT_WORDS);
    ndc_n[NDC_ENT]++;
    p[0] = NDC_HDR(NDC_ENT, G_PM_ENT_WORDS, 0);
    p[1] = 0;
    memcpy(p + 2, rec, G_PM_ENT_WORDS * 8u);
}
void nd_snap_mesh(void); /* NDR0E2: prototype (fix: implicit declaration warning); strong in nd_snap.c */
void nd_snap_rect(void); /* NDR1C: strong in nd_snap.c */
void nd_rec_texrect(uint32_t w0, uint32_t w1, uint32_t hs, uint32_t hd) {
    nd_snap_rect(); /* NDR1C: the snap of every texture rectangle (memo: one per new input) */
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
static uint32_t* napi_last; /* NCONV1D1: the data of the last nd_rec_napi record (nd_rec_last_pay) */
void nd_rec_napi(NdNapiRun run, const void* rec, unsigned units) {
    if (units == 0u || units > 255u) napi_bad(units);
    rec_open();
    uint32_t* p = rec_take(2u + 2u * units);
    ndc_n[NDC_NAPI]++;
    p[0] = NDC_HDR(NDC_NAPI, units, 0);
    p[1] = (uint32_t)(uintptr_t)run;
    memcpy(p + 2, rec, units * 8u);
    napi_last = p + 2u;
}
static unsigned ndc_snapc; /* NDR3: the records of nd_rec_napi_s (not a command of the snap memo) */
void nd_rec_napi_s(NdNapiRun run, const void* rec, unsigned units) { /* NDR3: as nd_rec_napi; the runner takes the armed snap */
    if (units == 0u || units > 255u) napi_bad(units);
    rec_open();
    uint32_t* p = rec_take(2u + 2u * units);
    ndc_n[NDC_NAPI]++;
    ndc_snapc++;
    p[0] = NDC_HDR(NDC_NAPI, units, 1);
    p[1] = (uint32_t)(uintptr_t)run;
    memcpy(p + 2, rec, units * 8u);
    napi_last = p + 2u; /* NAT5R */
}
/* NDR5: a transform record is taken as a state op: the shadow stays, no dschk record, nd_ds.sc_lost stays (rec_take :188-201) */
void nd_rec_xf(NdNapiRun run, const uint32_t* rec) {
    st_take = 1;
    nd_rec_napi(run, rec, 1u);
    st_take = 0;
}
/* NDR0E2: a snap record taken as a state op (shadow, dschk and sc_lost stay) */
void nd_rec_st(NdNapiRun run, const void* rec, unsigned units) {
    st_take = 1;
    nd_rec_napi(run, rec, units);
    st_take = 0;
}
/* NATIVEAPI S4: a run of particle draws (nd_api.h NdParts). S7CONV: a list is a flat particle list when its v2 entry has
 * NDN_FL_PARTS (nd_gtab.h: the build-time form of the old parts_ok rule, proven equal by nd_gtab_test.c). Memo: the last 8 lists found. */
/* NST2C: the units of a native record feed core 1 (nd_ds_unit; a sync is skipped) */
void nd_ds_feed(const uint32_t* u, unsigned n) {
    for (unsigned k = 0; k < n; k++) if (u[2u * k] != ND_NOP_A) nd_ds_unit(&nd_ds, u[2u * k], u[2u * k + 1u]);
}
void nd_rec_nr(uint32_t w0, uint32_t w1); /* the stream form of one typed unit (below) */
static const NdNDev* parts_tab(const void* l) {
    static const void* memo_l[8];
    static const NdNDev* memo_t[8];
    static unsigned mi;
    for (unsigned k = 0; k < 8u; k++) if (memo_l[k] == l) return memo_t[k];
    const NdNDev* t = nd_ntab_find(l);
    if (!t || !(t->fl & NDN_FL_PARTS)) return NULL;
    memo_l[mi] = l; memo_t[mi] = t; mi = (mi + 1u) & 7u;
    return t;
}
void nd_parts_begin(NdParts* b, unsigned pre, unsigned post) {
    b->n = 0; b->pre = pre; b->m = pre + 1u + post; b->ok = 0; b->list = NULL;
}
/* units: m units (8 bytes each) of one instance; the ND_FS_CALL unit of its list at index pre (the push form). A list that is not a particle list
 * (or a call that is not the push form) takes the stream form of its units: the open run is recorded first, so the order is kept. */
void nd_parts_add(NdParts* b, const uint32_t* units) {
    const unsigned m = b->m, pre = b->pre;
    const uint32_t a = units[2u * pre + 1u];
    const NdNDev* t = units[2u * pre] == ND_OP_CALL_W0 && is_host(a) ? parts_tab((const void*)(uintptr_t)a) : NULL;
    if (!t) {
        nd_parts_end(b);
        for (unsigned k = 0; k < m; k++) nd_rec_nr(units[2u * k], units[2u * k + 1u]);
        return;
    }
    memcpy(b->u + 2u + 2u * m * b->n, units, 2u * m * 4u);
    b->u[2u + 2u * m * b->n + 2u * pre + 1u] = (uint32_t)(uintptr_t)t; /* DEL5B2: the call unit carries the v2 entry (nd_parts_run, nd_part_draw) */
    b->n++;
    /* the record: 1 + n * m units (at most 255); the instances sit in u (252 units) */
    if (1u + (b->n + 1u) * m > 255u || (b->n + 1u) * m > 252u) nd_parts_end(b);
}
void nd_parts_end(NdParts* b) {
    if (!b->n) return;
    const unsigned n = b->n, pre = b->pre, m = b->m; static const uint32_t z[2] = { 0u, 0u };
    nd_rec_napi(nd_pbeg_run, z, 1u); nd_ds.sc_lost = 1; nd_snap_vprim(1u); /* NAT5E: the run start (a command, as the old record): vprim 1, sc_ok 0 */
    for (unsigned k = 0; k < n; k++) { const uint32_t* p = b->u + 2u + 2u * m * k;
        for (unsigned j = 0; j < m; j++) { if (j == pre) { nd_snap_mesh(); nd_rec_nat(nd_pdraw_run, p[2u * j + 1u], 0u); } /* NAT5E: the instance snap and the list draw */
            else if (p[2u * j] != ND_NOP_A) nd_rec_nr(p[2u * j], p[2u * j + 1u]); } }
    nd_rec_nat(nd_pend_run, 0u, 0u); nd_snap_vprim(0u); nd_ds.sc_lost = 0; nd_ds.sc_dirty = 0; /* NAT5E: the run end (nd_comb_sync on core 0) */
    b->n = 0;
}
void nd_ds_feed_dir(const uint32_t* u, unsigned n) { nd_ds_feed(u, n); nd_ds.sc_lost = 1; } /* NST2J: a direct draw ends with sc_ok 0 */
/* D4b-3: a state op (HC4a: typed by the recorder) extends the open native record run when nothing was recorded after it (no span, no other command) and the chunk has room;
 * else it opens a new run. HC4b: a typed op the shadow drops is not written. */
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
    ndc_n[NDC_NRUN]++;
    p[0] = NDC_HDR(NDC_NAPI, 1, 0);
    p[1] = (uint32_t)(uintptr_t)nd_nr_run;
    p[2] = w0;
    p[3] = w1;
    st_hdr = p;
    st_end = wp;
}
/* HC4d2: a typed op into the open state run (the shadow decides a drop). */
static void rec_op_t(const uint32_t t[2]) {
    rec_open();
    ndc_sw++;
    if (sh_drop(t)) return; /* HC4b: a typed op the shadow drops is not written */
    st_put(t[0], t[1]);
}
static __attribute__((cold, noinline)) void word_bad(uint32_t w0) {
    static int k;
    if (k++ < 4) pm_log_e("ndc: untyped word %08x not recorded", (unsigned)w0);
}
/* HC4a: typed on the recorder (nd_op.h). HC4e4: an untyped word is not recorded. */
__attribute__((weak)) void nd_sci_run(const uint32_t* r) { (void)r; } __attribute__((weak)) void nd_geo_run(const uint32_t* r) { (void)r; } __attribute__((weak)) void nd_mw_run(const uint32_t* r) { (void)r; } /* NAT4I: strong in nd_set.c */
__attribute__((weak)) void nd_tex_run(const uint32_t* r) { (void)r; } __attribute__((weak)) void nd_omh_run(const uint32_t* r) { (void)r; } /* NAT4J: strong in nd_set.c */
static void rec_nat(NdNapiRun run, uint32_t a, uint32_t b) { /* NAT4I: a native state record in the place of a stream unit; header flag 1 keeps the snap arm */
    st_take = 1; uint32_t* p = rec_take(4); st_take = 0; ndc_n[NDC_NRUN]++;
    p[0] = NDC_HDR(NDC_NAPI, 1, 1); p[1] = (uint32_t)(uintptr_t)run; p[2] = a; p[3] = b; }
void nd_rec_nat(NdNapiRun run, uint32_t a, uint32_t b) { rec_open(); rec_nat(run, a, b); } /* NAT5E */
__attribute__((weak)) void nd_mls_run(const uint32_t* r) { (void)r; } __attribute__((weak)) void nd_mle_run(const uint32_t* r) { (void)r; } /* NAT5B: strong in nd_walk.c */
void nd_alt_lst(unsigned end) { nd_rec_nat(end ? nd_mle_run : nd_mls_run, 0u, 0u); } /* NAT5B: the list start and end markers of nd_nr_list keep 0 */
static const NdDs* alt_s = &nd_ds; /* NAT5T: the state that the txr puts read (a scratch copy on the static paths) */
static uint32_t txr_take(uint32_t a, uint32_t b) { /* NAT5T, core 1: the window of a TILESIZE of tile 0 or 1, as nd_tsz_set and tex_tile1 compute it */
    const NdDs* s = alt_s; const NdTst* t = &s->w.tst1; const unsigned tile = (b >> 24) & 7u; NdWin wn;
    int uls = (int)((a >> 12) & 0xfff), ult = (int)(a & 0xfff), lrs = (int)((b >> 12) & 0xfff), lrt = (int)(b & 0xfff);
    if (((lrs - uls) >> 2) + 1 <= 0 || ((lrt - ult) >> 2) + 1 <= 0) tex_win_s25(&uls, &ult, &lrs, &lrt, t->tile_wrap[tile][0], t->tile_wrap[tile][2], t->tile_wrap[tile][1], t->tile_wrap[tile][3]);
    NdTxR* r = (NdTxR*)nd_rec_data_st(sizeof *r); memset(r, 0, sizeof *r);
    if (tile == 0u) nd_texw0(t, &s->tm, s->w.timg, uls, ult, lrs, lrt, &wn); else nd_texw1(t, &s->tm, s->w.timg, uls, ult, lrs, lrt, &wn);
    r->tile = tile; r->uls = uls; r->ult = ult; r->lrs = lrs; r->lrt = lrt; r->src = wn.src; r->w = wn.w; r->h = wn.h; r->stride = wn.stride; r->x0 = wn.x0;
    r->pal = s->tm.tlut_pal[t->tile_pal[tile] & 15] ? s->tm.tlut_pal[t->tile_pal[tile] & 15] : s->w.tlut; r->pal_n = s->w.tlut_n; r->timg = s->w.timg;
    if (tile == 1u && t->tile_fmt[1] == 2 && t->tile_siz[1] == 0 && s->tm.ld_valid && s->w.timg) { r->m1v = 1; r->m1src = (const uint8_t*)s->w.timg + (size_t)s->tm.ld_y * (size_t)t->timg_stride + (size_t)s->tm.ld_x;
        r->m1stride = t->timg_stride * 2; r->m1w = ((lrs - uls) >> 2) + 1; r->m1h = ((lrt - ult) >> 2) + 1; r->m1x0 = uls >> 2; r->m1y0 = ult >> 2; } /* tex_tile1 :457-466 */
    return (uint32_t)(uintptr_t)r; }
NdDs* nd_alt_scr_on(void) { static NdDs scr; scr = nd_ds; alt_s = &scr; return &scr; } /* NAT5T: a scratch copy of core 1 state for the static paths (nd_rec_ndev, the stream CALL, nd_rec_alt_raw): core 1 state stays as it was */
void nd_alt_scr_off(void) { alt_s = &nd_ds; } /* NAT5T */
void nd_rec_nr(uint32_t w0, uint32_t w1) {
    const uint32_t t[2] = { w0, w1 };
    if (__builtin_expect(!nd_op_typed(w0 >> 24), 0)) { word_bad(w0); return; }
    if ((w0 >> 24) == ND_FS_TRI1 || (w0 >> 24) == ND_FS_TRI2) nd_snap_mesh(); /* NAT2G: every stream triangle has a snap, or the memo says the last one holds */
    if ((w0 >> 24) == ND_FS_RECT && !(w1 & ND_FS_RECT_TEX)) nd_snap_rect(); /* NAT2H: a fill rectangle takes a snap, as a texture rectangle (nd_rec_texrect :264) */
    const unsigned id = w0 >> 24; const int lost = nd_ds.sc_lost, dirty = nd_ds.sc_dirty; /* NAT4A */
    const unsigned ch = nd_ds_apply(&nd_ds, w0, w1); /* NAT4A: nd_ds_unit's work, with the change bit */
    rec_open(); ndc_sw++; if (sh_drop(t)) return; /* NAT4A: rec_op_t, inline */
    if (id == ND_FS_SCISSOR || id == ND_FS_GEOM || id == ND_FS_MW || id == ND_FS_TEXTURE || id == ND_FS_OMH) { const uint32_t mk = nd_ds_ep_rule(w0, ch, lost, dirty) ? 0x80000000u : 0u; /* NAT4I: no mark, no unit */
        if (id == ND_FS_SCISSOR) rec_nat(nd_sci_run, ((uint32_t)nd_ds.sci_x0 & 0xffffu) | (uint32_t)nd_ds.sci_y0 << 16, ((uint32_t)nd_ds.sci_x1 & 0xffffu) | (uint32_t)nd_ds.sci_y1 << 16);
        else if (id == ND_FS_GEOM) rec_nat(nd_geo_run, nd_ds.w.geom, mk);
        else if (id == ND_FS_TEXTURE) rec_nat(nd_tex_run, (w0 & 0xffffffu) | mk, w1); /* NAT4J */
        else if (id == ND_FS_OMH) rec_nat(nd_omh_run, (w0 & 0xffffu) | mk, w1); /* NAT4J */
        else { const unsigned k = (w0 >> 16) & 0xffu, g = w0 & 0xffffu; rec_nat(nd_mw_run, (w0 & 0xffffffu) | mk, (k == ND_FS_MW_SEG && g < 16u) ? nd_ds.w.segbase[g] : k == ND_FS_MW_NUMLIGHT ? nd_ds.lt_n : w1); }
        return; }
    if ((id >= ND_FS_OMH && id <= ND_FS_COMB) || (id >= ND_FS_TIMG && id <= ND_FS_LOADTLUT)) { /* NAT4G */ const unsigned rc = nd_ds_ep_rule(w0, ch, lost, dirty);
        if (rc) { st_put(ND_FS_W0(ND_FS_MISC, ND_FS_MISC_EP << 16), rc | (lost ? 0x100u : 0u) | (dirty ? 0x200u : 0u) | (ch & 1u) << 10); /* NAT4B: the cut and its reason */
            if (id == ND_FS_OML || (id >= ND_FS_PRIM && id <= ND_FS_PRIMZ) || id == ND_FS_RARE || id == ND_FS_COMB) return; } } /* NAT4K: a marked combine class unit is not recorded */
    if (id == ND_FS_CALL) { /* NAT5T: the called static list as a raw alt range, its tile sizes carry the core 1 window; core 1 state keeps no nested state */
        NdDs* const sc = nd_alt_scr_on(); const unsigned char om = nd_alt_raw_on; nd_alt_raw_on = 1; uint32_t* const blk = nd_alt_call(w0, w1);
        nd_ds_list(sc, (const void*)(uintptr_t)w1, 1); nd_alt_call_end(blk); nd_alt_raw_on = om; nd_alt_scr_off(); return; }
    if (id == ND_FS_TILESIZE && ((w1 >> 24) & 7u) < 2u) { st_put(ND_FS_W0(ND_FS_TXR, 0), txr_take(w0, w1)); return; } /* NAT5T */
    st_put(t[0], t[1]); /* NAT4A */
}
/* NAT5R, core 1: a record (run, rec, units) with one spare unit, then its fallback units au as an alt range that the main stream jumps over.
 * The spare unit holds the range bounds (nd_alt_run). The units feed nd_ds (nd_rec_nr); the snap memo restarts at both ends. */
void nd_rec_alt(NdNapiRun run, const void* rec, unsigned units, int s, const uint32_t* au, unsigned an, int lost) {
    if (units + 1u > 255u) napi_bad(units + 1u); /* NAT5R */
    rec_open(); uint32_t* p = rec_take(4u + 2u * units); ndc_n[NDC_NAPI]++; if (s) ndc_snapc++; /* NAT5R */
    p[0] = NDC_HDR(NDC_NAPI, units + 1u, s ? 1u : 0u); p[1] = (uint32_t)(uintptr_t)run; memcpy(p + 2, rec, units * 8u); napi_last = p + 2u; /* NAT5R */
    uint32_t* const ab = p + 2u + 2u * units; ab[0] = ab[1] = 0u;
    uint32_t* const br = rec_take(2); br[0] = NDC_HDR(NDC_JUMP, 0, 0); br[1] = 0u; ab[0] = (uint32_t)(uintptr_t)wp; nd_snap_task(); /* NAT5R */
    for (unsigned k = 0; k < an; k++) { const uint32_t w0 = au[2u * k], w1 = au[2u * k + 1u];
        if (w0 == ND_NOP_A) continue;
        if ((w0 >> 24) == ND_FS_RECT && (w1 & ND_FS_RECT_TEX) && k + 2u < an) { nd_rec_texrect(w0, w1 & ~ND_FS_RECT_TEX, au[2u * k + 3u], au[2u * k + 5u]); k += 2u; continue; } /* as nd_rec_list */
        nd_rec_nr(w0, w1); }
    st_hdr = NULL; sh_all(); nd_snap_task(); br[1] = (uint32_t)(uintptr_t)wp; ab[1] = (uint32_t)(uintptr_t)wp; /* NAT5R */
    if (lost) nd_ds.sc_lost = 1; /* the old nd_ds_feed_dir */
}
/* NAT5T, core 1: as nd_rec_alt, the fallback units au copied raw (tile sizes of tile 0 and 1 carry their window); the units apply to a scratch copy of core 1 state, which stays as it was. */
void nd_alt_put(uint32_t w0, uint32_t w1);
void nd_rec_alt_raw(NdNapiRun run, const void* rec, unsigned units, int s, const uint32_t* au, unsigned an) {
    if (units + 1u > 255u) napi_bad(units + 1u);
    rec_open(); uint32_t* p = rec_take(4u + 2u * units); ndc_n[NDC_NAPI]++; if (s) ndc_snapc++;
    p[0] = NDC_HDR(NDC_NAPI, units + 1u, s ? 1u : 0u); p[1] = (uint32_t)(uintptr_t)run; memcpy(p + 2, rec, units * 8u); napi_last = p + 2u;
    uint32_t* const ab = p + 2u + 2u * units; ab[0] = ab[1] = 0u;
    uint32_t* const br = rec_take(2); br[0] = NDC_HDR(NDC_JUMP, 0, 0); br[1] = 0u; ab[0] = (uint32_t)(uintptr_t)wp; nd_snap_task();
    NdDs* const sc = nd_alt_scr_on();
    for (unsigned k = 0; k < an; k++) { const uint32_t w0 = au[2u * k], w1 = au[2u * k + 1u]; nd_alt_put(w0, w1); nd_ds_apply(sc, w0, w1); }
    nd_alt_scr_off();
    st_hdr = NULL; sh_all(); nd_snap_task(); br[1] = (uint32_t)(uintptr_t)wp; ab[1] = (uint32_t)(uintptr_t)wp;
}
/* NAT5F, core 1: a raw alt range. The units are copied as they are (no mark, no shadow drop, no snap); core 0 runs them with its list executor.
 * A texture rectangle and its 2 RDPHALF units stay in one NRUN run (nr_ctl reads at[3], at[5]). */
void nd_alt_put(uint32_t w0, uint32_t w1) {
    if ((w0 >> 24) == ND_FS_TILESIZE && ((w1 >> 24) & 7u) < 2u) { w1 = txr_take(w0, w1); w0 = ND_FS_W0(ND_FS_TXR, 0); } /* NAT5T */
    if ((w0 >> 24) == ND_FS_RECT && (w1 & ND_FS_RECT_TEX) && st_hdr && (((st_hdr[0] >> 16) & 0xffu) > 252u || wp + 6u > wend)) st_hdr = NULL;
    st_put(w0, w1); }
uint32_t* nd_alt_open(void) { rec_open(); uint32_t* const br = rec_take(2); br[0] = NDC_HDR(NDC_JUMP, 0, 0); br[1] = 0u; st_hdr = NULL; sh_all(); return br; }
void nd_alt_close(uint32_t* br, uint32_t* ab) { rec_open(); st_hdr = NULL; sh_all(); br[1] = (uint32_t)(uintptr_t)wp; ab[0] = (uint32_t)(uintptr_t)(br + 2); ab[1] = (uint32_t)(uintptr_t)wp; }
/* NAT5N, core 1: a call unit inside a raw alt range. A block {list word, a, b} in stream data, the unit CALL | 2 with w1 = the block,
 * then a jump over the nested range that ds_pairs fills with the called list; nd_alt_call_end closes it (blk[3] keeps the jump). */
uint32_t* nd_alt_call(uint32_t w0, uint32_t w1) {
    uint32_t* const blk = (uint32_t*)nd_rec_data_st(24u); blk[0] = w1; blk[1] = blk[2] = 0u; blk[4] = 0u; /* NAT5A */
    nd_alt_put(w0 | 2u, (uint32_t)(uintptr_t)blk); blk[3] = (uint32_t)(uintptr_t)nd_alt_open(); return blk; }
void nd_alt_call_end(uint32_t* blk) { nd_alt_close((uint32_t*)(uintptr_t)blk[3], blk + 1); blk[4] = (uint32_t)(uintptr_t)nd_alt_et_end(blk[2]); } /* NAT5A */
/* NAT5A, core 1: the cut table of a flat copy. A unit opens a new run when its class (1 VTX, 2 TRI1 or TRI2, 3 ND_NOP_A, 0 other) differs from the last one, and at every VTX. */
static uint32_t et_ua[ND_DLC_MAXOPS]; static unsigned et_n, et_c; static unsigned char et_on;
void nd_alt_et_begin(void) { if (et_on) return; et_on = 1; et_n = 0; et_c = 4u; }
void nd_alt_eput(uint32_t w0, uint32_t w1) {
    if (!et_on) { nd_alt_put(w0, w1); return; }
    const unsigned id = w0 >> 24, c = w0 == ND_NOP_A ? 3u : id == ND_FS_VTX ? 1u : (id == ND_FS_TRI1 || id == ND_FS_TRI2) ? 2u : 0u, cut = c != et_c || c == 1u;
    if (cut) st_hdr = NULL;
    nd_alt_put(w0, w1);
    if (et_n < ND_DLC_MAXOPS) et_ua[et_n] = cut ? (uint32_t)(uintptr_t)st_hdr : 0u;
    et_n++; et_c = c; }
static NdDs fl_sv[12]; static uint32_t* et_flat; /* NAT5T */
NdDs* nd_alt_flat_save(const NdDs* s, unsigned d) { fl_sv[d] = *s; return &fl_sv[d]; } /* NAT5T */
void nd_alt_flat(NdDs* sv, const NdNDev* nt) { /* NAT5T: the flat copy (mode 2) of a list with calls, from the saved state; its cut table waits in et_flat for nd_alt_et_end */
    const NdDs* const sa = alt_s; const unsigned char sr = nd_alt_raw_on; const unsigned char sl = nd_lsc_on; uint32_t ab[2];
    nd_lsc_on = 0; uint32_t* const br = nd_alt_open(); alt_s = sv; nd_alt_raw_on = 2; nd_alt_et_begin();
    nd_ds_pairs0(sv, nt->u, nt->nu);
    nd_alt_close(br, ab); nd_alt_raw_on = sr; et_flat = nd_alt_et_end(ab[1]);
    alt_s = sa; nd_lsc_on = sl; }
uint32_t* nd_alt_et_end(uint32_t end) { /* stream data: t[0] n, t[1 + i] the run header of cut unit i (else 0), t[1 + n] end. NULL: no copy */
    if (!et_on) { uint32_t* const fl = et_flat; et_flat = NULL; return fl; } /* NAT5T */
    et_on = 0;
    if (et_n >= ND_DLC_MAXOPS) return NULL;
    uint32_t* const t = (uint32_t*)nd_rec_data_st(4u * (et_n + 2u)); t[0] = et_n; memcpy(t + 1, et_ua, 4u * et_n); t[1u + et_n] = end; return t; }
/* D4b-3, D5: the nd_gbi.h redirect. The packet gMainGfxPos++ is taken back and the op recorded; the RDP syncs are dropped (the walker skips
 * them). A texture rectangle (its op word and the two RDPHALF words, written by one macro) is held until its last word and recorded as
 * NDC_TEXRECT. Any other packet (a local list) and the branch form of G_DL keep the word; G_ENDDL at the top level is dropped (NDC_END ends a
 * task and a custom gfx range). */
/* DIRECT D11a: a push G_DL whose target is a static state list (in the program image, not a map shape list; to its G_ENDDL at most 64 ops,
 * each a state op of the executor: no list, vertex, triangle, rectangle, image or record op) is recorded in line: G_PM_LST and the list's
 * ops without the RDP syncs. The replay ran such a list on the op executor (walk_call: ent_gdl refuses a list without triangles, nd_sg_try a
 * list outside the map shapes; nd_ops_run with the sync state kept); G_PM_LST carries its list start (gv_epoch, ent_skipgen). The list is
 * read here on the recorder side, the replay reads no list. Any other target keeps its G_DL. */
extern char __end__[];
extern u8 gMapShapeData[];
/* The class of a typed list word for rec_inline: 0 not a state op, 1 a state op, 2 a sync (dropped), 3 the list end. */
static unsigned st_kind(uint32_t w0) {
    const unsigned op = w0 >> 24;
    if (!nd_op_typed(op)) return 0u; /* HC4e4: no raw word in a typed list */
    if (op == ND_FS_END) return 3u;
    if (op == ND_FS_MISC) return (w0 & 0xffffffu) == (ND_FS_MISC_NOP << 16) ? 2u : 0u;
    return (op <= ND_FS_COMB || nd_op_tex(op) || (op >= ND_FS_MTX && op <= ND_FS_MOVEMEM)) ? 1u : 0u;
}
int nd_rec_inline(uint32_t a) {
    if (a < 0x00100000u || a >= (uint32_t)(uintptr_t)__end__ || (a & 7u) || a - (uint32_t)(uintptr_t)gMapShapeData < 0x40000u) return 0;
    const NdNDev* nt = nd_ntab_find((const void*)(uintptr_t)a);
    const uint32_t* w = nt ? nt->u : (const uint32_t*)(uintptr_t)a; /* GKEY2: the words of a list with a device entry come from the entry, its end at nu */
    unsigned n = 0;
    for (;; n++) {
        if (n == 64u) return 0;
        if (nt && n == nt->nu) break;
        const unsigned k = st_kind(w[2u * n]);
        if (k == 3u) break;
        if (!k) return 0;
    }
    { static const uint32_t lst[2] = { ND_FS_W0(ND_FS_MISC, ND_FS_MISC_LST << 16), 0 }; rec_op_t(lst); }
    for (unsigned i = 0; i < n; i++) if (st_kind(w[2u * i]) == 1u) nd_rec_nr(w[2u * i], w[2u * i + 1u]);
    return 1;
}
const uint32_t* nd_rec_pos(void) { return wp; }
const uint32_t* nd_rec_mark(void) {
    rec_open();
    st_hdr = NULL; /* a range bound closes the state run: the range starts or ends at a command header */
    sh_all(); /* HC4b: a range replays elsewhere */
    nd_ds_mark(wp); /* NST2D: a range bound of the unit log */
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
    nd_ds_range(&nd_ds, a, b); /* NST2D: the range units into the core 1 state */
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
uint32_t* nd_rec_last_pay(void) { return napi_last; } /* NCONV1D1 */
void* nd_rec_data_st(unsigned bytes) { st_take = 1; void* p = nd_rec_data(bytes); st_take = 0; return p; } /* NCONV1D1: a state op (no dschk record, the shadow stays) */
void nd_sub_open(NdSub* s) {
    rec_open();
    s->br = rec_take(2);
    s->br[0] = NDC_HDR(NDC_JUMP, 0, 0);
    s->br[1] = 0;
    s->a = nd_rec_mark();
    nd_ds_sub_save(); nd_ds_lg_open(); /* NST2D */
}
void nd_sub_close(NdSub* s) {
    s->b = nd_rec_mark();
    s->br[1] = (uint32_t)(uintptr_t)s->b;
    nd_ds_lg_close(); nd_ds_sub_restore(); /* NST2D */
}
void nd_sub_call(const NdSub* s) { nd_rec_call(s->a, s->b); }
/* D10a: the end of a custom gfx range (build_custom_gfx): the replay of a range (nd_range_run) stops at its NDC_END. */
void nd_rec_end(void) {
    rec_open();
    uint32_t* e = rec_take(2);
    e[0] = NDC_HDR(NDC_END, 0, 0);
    e[1] = 0;
    nd_ds_mark(wp); /* NST2D: the range end bound */
}
/* D10a: the ops of a static list (a custom gfx list that a map script set, SetCustomGfx) recorded as commands, as the walker would run the
 * list: the syncs dropped, a texture rectangle as NDC_TEXRECT, a branch followed, a push G_DL kept as a state op (walk_call at replay), the
 * list ends at its G_ENDDL. A producer record in a static list is an error (logged, the list ends). */
static __attribute__((cold, noinline)) void list_bad(const uint32_t* g, unsigned n) {
    static int k;
    if (k++ < 4) pm_log_e("ndc: static list op %08x at %p after %u words not recorded", (unsigned)g[0], (const void*)g, n);
}
void nd_rec_list(const void* l) { /* DEL1A: the typed units of the list, no G_ translation */
    uint32_t kb[2u * 64u];
    const NdNDev* ke = nd_ntab_find(l);
    const uint32_t* g = (const uint32_t*)(ke == (const NdNDev*)l ? (const void*)nd_lkey_end(l, ke, kb, 64u) : l); /* CGFX: a moved key (NDN) */
    for (unsigned n = 0; n < 1024u; n++, g += 2) {
        const uint32_t w0 = g[0], w1 = g[1]; const unsigned op = w0 >> 24;
        if (!nd_op_typed(op)) { list_bad(g, n); return; } /* HC4e4: a static list is typed at compile time */
        if (op == ND_FS_END) return;
        if (IS_NOP(w0)) continue; /* a sync or a no-op */
        if (op == ND_FS_RECT && (w1 & ND_FS_RECT_TEX)) { nd_rec_texrect(w0, w1 & ~ND_FS_RECT_TEX, g[3], g[5]); g += 4; n += 2; continue; }
        if (op == ND_FS_CALL && (w0 & 1u)) { if (!is_host(w1)) { list_bad(g, n); return; } g = (const uint32_t*)(uintptr_t)w1 - 2; continue; } /* a branch */
        if (op == ND_FS_CALL && nd_rec_inline(w1)) continue;
        nd_rec_nr(w0, w1);
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
/* NDR0A: the commands of every kind since the start (nd_snap_mesh takes a new snapshot when it changes). */
unsigned nd_rec_cmds(void) { return ndc_n[NDC_MODEL] + ndc_n[NDC_SPR] + ndc_n[NDC_ENT] + ndc_n[NDC_NAPI] - ndc_snapc + ndc_n[NDC_CALL]; }

static __attribute__((cold, noinline)) void run_bad(uint32_t h, const uint32_t* p) {
    static int n;
    if (n++ < 4) pm_log_e("ndc: bad command %08x at %p", (unsigned)h, (const void*)p);
}
__attribute__((weak)) void nd_snap_mesh(void) {} /* NDR0E2: strong in nd_snap.c; weak for the host units that link nd_rec.c */
__attribute__((weak)) void nd_snap_task(void) {} /* NAT2J: host units link nd_rec.c without nd_snap.c */
__attribute__((weak)) void nd_snap_rect(void) {} /* NDR1C */
__attribute__((weak)) void nd_msnap_rec(uint32_t* p, unsigned units) { (void)p; (void)units; } /* NREC1A */
__attribute__((weak)) void nd_msnap_pre(const NdDs* s) { (void)s; } /* NAT2C */
__attribute__((weak)) void nd_lsc_begin(void) {} /* NREC2 */
__attribute__((weak)) void nd_lsc_end_at(uint32_t* dst) { (void)dst; } /* NREC2 */
__attribute__((weak)) void nd_lsc_begin_v(unsigned v) { (void)v; } /* NREC4 */
__attribute__((weak)) void nd_snap_vprim(unsigned v) { (void)v; } __attribute__((weak)) void nd_pbeg_run(const uint32_t* r) { (void)r; } __attribute__((weak)) void nd_pdraw_run(const uint32_t* r) { (void)r; } __attribute__((weak)) void nd_pend_run(const uint32_t* r) { (void)r; } /* NAT5E */
unsigned char nd_snap_arm __attribute__((weak)); /* NDR0A: strong in nd_snap.c; weak so the host units that link nd_rec.c need no nd_snap.c */
unsigned char nd_snap_live __attribute__((weak)); /* NDR1C */
const uint32_t* nd_stream_cur; /* POSTFAIL2 (debug tool): the stream command that runs now (core 0); gbi_cold_bad_vp (nd_walk.c) logs it */
/* POSTFAIL2: the context of the chain whose chunk holds p (*chunk: its index), else -1. Cold: the bad viewport log only. */
int nd_rec_where(const void* p, unsigned* chunk) {
    for (unsigned k = 0; k < 2u; k++)
        for (unsigned i = 0; i < chain[k].nch; i++)
            if ((const uint32_t*)p >= chain[k].ch[i] && (const uint32_t*)p < chain[k].ch[i] + CH_WORDS) { *chunk = i; return (int)k; }
    return -1;
}
int nd_rec_ctx(void) { return rec_ctx; } /* POSTFAIL2: the chain core 1 records into now */
static void stream_run(const uint32_t* p, const uint32_t* stop) {
    for (;;) {
        __builtin_prefetch(p + 16); /* HC9B: the stream 64 B ahead (no hardware prefetcher on the ARM11) */
        if (p == stop) return;
        const uint32_t h = p[0];
        nd_stream_cur = p;
        { const unsigned op = h >> 24; if (op != NDC_JUMP && op != NDC_TEXRECT && op != NDC_SPR && (op != NDC_NAPI || (p[1] != (uint32_t)(uintptr_t)nd_nr_run && !(h & 1u)))) nd_snap_arm = nd_snap_live = 0; } /* NDR0A: a command but the native runs disarms the snapchk; NDR3: a snap-taking record keeps the arm */
        switch (h >> 24) {
        case NDC_END: return;
        case NDC_JUMP: p = (const uint32_t*)(uintptr_t)p[1]; continue;
        case NDC_MODEL: nd_walk_rec_model((const NdW2*)(p + 2)); break;
        case NDC_SPR: nd_walk_spr((const NdW2*)(p + 2)); break;
        case NDC_ENT: nd_walk_ent((const NdW2*)(p + 2), (h >> 16) & 0xffu); break;
        case NDC_TEXRECT: nd_texrect_run(p); break;
        case NDC_NAPI: ((NdNapiRun)(uintptr_t)p[1])(p + 2); break;
        case NDC_CALL: nd_ds_rpl++; stream_run((const uint32_t*)(uintptr_t)p[1], (const uint32_t*)(uintptr_t)p[2]); nd_ds_rpl--; break;
        default: run_bad(h, p); return;
        }
        p += 2u + 2u * ((h >> 16) & 0xffu);
    }
}
void nd_stream_run(const uint32_t* p) { stream_run(p, NULL); }
void nd_alt_run(const uint32_t* ab) { nd_ds_rpl++; stream_run((const uint32_t*)(uintptr_t)ab[0], (const uint32_t*)(uintptr_t)ab[1]); nd_ds_rpl--; } /* NAT5R */
