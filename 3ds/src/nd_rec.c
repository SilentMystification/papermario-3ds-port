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
#include "gbi_custom.h"

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
/* The context of the build: a new one resets its chain. */
static void rec_ctx_check(void) {
    int x = (int)(gCurrentDisplayContextIndex & 1);
    if (x == rec_ctx) return;
    rec_ctx = x;
    ch_enter(&chain[x], 0);
    task_head = NULL;
    st_hdr = NULL;
}
/* Room for n words (header and payload): else the chunk ends with NDC_JUMP to the next one. */
static uint32_t* rec_take(unsigned n) {
    st_hdr = NULL; /* any record closes the open state run (nd_rec_state reopens its own) */
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
    if (!task_head) task_head = wp;
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
void nd_rec_model(const void* rec, unsigned words) {
    if (words > G_PM_REC_WORDS + ND_REC_AUX_MAX) model_bad(words);
    rec_open();
    uint32_t* p = rec_take(2u + 2u * words);
    ndc_n[NDC_MODEL]++;
    p[0] = NDC_HDR(NDC_MODEL, words, 0);
    p[1] = 0;
    memcpy(p + 2, rec, words * 8u);
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
    gSPDisplayList(&r[G_PM_ENT_WORDS], list);
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
/* D4b-3: a state op extends the open NDC_STATE run when nothing was recorded after it (no span, no other command) and the chunk has room;
 * else it opens a new run. */
void nd_rec_state(uint32_t w0, uint32_t w1) {
    rec_open();
    ndc_sw++;
    uint32_t* h = st_hdr;
    if (h && wp == st_end && ((h[0] >> 16) & 0xffu) < 255u && wp + 2u <= wend) {
        h[0] += 1u << 16;
        uint32_t* p = rec_take(2);
        p[0] = w0;
        p[1] = w1;
        st_hdr = h;
        st_end = wp;
        return;
    }
    uint32_t* p = rec_take(4);
    ndc_n[NDC_STATE]++;
    p[0] = NDC_HDR(NDC_STATE, 1, 0);
    p[1] = 0;
    p[2] = w0;
    p[3] = w1;
    st_hdr = p;
    st_end = wp;
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
static const uint8_t st_ok[256] = {
    [G_NOOP] = 1, [G_SETOTHERMODE_H] = 1, [G_SETOTHERMODE_L] = 1, [G_GEOMETRYMODE] = 1, [G_TEXTURE] = 1, [G_SETCOMBINE] = 1,
    [G_SETPRIMCOLOR] = 1, [G_SETENVCOLOR] = 1, [G_SETFOGCOLOR] = 1, [G_SETBLENDCOLOR] = 1, [G_SETFILLCOLOR] = 1, [G_SETTIMG] = 1,
    [G_SETTILE] = 1, [G_LOADBLOCK] = 1, [G_LOADTILE] = 1, [G_LOADTLUT] = 1, [G_SETTILESIZE] = 1, [G_SETSCISSOR] = 1, [G_SETPRIMDEPTH] = 1,
    [G_SETCONVERT] = 1, [G_SETKEYR] = 1, [G_SETKEYGB] = 1, [G_MOVEWORD] = 1, [G_MOVEMEM] = 1, [G_MTX] = 1, [G_POPMTX] = 1,
    [G_RDPLOADSYNC] = 2, [G_RDPPIPESYNC] = 2, [G_RDPTILESYNC] = 2, [G_RDPFULLSYNC] = 2, [G_SPNOOP] = 2,
};
static int rec_inline(uint32_t a) {
    if (a < 0x00100000u || a >= (uint32_t)(uintptr_t)__end__ || (a & 7u) || a - (uint32_t)(uintptr_t)gMapShapeData < 0x40000u) return 0;
    const Gfx* l = (const Gfx*)(uintptr_t)a;
    unsigned n = 0;
    for (;; n++) {
        if (n == 64u) return 0;
        const unsigned op = l[n].words.w0 >> 24;
        if (op == G_ENDDL) break;
        if (!st_ok[op]) return 0;
    }
    nd_rec_state((uint32_t)G_PM_LST << 24, 0);
    for (unsigned i = 0; i < n; i++) if (st_ok[l[i].words.w0 >> 24] == 1) nd_rec_state(l[i].words.w0, l[i].words.w1);
    return 1;
}
static uint32_t tr_w[3];
static unsigned tr_n; /* the words of the texture rectangle being recorded */
static __attribute__((cold, noinline)) void tr_bad(uint32_t w0) {
    pm_log_e("ndc: texture rectangle word %u followed by %08x", tr_n, (unsigned)w0);
    pm_fatal_stop("ndc: broken texture rectangle");
}
void nd_gx_w(Gfx* g, uint32_t w0, uint32_t w1) {
    if (g != gMainGfxPos - 1) { g->words.w0 = w0; g->words.w1 = w1; return; }
    const unsigned op = w0 >> 24;
    if (op == G_DL && ((w0 >> 16) & 0xffu) == G_DL_NOPUSH) { g->words.w0 = w0; g->words.w1 = w1; return; }
    gMainGfxPos = g;
    if (op == G_ENDDL) return; /* D7: the end of a task list or of a custom gfx builder (NDC_END closes both) */
    if (__builtin_expect(tr_n != 0, 0)) {
        if (tr_n == 1 && op == G_RDPHALF_1) { tr_w[2] = w1; tr_n = 2; return; }
        if (tr_n == 2 && op == G_RDPHALF_2) { tr_n = 0; nd_rec_texrect(tr_w[0], tr_w[1], tr_w[2], w1); return; }
        tr_bad(w0);
    }
    if (op == G_TEXRECT || op == G_TEXRECTFLIP) { tr_w[0] = w0; tr_w[1] = w1; tr_n = 1; return; }
    if (op == G_RDPPIPESYNC || op == G_RDPTILESYNC || op == G_RDPLOADSYNC || op == G_RDPFULLSYNC || op == G_SPNOOP) return;
    if (op == G_DL && rec_inline(w1)) return;
    nd_rec_state(w0, w1);
}
const uint32_t* nd_rec_pos(void) { return wp; }
const uint32_t* nd_rec_mark(void) {
    rec_open();
    st_hdr = NULL; /* a range bound closes the state run: the range starts or ends at a command header */
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
        const u32 w0 = g->words.w0, w1 = g->words.w1;
        const unsigned op = w0 >> 24;
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
            if (!rec_inline(w1)) nd_rec_state(w0, w1);
            break;
        case G_PM_REC: case G_PM_SPR: case G_PM_ENT: list_bad(g, n); return;
        default: nd_rec_state(w0, w1); break;
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
        case NDC_CALL: stream_run((const uint32_t*)(uintptr_t)p[1], (const uint32_t*)(uintptr_t)p[2]); break;
        default: run_bad(h, p); return;
        }
        p += 2u + 2u * ((h >> 16) & 0xffu);
    }
}
void nd_stream_run(const uint32_t* p) { stream_run(p, NULL); }
