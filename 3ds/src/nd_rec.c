/* nd_rec.c: the frame stream (DIRECT, plan_direct.md sections 2 and 3). Record side: the game's producers append native commands to the
 * stream of the display context being built; the words that code not yet converted writes to the task list are cut into NDC_LIST spans.
 * Walker side: nd_stream_run replays one task stream (core 0, pm_frame_walk).
 * Storage: per display context a chain of 32 KB heap chunks, allocated at first use and kept (no per-frame malloc, hw rule 7). The chain of
 * a context is reset when the game starts building that context again (gCurrentDisplayContextIndex changed since the last record), so a
 * stream lives exactly as long as the list memory of its context (the depth-1 frame push of nd_wq.c keeps both safe). */
#include <stdlib.h>
#include <string.h>
#include "ultra64.h"
#include "variables.h"
#include "pm_port.h"
#include "nd.h"
#include "nd_cmd.h"
#include "gbi_custom.h"

extern s32 gCurrentDisplayContextIndex;
void nd_walk_ops(void* list, unsigned nbytes); /* nd_walk.c: a span that keeps the walker's sync state */
void nd_walk_rec_model(const Gfx* q);           /* nd_walk.c: a model record, as the walker's G_PM_REC arm */
void nd_walk_spr(const Gfx* q);                 /* nd_walk.c: a sprite record and its geometry words */
void nd_walk_ent(const Gfx* q, unsigned words); /* nd_walk.c: an entity record (and its G_DL) */
void nd_tex4_run(const uint32_t* p);            /* nd_tex_set.c: a compound texture load */
void nd_texrect_run(const uint32_t* p);         /* nd_tex_set.c: a texture rectangle */
int nd_rec_sub;

