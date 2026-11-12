/* nd_ent.c: the entity path (plan_s10_path group E, LS-2): the entity draw lists compiled from the static pools (ent_compile,
 * ent_run, ent_gdl, ent_draw), their key table and the record case of G_PM_ENT (nd_ent_case, called by the walk). Cut from
 * the former pm_gbi.c and nd_shape.c without logic change (D12). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_int.h"
#include "nd_cmd.h"
#include "nd_op.h"
#include "pm_gpu.h"
#include "nd.h"
#include "nd_scomp.h"
#include "nd_gtab.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"



/* A texture tile as the RDP holds it. Tile 0 is the render tile. A combiner that reads only TEXEL1
 * samples tile 1 (draw_box takes its window body from tile 1). One texture is bound at a time,
 * so the tile the next draw needs is bound just before it. */
/* S6-2a (entcheck): at the end record of an entity model draw (G_PM_ENT with G_PM_ENT_END, entity_model.c), the walker's tile 0 state (td[0], pending for the
 * next draw) against nd_dlc_tex_decode of the model list. eq: the same src, fmt, siz, w, h, stride, origin 0, and pal and pal_n for CI. Counts: ent (lists with a texture op),
 * nat (NDTX_OK, native-ready), fb per code (NDTX_MULTI and the fallback codes, index 2 to 8). Line every 300 frames. */
void pm_gbi_ent_texchk(const uint32_t* dl) {
    static unsigned n_ent, n_eq, n_diff, n_nat, n_fb[NDTX_N], n_dlog, n_tbad, next = 300u;
    NdTexDec d;
    int c = nd_dlc_tex_decode(dl, &d);
    unsigned f;
    extern void pm_log_prof_line(const char* fmt, ...);
    extern unsigned pm_frame_count(void);
    if (c != NDTX_NONE) {
        n_ent++;
        if (c == NDTX_OK) n_nat++; else n_fb[c]++;
        if (c == NDTX_WIN && n_tbad++ < 6) pm_log_prof_line("texwin f%u w %u h %u ldw %u ldh %u tsiz %u", pm_frame_count(), d.w, d.h, d.stride, d.pal_n, d.fmt);
        if (c == NDTX_TILE && n_tbad++ < 6) pm_log_prof_line("texbad f%u op %02x tile %u", pm_frame_count(), d.bad_op >> 8, d.bad_op & 7u);
        if (c == NDTX_OK || c == NDTX_MULTI) {
            int same = nd_ws.td[0].valid && (const void*)nd_ws.td[0].src == d.img && nd_ws.td[0].fmt == d.fmt && nd_ws.td[0].siz == d.siz && nd_ws.td[0].w == (int)d.w && nd_ws.td[0].h == (int)d.h &&
                       nd_ws.td[0].stride == (int)d.stride && nd_ws.td[0].x0 == 0 && nd_ws.td[0].y0 == 0 && (d.fmt != 2u || (nd_ws.td[0].pal == d.pal && nd_ws.td[0].pal_n == (int)d.pal_n));
            if (same) n_eq++;
            else {
                n_diff++;
                if (n_dlog++ < 8) pm_log_prof_line("texdiff f%u td v%d src %p fmt %u siz %u w %d h %d str %d x0 %d y0 %d pal %p n %d / dec src %p fmt %u siz %u w %u h %u str %u pal %p n %u",
                    pm_frame_count(), nd_ws.td[0].valid, (const void*)nd_ws.td[0].src, nd_ws.td[0].fmt, nd_ws.td[0].siz, nd_ws.td[0].w, nd_ws.td[0].h, nd_ws.td[0].stride, nd_ws.td[0].x0, nd_ws.td[0].y0, nd_ws.td[0].pal, nd_ws.td[0].pal_n,
                    d.img, d.fmt, d.siz, d.w, d.h, d.stride, d.pal, d.pal_n);
            }
        }
    }
    f = pm_frame_count();
    if (f >= next) {
        next = (f / 300u + 1u) * 300u;
        pm_log_prof_line("texchk f%u ent %u eq %u diff %u nat %u fb %u %u %u %u %u %u %u", f, n_ent, n_eq, n_diff, n_nat, n_fb[2], n_fb[3], n_fb[4], n_fb[5], n_fb[6], n_fb[7], n_fb[8]);
    }
}

/* S6-2c-2 (entcheck census): at the end record of an entity model draw, the list cut at its state ops (nd_ent_segments). Counts per draw: ok (the list
 * compiles to segments), refuse (NDR_OP and the other refusals), the first refused op code, segments and triangles. Line every 300 frames: entseg. */