#define CH_WORDS (32768u / 4u)
#define CH_MAX 16u
typedef struct { uint32_t* ch[CH_MAX]; unsigned nch, cur; } NdChain;
static NdChain chain[2];
static int rec_ctx = -1;   /* the context of the last record (-1: none yet) */
static uint32_t* wp;       /* write position */
static uint32_t* wend;     /* end of the current chunk, less room for one NDC_JUMP */
static uint32_t* task_head; /* first command of the open task (NULL: no task open) */
static uint32_t* first_list; /* the first NDC_LIST of the open task (its start is the list base, patched at close) */
static Gfx* cut;           /* the end of the last span (NULL: the task list base, not known before close) */
static unsigned hw_units;  /* PROF: the most 4-byte words of one context stream (the high-water mark) */
static unsigned ndc_n[NDC_N]; /* PROF: commands recorded per op (relaxed counters: the logic writes, the prof report reads) */

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
/* The context of the build: a new one resets its chain. */
static void rec_ctx_check(void) {
    int x = (int)(gCurrentDisplayContextIndex & 1);
    if (x == rec_ctx) return;
    rec_ctx = x;
    ch_enter(&chain[x], 0);
    task_head = NULL;
}
/* Room for n words (header and payload): else the chunk ends with NDC_JUMP to the next one. */
static uint32_t* rec_take(unsigned n) {
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
    if (task_head) return;
    task_head = wp;
    first_list = NULL;
    cut = NULL;
}
static void span_put(const Gfx* a, unsigned bytes, unsigned first) {
    uint32_t* p = rec_take(4);
    ndc_n[NDC_LIST]++;
    p[0] = NDC_HDR(NDC_LIST, 1, first);
    p[1] = (uint32_t)(uintptr_t)a;
    p[2] = bytes;
    p[3] = 0;
    if (first) first_list = p;
}
void nd_rec_cut(void) {
    task_open();
    Gfx* g = gMainGfxPos;
    if (!cut) { /* the first span: its start (the list base) is set at close */
        span_put(NULL, (unsigned)(uintptr_t)g, 1u); /* the end pointer until close */
        cut = g;
        return;
    }
    if (g != cut) { span_put(cut, (unsigned)((uintptr_t)g - (uintptr_t)cut), 0u); cut = g; }
}
static __attribute__((cold, noinline)) void model_bad(unsigned words) {
    pm_log_e("ndc: model record of %u words (most %u)", words, G_PM_REC_WORDS + ND_REC_AUX_MAX);
    pm_fatal_stop("ndc: model record too long");
}
static __attribute__((cold, noinline)) void model_behind(const void* g) {
    static int n;
    if (n++ < 4) pm_log_e("ndc: model record with the list at %p behind the last cut %p", g, (const void*)cut);
}
void nd_rec_model(const void* rec, unsigned words) {
    if (words > G_PM_REC_WORDS + ND_REC_AUX_MAX) model_bad(words);
    if (nd_rec_sub) { /* a list for later (a custom gfx builder): the record stays in that list (walker G_PM_REC arm) */
        memcpy(gMainGfxPos, rec, words * 8u);
        gMainGfxPos += words;
        return;
    }
    nd_rec_cut();
    if (gMainGfxPos < cut) model_behind(gMainGfxPos);
    uint32_t* p = rec_take(2u + 2u * words);
    ndc_n[NDC_MODEL]++;
    p[0] = NDC_HDR(NDC_MODEL, words, 0);
    p[1] = 0;
    memcpy(p + 2, rec, words * 8u);
}
void nd_rec_spr(const void* rec, const void* geo) {
    nd_rec_cut();
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
    gSPDisplayList(&r[G_PM_ENT_WORDS], list);
    if (nd_rec_sub) { memcpy(gMainGfxPos, r, sizeof r); gMainGfxPos += G_PM_ENT_WORDS + 1; return; } /* a list for later: the walker G_PM_ENT arm */
    nd_rec_cut();
    uint32_t* p = rec_take(2u + 2u * (G_PM_ENT_WORDS + 1u));
    ndc_n[NDC_ENT]++;
    p[0] = NDC_HDR(NDC_ENT, G_PM_ENT_WORDS + 1u, 0);
    p[1] = 0;
    memcpy(p + 2, r, sizeof r);
}
void nd_rec_ent(const void* rec) {
    if (nd_rec_sub) { memcpy(gMainGfxPos, rec, G_PM_ENT_WORDS * 8u); gMainGfxPos += G_PM_ENT_WORDS; return; }
    nd_rec_cut();
    uint32_t* p = rec_take(2u + 2u * G_PM_ENT_WORDS);
    ndc_n[NDC_ENT]++;
    p[0] = NDC_HDR(NDC_ENT, G_PM_ENT_WORDS, 0);
    p[1] = 0;
    memcpy(p + 2, rec, G_PM_ENT_WORDS * 8u);
}
void nd_rec_tex4(const NdTex4* t) {
    nd_rec_cut();
    uint32_t* p = rec_take(8);
    ndc_n[NDC_TEX4]++;
    p[0] = NDC_HDR(NDC_TEX4, 3, t->flags);
    p[1] = (uint32_t)(uintptr_t)t->raster;
    p[2] = (uint32_t)(uintptr_t)t->pal;
    p[3] = t->wh;
    p[4] = t->ul;
    p[5] = t->lr;
    p[6] = t->f;
    p[7] = t->siz;
}
void nd_rec_texrect(uint32_t w0, uint32_t w1, uint32_t hs, uint32_t hd) {
    nd_rec_cut();
    uint32_t* p = rec_take(6);
    ndc_n[NDC_TEXRECT]++;
    p[0] = NDC_HDR(NDC_TEXRECT, 2, 0);
    p[1] = 0;
    p[2] = w0;
    p[3] = w1;
    p[4] = hs;
    p[5] = hd;
}
/* D4b-1: the nd_gbi_tex.h redirect. g is the packet the macro evaluated (gMainGfxPos++); it is taken back, then recorded, or written as words
 * inside a list for later (nd_rec_sub > 0). */