void pm_gbi_ent_segchk(const uint32_t* dl) {
    static NdEntSeg seg[64];
    static unsigned n_draw, n_ok, n_ref, n_seg, n_tri, n_bad[6], next = 300u;
    static uint32_t last_bad;
    static unsigned n_opc[256];
    unsigned ns = 0, f;
    uint32_t bad = 0;
    extern void pm_log_prof_line(const char* fmt, ...);
    extern unsigned pm_frame_count(void);
    int rc = nd_ent_segments(dl, seg, 64u, &ns, &bad);
    n_draw++;
    if (rc == NDR_OK) {
        n_ok++; n_seg += ns;
        for (unsigned i = 0; i < ns; i++) n_tri += seg[i].ntri;
    } else {
        n_ref++;
        last_bad = bad;
        n_opc[bad & 0xFFu]++;
        if (n_bad[rc]++ < 1) pm_log_prof_line("entseg refuse f%u rc %d op %02x", pm_frame_count(), rc, bad);
    }
    f = pm_frame_count();
    if (f >= next) {
        next = (f / 300u + 1u) * 300u;
        pm_log_prof_line("entseg f%u draws %u ok %u refuse %u segs %u tri %u lastbad %02x", f, n_draw, n_ok, n_ref, n_seg, n_tri, last_bad);
        for (unsigned k = 0; k < 256u; k++) if (n_opc[k] && (k == 0x0Bu || k == 0x0Du || k == 0x0Fu || k == 0x11u || k == 0x09u || k == 0x08u || k == 0x07u || k == 0x02u || k == 0x03u || k == 0x04u || k >= 0xD0u)) pm_log_prof_line("entseg op %02x n %u", k, n_opc[k]);
    }
}

unsigned ent_nent, ent_gen_seen;

unsigned ent_opc[256];

EntCount ent_c;

GdlCount gdl_c; /* SGEN-1: the generic G_DL path */
unsigned ent_skipgen; /* the G_DL after a draw record that ent_draw refused: no second try */
int entchk_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("entcheck"); return v; }

GdlCount gdl_c; /* SGEN-1: the generic G_DL path */
unsigned ent_skipgen; /* the G_DL after a draw record that ent_draw refused: no second try */

const void* ent_env_vtx(uint32_t w1, unsigned n) {
    const Vtx* src = (const Vtx*)ptr_of(w1);
    if (!src || !is_host((u32)src) || n == 0 || n > 64) return NULL;
    return src;
}

static inline unsigned ent_uid(u32 w0, u32 w1) { (void)w1; return w0 >> 24; }

/* S6-5: the words of the list and of every nested list (G_DL), then the vertex span */
u32 ent_hash_walk(const Gfx* l, u32 h, int depth) {
    for (unsigned i = 0; i < ND_DLC_MAXOPS; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        h = (h ^ w0) * 16777619u; h = (h ^ w1) * 16777619u;
        uint32_t ut[2]; ut[0] = w0; ut[1] = w1;
        if ((ut[0] >> 24) == NDO_END) return h;
        if ((ut[0] >> 24) == NDO_CALL) {
            const Gfx* t = (const Gfx*)ptr_of(w1);
            if (t && is_host((u32)t) && depth < 5) h = ent_hash_walk(t, h, depth + 1);
            if (ut[0] & 1) return h;
        }
    }
    return h;
}

u32 ent_hash(const Gfx* l, const u8* vp, u32 vlen) {
    u32 h = ent_hash_walk(l, 2166136261u, 0);
    for (u32 i = 0; i < vlen; i += 4u) { u32 v; memcpy(&v, vp + i, 4); h = (h ^ v) * 16777619u; }
    return h;
}

int ent_flat(const Gfx* l, unsigned* n, int depth) {
    for (unsigned i = 0; i < ND_DLC_MAXOPS; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        uint32_t ut[2]; ut[0] = w0; ut[1] = w1;
        unsigned op = ut[0] >> 24;
        if (op == NDO_END) return 1;
        if (op == NDO_CALL) {
            const Gfx* t = (const Gfx*)ptr_of(w1);
            if (!t || !is_host((u32)t) || depth >= 5 || !ent_flat(t, n, depth + 1)) return 0;
            if (ut[0] & 1) return 1;
            continue;
        }
        if (*n >= ND_DLC_MAXOPS) return 0;
        ent_flat_buf[(*n)++] = l[i];
    }
    return 0;
}

/* the pool copies against the list: every triangle corner is the vertex the slot holds at the triangle (bytes 0 to 5 and 8 to 15; the flag word is the palette index). Per segment. */
void ent_refcheck(const EntKey* k, const Gfx* l) {
    const u8* slot[64];
    u8 bad[ENT_SEGMAX];
    unsigned gt = 0, s = 0;
    memset(slot, 0, sizeof slot);
    memset(bad, 0, sizeof bad);
    for (u32 i = 0; i < k->nops; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        unsigned op = ent_uid(w0, w1);
        if (op == NDO_VTX) {
            unsigned n = (w0 >> 12) & 0xff, e = (w0 >> 1) & 0x7f, v0 = e >= n ? e - n : 0;
            const u8* src = (const u8*)ent_env_vtx(w1, n);
            if (v0 + n > 64) n = 64 - v0;
            for (unsigned t = 0; t < n; t++) slot[v0 + t] = src ? src + 16u * t : NULL;
        } else if (op == NDO_TRI1 || op == NDO_TRI2) {
            for (int t = 0; t < (op == NDO_TRI2 ? 2 : 1); t++, gt++) {
                u32 w = t ? w1 : w0;
                int ix[3] = { (int)((w >> 16) & 0xff) / 2, (int)((w >> 8) & 0xff) / 2, (int)(w & 0xff) / 2 };
                while (s + 1 < k->nseg && gt >= k->seg[s].tri0 + k->seg[s].ntri) s++;
                for (int c = 0; c < 3; c++) {
                    const u8* p = ix[c] < 64 ? slot[ix[c]] : NULL;
                    const u8* q = k->vtx + 16u * (k->idx[3u * gt + (unsigned)c] - k->vfirst);
                    if (!p || memcmp(p, q, 6) != 0 || memcmp(p + 8, q + 8, 8) != 0) bad[s] = 1;
                }
            }
        }
    }
    for (unsigned q = 0; q < k->nseg; q++) {
        if (!k->seg[q].ntri) continue;
        ent_c.cseg++;
        if (bad[q] || gt != k->seg[k->nseg - 1].tri0 + k->seg[k->nseg - 1].ntri) ent_c.cdiff++; else ent_c.ceq++;
    }
}

/* CORE2-2 census: a flattened list (fw) is walker memory; when the walker frees it, its census entries go (the game may reuse the block). */
extern int nd_c2_on;
void nd_c2_forget_blk(const void* blk);
static inline void nd_c2_forget(const void* blk) { if (nd_c2_on > 0 && blk) nd_c2_forget_blk(blk); }
/* MERLEE: a list in a game heap (the general heap, or the battle heap, which is gFrameBuf2) was built at run time: the imgfx animated mesh lists
 * (imgfx.c imgfx_load_anim) point at vertex buffers that imgfx_mesh_anim_update rewrites every frame. Its vertex span goes into the per-frame hash
 * (ent_run), as for a keyed list; without it the first compiled pose was drawn for the whole animation. Static lists (entity, map, overlay data) keep the list-only hash. */
static int ent_heap_list(const Gfx* l) {
    extern u8 heap_generalHead[]; extern u16 gFrameBuf2[];
    return (u32)l - (u32)heap_generalHead < (u32)GENERAL_HEAP_SIZE || (u32)l - (u32)gFrameBuf2 < (u32)BATTLE_HEAP_SIZE;
}
unsigned nd_ent_sc; /* S6E: gaps run as compiled ranges (entsync line) */
static NdSDef* ent_sdef(const Gfx* l, u32 a, u32 b) { /* the gap [a, b) of l as an S6T def (1 to 32 units), else NULL */
    NdSDef* d = (b <= a || b - a > 32u) ? NULL : (NdSDef*)calloc(1, sizeof *d);
    if (d) { d->u = l + a; d->n = (uint8_t)(b - a); } return d; }
static void ent_sdef_free(EntKey* k) { for (unsigned s = 0; s < k->nseg && k->seg; s++) free(k->seg[s].sd); free(k->tail); k->tail = NULL; free(k->pc); k->pc = NULL; k->tpc0 = k->tpcn = 0; }
/* S7ENT: the pieces of the S7 table of a list (nd_gtab.h) in flat order (ent_flat): a def, a unit, a no-op unit. The walk follows the table's list as ent_flat
 * follows the words: a call is entered (its callee table is the call op's p), a branch-form call ends the walk. A piece that lies inside a gap [ga, gb) of the
 * list is recorded for it; a piece that crosses a gap edge marks the gap bad. A gap is covered when its pieces tile it (their units add up to its length). */