static __attribute__((cold, noinline)) void gx_bad(const Gfx* g) {
    pm_log_e("ndc: redirected texture macro with the packet %p, not gMainGfxPos - 1 (%p)", (const void*)g, (const void*)(gMainGfxPos - 1));
    pm_fatal_stop("ndc: redirected macro on a local list");
}
void nd_gx_tlut16(Gfx* g, unsigned slot, const void* dram) {
    if (g != gMainGfxPos - 1) gx_bad(g);
    gMainGfxPos = g;
    if (nd_rec_sub) { gDPLoadTLUT_pal16(gMainGfxPos++, slot, dram); return; }
    NdTex4 t = { NULL, dram, 1u, 0, 0, 0, (slot & 15u) << 28 };
    nd_rec_tex4(&t);
}
void nd_gx_tex4(Gfx* g, const void* timg, unsigned fmt, unsigned w, unsigned h, int uls, int ult, int lrs, int lrt, unsigned pal,
                unsigned cms, unsigned cmt, unsigned masks, unsigned maskt, unsigned shifts, unsigned shiftt, unsigned tile_form) {
    if (g != gMainGfxPos - 1) gx_bad(g);
    gMainGfxPos = g;
    if (nd_rec_sub) {
        if (tile_form) { gDPLoadTextureTile_4b(gMainGfxPos++, timg, fmt, w, h, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt); }
        else { gDPLoadTextureBlock_4b(gMainGfxPos++, timg, fmt, w, h, pal, cms, cmt, masks, maskt, shifts, shiftt); }
        return;
    }
    NdTex4 t = { timg, NULL, tile_form ? 6u : 2u, (w & 0xffffu) | (h << 16), ((unsigned)uls & 0xffffu) | ((unsigned)ult << 16),
                 ((unsigned)lrs & 0xffffu) | ((unsigned)lrt << 16),
                 (fmt & 7u) | ((cms & 3u) << 4) | ((cmt & 3u) << 6) | ((masks & 15u) << 8) | ((maskt & 15u) << 12) | ((shifts & 15u) << 16) |
                 ((shiftt & 15u) << 20) | ((pal & 15u) << 24) };
    nd_rec_tex4(&t);
}
void nd_gx_texrect(Gfx* g, int xl, int yl, int xh, int yh, unsigned tile, int s, int t, int dsdx, int dtdy) {
    if (g != gMainGfxPos - 1) gx_bad(g);
    gMainGfxPos = g;
    Gfx r[3];
    Gfx* rp = r;
    gSPTextureRectangle(rp++, xl, yl, xh, yh, tile, s, t, dsdx, dtdy);
    if (nd_rec_sub) { memcpy(gMainGfxPos, r, sizeof r); gMainGfxPos += 3; return; }
    nd_rec_texrect(r[0].words.w0, r[0].words.w1, r[1].words.w1, r[2].words.w1);
}
void nd_gx_textile(Gfx* g, const void* timg, unsigned fmt, unsigned siz, unsigned w, unsigned h, int uls, int ult, int lrs, int lrt, unsigned pal,
                   unsigned cms, unsigned cmt, unsigned masks, unsigned maskt, unsigned shifts, unsigned shiftt) {
    if (g != gMainGfxPos - 1) gx_bad(g);
    gMainGfxPos = g;
    if (nd_rec_sub) { /* the words of gDPLoadTextureTile (its siz##_TILE_BYTES and _LINE_BYTES are 0, 1, 2, 2 for 4b, 8b, 16b, 32b) */
        static const unsigned char tb[4] = { 0, 1, 2, 2 };
        unsigned line = (((unsigned)(lrs - uls + 1) * tb[siz & 3u]) + 7u) >> 3;
        gDPSetTextureImage(gMainGfxPos++, fmt, siz, w, timg);
        gDPSetTile(gMainGfxPos++, fmt, siz, line, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts);
        gDPLoadSync(gMainGfxPos++);
        gDPLoadTile(gMainGfxPos++, G_TX_LOADTILE, uls << G_TEXTURE_IMAGE_FRAC, ult << G_TEXTURE_IMAGE_FRAC, lrs << G_TEXTURE_IMAGE_FRAC, lrt << G_TEXTURE_IMAGE_FRAC);
        gDPPipeSync(gMainGfxPos++);
        gDPSetTile(gMainGfxPos++, fmt, siz, line, 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts);
        gDPSetTileSize(gMainGfxPos++, G_TX_RENDERTILE, uls << G_TEXTURE_IMAGE_FRAC, ult << G_TEXTURE_IMAGE_FRAC, lrs << G_TEXTURE_IMAGE_FRAC, lrt << G_TEXTURE_IMAGE_FRAC);
        return;
    }
    NdTex4 t = { timg, NULL, 10u, (w & 0xffffu) | (h << 16), ((unsigned)uls & 0xffffu) | ((unsigned)ult << 16),
                 ((unsigned)lrs & 0xffffu) | ((unsigned)lrt << 16),
                 (fmt & 7u) | ((cms & 3u) << 4) | ((cmt & 3u) << 6) | ((masks & 15u) << 8) | ((maskt & 15u) << 12) | ((shifts & 15u) << 16) |
                 ((shiftt & 15u) << 20) | ((pal & 15u) << 24), siz & 3u };
    nd_rec_tex4(&t);
}
void nd_gx_scisrect(Gfx* g, int xl, int yl, int xh, int yh, unsigned tile, int s, int t, int dsdx, int dtdy) {
    if (g != gMainGfxPos - 1) gx_bad(g);
    gMainGfxPos = g;
    Gfx r[3];
    Gfx* rp = r;
    gSPScisTextureRectangle(rp++, xl, yl, xh, yh, tile, s, t, dsdx, dtdy);
    if (nd_rec_sub) { memcpy(gMainGfxPos, r, sizeof r); gMainGfxPos += 3; return; }
    nd_rec_texrect(r[0].words.w0, r[0].words.w1, r[1].words.w1, r[2].words.w1);
}
const uint32_t* nd_rec_pos(void) { return wp; }
const uint32_t* nd_rec_mark(void) {
    nd_rec_cut();
    return wp;
}
void nd_rec_call(const uint32_t* a, const uint32_t* b) {
    nd_rec_cut();
    uint32_t* p = rec_take(4);
    ndc_n[NDC_CALL]++;
    p[0] = NDC_HDR(NDC_CALL, 1, 0);
    p[1] = (uint32_t)(uintptr_t)a;
    p[2] = (uint32_t)(uintptr_t)b;
    p[3] = 0;
}
static __attribute__((cold, noinline)) void close_bad(const void* list, unsigned nbytes, const void* a) {
    static int n;
    if (n++ < 4) pm_log_e("ndc: span %p outside the task list %p + %u", a, list, nbytes);
}
const uint32_t* nd_rec_close(const void* list, unsigned nbytes) {
    task_open();
    const Gfx* base = (const Gfx*)list;
    const Gfx* end = (const Gfx*)((const char*)list + nbytes);
    if (!cut) span_put(base, nbytes, 1u);
    else {
        const Gfx* fe = (const Gfx*)(uintptr_t)first_list[2]; /* the first span: base to the first cut */
        if (fe < base || fe > end) close_bad(list, nbytes, fe);
        first_list[1] = (uint32_t)(uintptr_t)base;
        first_list[2] = (unsigned)((uintptr_t)fe - (uintptr_t)base);
        if (cut < base || cut > end) close_bad(list, nbytes, cut);
        else if (cut != end) span_put(cut, (unsigned)((uintptr_t)end - (uintptr_t)cut), 0u);
    }
    uint32_t* e = rec_take(2);
    e[0] = NDC_HDR(NDC_END, 0, 0);
    e[1] = 0;
    const uint32_t* h = task_head;
    task_head = NULL;
    return h;
}
/* PROF: the commands per op since the last take, and the stream high-water mark in KB (pm_phase.c ndc line). */
void nd_rec_cnt_take(unsigned o[NDC_N], unsigned* kb) {
    static unsigned last[NDC_N];
    for (unsigned i = 0; i < NDC_N; i++) { unsigned v = ndc_n[i]; o[i] = v - last[i]; last[i] = v; }
    *kb = (hw_units * 4u + 1023u) >> 10;
}