typedef struct { u16 g; EntPiece pc; } EntTP;
#define ENT_TPMAX 1024u
static EntTP ent_tp[ENT_TPMAX];
static unsigned ent_ntp, ent_ngap, ent_gc;
static u32 ent_ga[ENT_SEGMAX + 1u], ent_gb[ENT_SEGMAX + 1u], ent_gn[ENT_SEGMAX + 1u];
static u8 ent_gbad[ENT_SEGMAX + 1u];
static void ent_tpiece(u32 at, u32 n, u8 kind, const void* p) {
    for (unsigned g = ent_gc; g < ent_ngap; g++) {
        if (ent_gb[g] <= at) { ent_gc = g + 1u; continue; }
        if (ent_ga[g] >= at + n) break;
        if (ent_ga[g] <= at && at + n <= ent_gb[g] && ent_ntp < ENT_TPMAX) {
            ent_tp[ent_ntp].g = (u16)g; ent_tp[ent_ntp].pc.p = p; ent_tp[ent_ntp].pc.n = (u16)n; ent_tp[ent_ntp].pc.kind = kind; ent_ntp++;
            ent_gn[g] += n;
        } else ent_gbad[g] = 1;
        if (at + n <= ent_gb[g]) break;
    }
}
static int ent_tdwalk(const NdGTab* t, unsigned* flat, int depth) {
    const uint32_t* L = t->list;
    const NdGOp* o = t->op;
    for (unsigned k = 0; k < ND_DLC_MAXOPS; ) {
        const uint32_t w0 = L[2u * k];
        const unsigned id = w0 >> 24;
        if (id == NDO_END) return 0;
        if (w0 == ND_NOP_A) { ent_tpiece(*flat, 1u, ENP_SKIP, NULL); (*flat)++; k++; continue; }
        if (o->k == NDG_END) return 0;
        if (id == NDO_CALL) {
            if (o->k != NDG_CALL || depth >= 5) return -1;
            const int r = ent_tdwalk((const NdGTab*)o->p, flat, depth + 1);
            if (r < 0) return r;
            o++; k++;
            if (w0 & 1u) return 1;
            continue;
        }
        if (o->k == NDG_DEF) {
            const NdGDef* g = (const NdGDef*)o->p;
            if (g->k0 != k) return -1;
            ent_tpiece(*flat, g->d.n, ENP_DEF, g);
            *flat += g->d.n; k += g->d.n; o++;
            continue;
        }
        ent_tpiece(*flat, 1u, ENP_UNIT, &L[2u * k]);
        o += (id == NDO_TRI2) ? 2 : 1;
        (*flat)++; k++;
    }
    return -1;
}
unsigned nd_ent_tab, nd_ent_notab, nd_ent_gapt, nd_ent_gaps, nd_ent_gapn; /* S7ENT census (entsync line): lists compiled with a table, without; gaps run as a chain of table pieces, gaps from ent_sdef in tabled lists, gaps of lists without a table */
void ent_compile(EntKey* k, const Gfx* l0) {
    pm_xf.ent_n++; /* MAPX-1 */
    unsigned ns = 0, nops = 0;
    u32 bad = 0, ntri = 0;
    const u8 *lo = NULL, *hi = NULL;
    NdObj o;
    ent_sdef_free(k);
    free(k->seg); k->seg = NULL; k->nseg = 0;
    nd_c2_forget(k->fw); free(k->fw); k->fw = NULL;
    k->st = ENT_REF; k->why = 0; k->fr = pm_frame_count(); k->retry = k->fr + 300u;
    ent_c.comp++;
    if (!ent_flat(l0, &nops, 0)) { k->why = 5; return; }
    ent_flat_buf[nops].words.w0 = NDO_W0(NDO_END, 0); ent_flat_buf[nops].words.w1 = 0;
    if (!k->vb) { for (unsigned i = 0; i < nops; i++) if (ent_uid(ent_flat_buf[i].words.w0, ent_flat_buf[i].words.w1) == NDO_VTX) { u32 a = ent_flat_buf[i].words.w1; if (!is_host(a) && (a >> 24) > 0 && (a >> 24) < 16u) { k->useseg = 1; break; } } if (k->useseg) { k->why = 9; return; } }
    k->fw = (Gfx*)malloc((nops + 1u) * sizeof(Gfx));
    if (!k->fw) { k->why = 5; return; }
    memcpy(k->fw, ent_flat_buf, (nops + 1u) * sizeof(Gfx));
    const Gfx* l = k->fw;
    const u32* w = (const u32*)l;
    int rc = nd_ent_segments(w, ent_sbuf, ENT_SEGMAX, &ns, &bad);
    if (rc != NDR_OK) { k->why = (u8)(rc + 1); if (rc == NDR_OP) ent_opc[bad & 0xffu]++; return; }
    if (ns == 0) { k->why = 8; return; }
    nd_ent_strip(w, dlc_words);
    for (unsigned s = 0; s < ns; s++) ntri += ent_sbuf[s].ntri;
    /* BTLUI: a list whose vertex loads feed a later list (no triangle after its last G_VTX, e.g. the damage star corners of
     * damage_indicator D_09002150..80) stays on the walker: ent_run skips loads without triangles and clears the walker slots. */
    { unsigned lastv = 0, lastt = 0;
      for (unsigned i = 0; i < nops; i++) { unsigned op = ent_uid(ent_flat_buf[i].words.w0, ent_flat_buf[i].words.w1);
          if (op == NDO_VTX) lastv = i + 1; else if (op == NDO_TRI1 || op == NDO_TRI2) lastt = i + 1; }
      if (ntri == 0 || lastv > lastt) { k->why = 8; return; } }
    if (k->vb || ent_heap_list(l0)) for (unsigned i = 0; i < nops; i++) { /* the vertex span of the vertex array: hashed once per frame */
        if (ent_uid(l[i].words.w0, l[i].words.w1) != NDO_VTX) continue;
        const u8* p = (const u8*)ptr_of(l[i].words.w1);
        unsigned n = (l[i].words.w0 >> 12) & 0xff;
        if (!p || !is_host((u32)p)) continue;
        if (!lo || p < lo) lo = p;
        if (!hi || p + 16u * n > hi) hi = p + 16u * n;
    }
    if (lo && (u32)(hi - lo) > 0x20000u) { k->why = 5; return; }
    NdDlcEnv env = { ent_env_vtx, nd_env_valloc, nd_env_ialloc, NULL, 0 };
    unsigned dp_t0 = pm_ticks(); /* DPCEN2: compile time of the entity list */
    rc = nd_dlc_compile(&o, dlc_words, &env);
    { extern unsigned pm_dp_cmp_n, pm_dp_cmp_us; pm_dp_cmp_n++; pm_dp_cmp_us += (pm_ticks() - dp_t0) / 268u; }
    if (rc != NDR_OK) { k->why = (u8)(rc + 1); return; }
    if (o.nidx != 3u * ntri) { nd_dlc_free(&o); k->why = 7; return; }
    k->seg = (EntSeg*)malloc(ns * sizeof(EntSeg));
    if (!k->seg) { nd_dlc_free(&o); k->why = 5; return; }
    const NdGTab* gt = nd_gtab_find(l0);
    for (unsigned s = 0; s < ns; s++) { ent_ga[s] = s ? ent_sbuf[s - 1].op1 : 0u; ent_gb[s] = ent_sbuf[s].op0; }
    ent_ga[ns] = ent_sbuf[ns - 1].op1; ent_gb[ns] = nops;
    ent_ngap = ns + 1u; ent_ntp = 0; ent_gc = 0;
    memset(ent_gn, 0, ent_ngap * sizeof ent_gn[0]); memset(ent_gbad, 0, ent_ngap);
    if (gt) { unsigned fl = 0; if (ent_tdwalk(gt, &fl, 0) < 0) memset(ent_gbad, 1, ent_ngap); nd_ent_tab++; } else { memset(ent_gbad, 1, ent_ngap); nd_ent_notab++; }
    k->pc = NULL; k->tpc0 = k->tpcn = 0;
    { unsigned tot = 0;
      for (unsigned i = 0; i < ent_ntp; i++) { const unsigned g = ent_tp[i].g; if (!ent_gbad[g] && ent_gb[g] > ent_ga[g] && ent_gn[g] == ent_gb[g] - ent_ga[g]) tot++; }
      if (tot) k->pc = (EntPiece*)malloc(tot * sizeof(EntPiece)); }
    unsigned npc = 0;
    for (unsigned g = 0; g < ent_ngap; g++) {
        u16 pc0 = 0, pcn = 0;
        if (ent_gb[g] > ent_ga[g]) {
            if (!ent_gbad[g] && ent_gn[g] == ent_gb[g] - ent_ga[g] && k->pc) {
                pc0 = (u16)npc;
                for (unsigned i = 0; i < ent_ntp; i++) if (ent_tp[i].g == g) { k->pc[npc++] = ent_tp[i].pc; pcn++; }
                nd_ent_gapt++;
            } else if (gt) nd_ent_gaps++; else nd_ent_gapn++;
        }
        if (g < ns) {
            k->seg[g].a0 = ent_ga[g]; k->seg[g].op0 = ent_gb[g]; k->seg[g].op1 = ent_sbuf[g].op1;
            k->seg[g].tri0 = ent_sbuf[g].tri0; k->seg[g].ntri = ent_sbuf[g].ntri;
            k->seg[g].pc0 = pc0; k->seg[g].pcn = pcn;
            k->seg[g].sd = pcn ? NULL : ent_sdef(l, ent_ga[g], ent_gb[g]);
        } else { k->tpc0 = pc0; k->tpcn = pcn; k->tail = pcn ? NULL : ent_sdef(l, ent_ga[g], ent_gb[g]); }
    }
    k->nseg = (u16)ns; k->nops = nops;
    k->idx = o.idx; k->vtx = o.vtx; k->vfirst = o.vfirst; k->nvtx = o.nvtx;
    if (o.nvtx) pm_gpu_gstat_commit(o.vfirst, o.nvtx);
    if (o.nidx) pm_gpu_gidxs_commit(o.idx, o.nidx);
    nd_dlc_free(&o); /* the groups and geometry ops; the pool copies stay */
    k->vp = lo; k->vlen = lo ? (u32)(hi - lo) & ~3u : 0u;
    k->hash = ent_hash(l0, k->vp, k->vlen);
    k->st = ENT_OK;
    if (entchk_on()) ent_refcheck(k, l);
}

void ent_reset(void) {
    for (unsigned i = 0; i < ENT_TAB; i++) if (ent_tab[i]) { ent_sdef_free(ent_tab[i]); free(ent_tab[i]->seg); nd_c2_forget(ent_tab[i]->fw); free(ent_tab[i]->fw); free(ent_tab[i]); ent_tab[i] = NULL; }
    ent_nent = 0; ent_gen_seen = pm_dlc_gen;
}

EntKey* ent_lookup(const Gfx* l, u32 vb) {
    unsigned h = (((u32)l >> 3) * 2654435761u ^ vb * 40503u) >> 20 & (ENT_TAB - 1);
    for (unsigned n = 0; n < ENT_TAB; n++, h = (h + 1) & (ENT_TAB - 1)) {
        EntKey* k = ent_tab[h];
        if (!k) {
            if (ent_nent >= ENT_TAB * 3 / 4) return NULL;
            k = (EntKey*)calloc(1, sizeof *k);
            if (!k) return NULL;
            k->list = (u32)l; k->vb = vb; k->id = ent_nent;
            ent_tab[h] = k; ent_nent++;
            ent_compile(k, l);
            return k;
        }
        if (k->list == (u32)l && k->vb == vb) return k;
    }
    return NULL;
}

extern unsigned char pm_ent_decal_ok; /* pm_gpu.c: pm_gpu_grange takes a decal with blend (two native passes at the close) */
unsigned ent_sw[16], ent_gw[8]; /* ent_gw: pm_gpu_grange_why of a grange refusal (1 gv or gpu off, 2 zmark, 3 decal and blend, 4 wide, 5 ring) */ /* segments the static path refused, by reason: 1 lit, 2 fog, 3 index room, 4 matrix table, 5 + dlc_seg code (1 + gv state why, 9 grange) */
__attribute__((noinline)) int ent_seg_native(const EntKey* k, const EntSeg* s) {
    extern unsigned char pm_ent_decal_ok;
    if (nd_ws.geom & G_LIGHTING) { ent_sw[1]++; return 0; }
    if (fogmix_on && (nd_ws.geom & G_FOG) && !gv_fog_gpu_ok()) { ent_sw[2]++; return 0; }
    if (!pm_gpu_gidx_room(3u * s->ntri)) { ent_sw[3]++; return 0; }
    pm_gpu_set_mode((nd_ws.geom & G_ZBUFFER) != 0, (nd_ws.geom & G_LIGHTING) != 0);
    unsigned gmi = dlc_gmi();
    if (gmi == GV_NONE) { ent_sw[4]++; return 0; }
    pm_ent_decal_ok = 1;
    int r = dlc_seg(k->idx + 3u * s->tri0, s->ntri, gmi);
    pm_ent_decal_ok = 0;
    if (r) return 1;
    ent_sw[5 + (dbg_seg < 10 ? dbg_seg : 10)]++;
    if (dbg_seg == 9) { pm_ent_decal_ok = 1; ent_gw[pm_gpu_grange_why(gm_tab[gmi], 3u * s->ntri, k->idx + 3u * s->tri0) & 7]++; pm_ent_decal_ok = 0; }
    ((EntKey*)k)->nint++;
    return 0;
}