static __attribute__((cold, noinline)) void run_bad(uint32_t h, const uint32_t* p) {
    static int n;
    if (n++ < 4) pm_log_e("ndc: bad command %08x at %p", (unsigned)h, (const void*)p);
}
static void stream_run(const uint32_t* p, const uint32_t* stop) {
    for (;;) {
        if (p == stop) return;
        const uint32_t h = p[0];
        switch (h >> 24) {
        case NDC_END: return;
        case NDC_JUMP: p = (const uint32_t*)(uintptr_t)p[1]; continue;
        case NDC_LIST:
            if (p[2]) {
                if (h & 1u) pm_gbi_run((void*)(uintptr_t)p[1], p[2]); /* the first span: a new list (full sync) */
                else nd_walk_ops((void*)(uintptr_t)p[1], p[2]);
            }
            break;
        case NDC_MODEL: nd_walk_rec_model((const Gfx*)(p + 2)); break;
        case NDC_SPR: nd_walk_spr((const Gfx*)(p + 2)); break;
        case NDC_ENT: nd_walk_ent((const Gfx*)(p + 2), (h >> 16) & 0xffu); break;
        case NDC_TEX4: nd_tex4_run(p); break;
        case NDC_TEXRECT: nd_texrect_run(p); break;
        case NDC_CALL: stream_run((const uint32_t*)(uintptr_t)p[1], (const uint32_t*)(uintptr_t)p[2]); break;
        default: run_bad(h, p); return;
        }
        p += 2u + 2u * ((h >> 16) & 0xffu);
    }
}
void nd_stream_run(const uint32_t* p) { stream_run(p, NULL); }