extern unsigned char pm_ent_decal_ok; /* pm_gpu.c: pm_gpu_grange takes a decal with blend (two native passes at the close) */
unsigned ent_sw[16], ent_gw[8]; /* ent_gw: pm_gpu_grange_why of a grange refusal (1 gv or gpu off, 2 zmark, 3 decal and blend, 4 wide, 5 ring) */ /* segments the static path refused, by reason: 1 lit, 2 fog, 3 index room, 4 matrix table, 5 + dlc_seg code (1 + gv state why, 9 grange) */

void ent_log(void) { /* PROF2-3: census lines, from pm_gpu.c cen_lines_frame (frame end, pm_cen_lines) */
    static unsigned next = 300u;
    unsigned f = pm_frame_count();
    extern void pm_log_w(const char* fmt, ...);
    if (f < next) return;
    next = (f / 300u + 1u) * 300u;
    pm_log_w("entnat f%u draws %u native %u fb %u segs native %u interp %u keys %u comp %u dyn %u", f, ent_c.draws, ent_c.nat, ent_c.fb, ent_c.segn, ent_c.segi, ent_nent, ent_c.comp, ent_c.dyn);
    { static unsigned l0, l1, l2, l3; pm_log_w("entsync f%u walks %u csync %u rsync %u gdl %u sc %u (since the last line) tab %u notab %u gapt %u gaps %u gapn %u", f, nd_ent_gbi - l0, nd_ent_cfull - l1, nd_ent_rfull - l2, gdl_c.lists, nd_ent_sc - l3, nd_ent_tab, nd_ent_notab, nd_ent_gapt, nd_ent_gaps, nd_ent_gapn); l0 = nd_ent_gbi; l1 = nd_ent_cfull; l2 = nd_ent_rfull; l3 = nd_ent_sc; }
    pm_log_w("entnat why 1 table %u 2 op %u 3 slot %u 4 vtx %u 5 long %u 6 pool %u 7 count %u 8 none %u", ent_c.why[1], ent_c.why[2], ent_c.why[3], ent_c.why[4], ent_c.why[5], ent_c.why[6], ent_c.why[7], ent_c.why[8]);
    for (unsigned q = 0; q < 256u; q++) if (ent_opc[q]) pm_log_w("entnat refused op %02x keys %u", q, ent_opc[q]);
    for (unsigned q = 0, n = 0; q < ENT_TAB && n < 16u; q++) if (ent_tab[q] && (ent_tab[q]->nint || ent_tab[q]->st != ENT_OK)) { const EntKey* k = ent_tab[q]; n++; pm_log_w("entnat key list %08x vb %08x st %u why %u nseg %u nops %u draws %u seg %u int %u", k->list, k->vb, k->st, k->why, k->nseg, k->nops, k->ndraw, k->segid, k->nint); }
    pm_log_w("entnat grange why 0 %u 1 %u 2 %u 3 %u 4 %u 5 %u", ent_gw[0], ent_gw[1], ent_gw[2], ent_gw[3], ent_gw[4], ent_gw[5]);
    pm_log_w("entnat segwhy lit %u fog %u room %u mtx %u | dlc_seg code (0 none, 1 + gv why, 9 grange) 0:%u 1:%u 2:%u 3:%u 4:%u 5:%u 6:%u 7:%u 8:%u 9:%u", ent_sw[1], ent_sw[2], ent_sw[3], ent_sw[4], ent_sw[5], ent_sw[6], ent_sw[7], ent_sw[8], ent_sw[9], ent_sw[10], ent_sw[11], ent_sw[12], ent_sw[13], ent_sw[14]);
    { extern void pm_gpu_nraw_why(unsigned* o); unsigned nw[10]; pm_gpu_nraw_why(nw); extern unsigned pm_gpu_cmd_split_n(void); extern void pm_gpu_fb2_stat(unsigned* o); unsigned fb[3]; pm_gpu_fb2_stat(fb); pm_log_w("nrawwhy f%u zmark %u tex1 %u unit0 %u tmpl %u cmdsplit %u tmplhi %u evict %u", f, nw[4], nw[6], nw[8], nw[9], pm_gpu_cmd_split_n(), fb[0], fb[1]); }
    pm_log_w("gdl f%u lists %u native %u dyn %u refuse %u skip %u draws native %u walker %u", f, gdl_c.lists, gdl_c.nat, gdl_c.dyn, gdl_c.ref, gdl_c.skip, ent_c.segn, ent_c.segi);
    if (entchk_on()) pm_log_w("entsegcmp f%u segs %u eq %u diff %u", f, ent_c.cseg, ent_c.ceq, ent_c.cdiff);
}

EntKey* ent_find(const Gfx* l, u32 vb) {
    unsigned h = (((u32)l >> 3) * 2654435761u ^ vb * 40503u) >> 20 & (ENT_TAB - 1);
    for (unsigned n = 0; n < ENT_TAB; n++, h = (h + 1) & (ENT_TAB - 1)) {
        EntKey* k = ent_tab[h];
        if (!k) return NULL;
        if (k->list == (u32)l && k->vb == vb) return k;
    }
    return NULL;
}

u32 ent_segkey(void) { u32 h = 2166136261u; for (unsigned i = 1; i < 16; i++) h = (h ^ nd_ws.segbase[i]) * 16777619u; return h | 1u; }

/* SGEN-1: every static list the walker meets (G_DL arm, no record). Lists in the display contexts are dynamic by address: walker (DIRECT D10a:
 * the frame stream holds no list). 1: drawn, 0 not drawn, -1 ring path wanted (the caller skips dlc_try). */
int ent_gdl(const Gfx* l) {
    if (!dlc_on || ((u32)l & 7u) || (u32)l - (u32)D_80164000 < 2u * sizeof(DisplayContext)) { gdl_c.skip++; return 0; }
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    if (ent_gen_seen != pm_dlc_gen) ent_reset();
    gdl_c.lists++;
    EntKey* k = ent_find(l, 0);
    if (!k) {
        if (ent_nent >= ENT_TAB * 3 / 4) { gdl_c.ref++; return 0; }
        k = (EntKey*)calloc(1, sizeof *k);
        if (!k) return 0;
        unsigned h = (((u32)l >> 3) * 2654435761u) >> 20 & (ENT_TAB - 1);
        while (ent_tab[h]) h = (h + 1) & (ENT_TAB - 1);
        k->list = (u32)l; k->vb = 0; k->id = ent_nent; ent_tab[h] = k; ent_nent++;
        ent_compile(k, l);
    }
    if (k->useseg) { /* the vertices sit behind segment addresses: the entry of these segment bases */
        u32 vb = ent_segkey();
        k = ent_find(l, vb);
        if (!k) {
            if (ent_nent >= ENT_TAB * 3 / 4) { gdl_c.ref++; return 0; }
            k = ent_lookup(l, vb);
            if (!k) { gdl_c.ref++; return 0; }
        }
    }
    { const u32 f = pm_frame_count(); if (k->cf != f) { k->pcalls = k->cf + 1u == f ? k->calls : 0; k->calls = 0; k->cf = f; } if (k->calls < 0xffffu) k->calls++;
      if (k->pcalls >= 4u) return -1; } /* PBATCH1: a particle list (4+ calls last frame): the ring path batches 8 matrices per draw */
    if (k->st == ENT_DYN) { gdl_c.dyn++; return 0; }
    if (k->st == ENT_REF && k->why != 6) { gdl_c.ref++; return 0; }
    int r = ent_run(l, k, 0);
    if (r) gdl_c.nat++; else if (k->st == ENT_DYN) gdl_c.dyn++; else gdl_c.ref++;
    return r;
}

int ent_draw(const Gfx* rec) {
    const Gfx* l = (const Gfx*)ptr_of(rec[0].words.w1);
    const Gfx* nx = rec + G_PM_ENT_WORDS;
    if (!l || !dlc_on || !is_host((u32)l) || ((u32)l & 7u) || nx->words.w0 != ND_OP_CALL_W0 || nx->words.w1 != rec[0].words.w1) return 0;
    if (dlc_seen_gen != pm_dlc_gen) { dlc_reset(); dlc_seen_gen = pm_dlc_gen; }
    if (ent_gen_seen != pm_dlc_gen) ent_reset();
    ent_c.draws++;
    EntKey* k = ent_lookup(l, rec[1].words.w1);
    return ent_run(l, k, rec[1].words.w0 | (rec[2].words.w0 << 8));
}

unsigned nd_ent_in, nd_ent_cfull, nd_ent_rfull, nd_ent_gbi;
unsigned nd_ent_cfull_all(void) { return nd_ent_cfull; }
unsigned nd_gdl_lists(void) { return gdl_c.lists; }
#define ENT_GBI(l_, n_) (nd_ent_gbi++, nd_ops_run((l_), (n_) >> 3, 1)) /* PERF2: the pieces of the flattened list keep the walker's sync state (D10b: the op executor) */
static void ent_gap(NdSDef* d, const Gfx* l, u32 n) { /* S6E: a gap as a compiled range, with nd_ops_run's list start and end; only when every value unit precedes the statics (the op order) */
    if (d && !d->st) d->st = nd_scomp_analyse(d) ? 1u : 2u;
    if (d && d->st == 1u && d->nv == d->pre) {
        gv_epoch++; dlc_on = 1; pm_walk_side++; const int ok = nd_scomp_exec_own(d); pm_walk_side--;
        if (ok) { nd_ent_sc++; gv_streak_flush(); tex_streak_flush(); return; } }
    ENT_GBI((void*)l, n * 8u);
}
/* HC8 step 7: the recompile of a changed entity (content hash), the interpreter run of one segment (the fallback loop) are cold: out of line. */
static void __attribute__((noinline, cold)) ent_run_recompile(EntKey* k, const Gfx* l) {
    if (++k->nrec > 3) { k->st = ENT_DYN; ent_c.dyn++; }
    else { u8 nr = k->nrec; ent_compile(k, l); k->nrec = nr; }
}
static void __attribute__((noinline, cold)) ent_run_seg_interp(const EntKey* k, const EntSeg* e) {
    const Gfx* fw = k->fw;
    ent_c.segi++; /* the old words: the vertex loads before the segment, then its own ops */
    for (u32 i = 0; i < e->op0; i++) if (ent_uid(fw[i].words.w0, fw[i].words.w1) == NDO_VTX) ENT_GBI((void*)(fw + i), 8u);
    if (e->op1 > e->op0) ENT_GBI((void*)(fw + e->op0), (e->op1 - e->op0) * 8u);
}
int ent_run(const Gfx* l, EntKey* k, u32 segid) {
    const unsigned f = pm_frame_count();
    if (!k) { ent_c.why[1]++; ent_c.fb++; return 0; }
    if (k->st == ENT_OK && k->fr != f) { /* content, once per frame */
        k->fr = f;
        if (ent_hash(l, k->vp, k->vlen) != k->hash) ent_run_recompile(k, l);
    } else if (k->st == ENT_REF && k->why == 6 && f >= k->retry) { ent_compile(k, l); }
    k->ndraw++; k->segid = segid;
    if (k->st != ENT_OK) { ent_c.fb++; if (k->st == ENT_REF) ent_c.why[k->why < 12 ? k->why : 0]++; return 0; }
    if (entchk_on()) { ent_c.nat++; return 0; }
    nd_ent_in = 1;
    for (unsigned s = 0; s < k->nseg; s++) {
        const EntSeg* e = &k->seg[s];
        const Gfx* fw = k->fw;
        if (e->op0 > e->a0) { if (e->pcn) nd_ent_sc += (unsigned)nd_ent_pieces(k->pc + e->pc0, e->pcn); else ent_gap(e->sd, fw + e->a0, e->op0 - e->a0); }
        if (!e->ntri) continue; /* a state-only or vertex-only segment: the loads replay when a later segment needs them */
        if (e->ntri && ent_seg_native(k, e)) { ent_c.segn++; continue; }
        ent_run_seg_interp(k, e);
    }
    { const EntSeg* e = &k->seg[k->nseg - 1]; if (k->nops > e->op1) { if (k->tpcn) nd_ent_sc += (unsigned)nd_ent_pieces(k->pc + k->tpc0, k->tpcn); else ent_gap(k->tail, k->fw + e->op1, k->nops - e->op1); } }
    tri_since_vtx = 1; memset(vert_live, 0, sizeof vert_live); dlc_dirty = 1; /* as the skipped groups of dlc_try */
    nd_ent_in = 0;
    ent_c.nat++;
    return 1;
}

/* G_PM_ENT in the walk (nd_walk.c): returns 1 when the entity draw ran from the pools (the walk skips the record's G_DL), else 0. */
int nd_ent_case(const Gfx* rec, u32 w0, u32 w1) {
    if (w0 & G_PM_ENT_END) { pm_gbi_ent_texchk((const uint32_t*)(const void*)(uintptr_t)w1); pm_gbi_ent_segchk((const uint32_t*)(const void*)(uintptr_t)w1); return 0; }
    if (w0 & G_PM_ENT_DRAW) {
        pm_tex_tagi = 4;
        if (ent_draw(rec)) { if (pm_ph_cen) pk_nat[pmx_cur]++; return 1; }
        ent_skipgen = 1;
    }
    return 0;
}

/* MAGI fspk (debug): lifetime entity key compiles, static G_DL lists met, lists drawn from the pools, segments drawn by the walker */
void nd_ent_stat(unsigned o[4]) { o[0] = ent_c.comp; o[1] = gdl_c.lists; o[2] = gdl_c.nat; o[3] = ent_c.segi; }
