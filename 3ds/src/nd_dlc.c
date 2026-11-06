/* nd_dlc.c: static display list compiler (plan_native_slices S3-1). Moved from the DLC compile code (pm_gbi.c dlc_compile) (the group cut, the slot rules, the boxes) and dlc_static_build
 * (the Vtx copies with flag 0 and the triangle indices). Pure C on host-order word pairs: the host test builds it. The batch merge of dlc_batch_compile is not here:
 * it belongs to the sort and merge of nd_core (S3-5). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"
#include "nd_ops.h"

typedef struct { NdGrp g; } Tmp;
static NdGrp tg[ND_DLC_MAXGRP];
static uint32_t tgop[ND_DLC_MAXGOP][2];

void nd_dlc_free(NdObj* o) {
    free(o->grp); free(o->gop); free(o->gat);
    if (!o->pooled) { free(o->vtx); free(o->idx); }
    memset(o, 0, sizeof *o);
}

int nd_dlc_compile(NdObj* o, const uint32_t* l, const NdDlcEnv* env) {
    unsigned ng = 0, ngop = 0, i, tri_total = 0, vtx_total = 0;
    uint32_t cam = 0xFFFFFFFFu, com = 0;
    unsigned char live[64];
    int tsv = -1, in = 0;
    NdGrp* g = NULL;
    memset(o, 0, sizeof *o);
    memset(live, 2, sizeof live);
    /* pass 1: cut into groups (dlc_compile) */
    for (i = 0; i < ND_DLC_MAXOPS; i++) {
        uint32_t w0 = l[2 * i], w1 = l[2 * i + 1];
        unsigned op = w0 >> 24;
        if (op == OP_ENDDL) break;
        switch (op) {
        case OP_VTX: {
            unsigned n = (w0 >> 12) & 0xFF, v0n = (w0 >> 1) & 0x7F, v0 = v0n >= n ? v0n - n : 0;
            const int16_t* src = (const int16_t*)env->vtx(w1, n);
            if (!src || n == 0 || n > 64 || v0 >= 64) return NDR_VTX;
            if (v0 + n > 64) n = 64 - v0;
            if (!in || (v0 == 0 && tsv == 1)) {
                if (g) { g->op1 = i; g->tsv_exit = (uint8_t)(tsv == 1); }
                if (ng >= ND_DLC_MAXGRP) return NDR_LONG;
                g = &tg[ng++];
                memset(g, 0, sizeof *g);
                g->op0 = i; g->am = cam; g->om = com; g->gbase = ngop;
                for (int q = 0; q < 3; q++) { g->lo[q] = 32767; g->hi[q] = -32768; }
                if (in) memset(live, 0, sizeof live);
                in = 1;
            }
            tsv = 0;
            for (unsigned k = 0; k < n; k++) {
                for (int q = 0; q < 3; q++) { int16_t v = src[8 * k + q]; if (v < g->lo[q]) g->lo[q] = v; if (v > g->hi[q]) g->hi[q] = v; }
                live[v0 + k] = 1;
            }
            g->nvtx = (uint16_t)(g->nvtx + n);
            break;
        }
        case OP_TRI1: case OP_TRI2: {
            if (!in) return NDR_SLOT;
            for (int t = 0; t < (op == OP_TRI2 ? 2 : 1); t++) {
                uint32_t w = t ? w1 : w0;
                int ix[3] = { (int)((w >> 16) & 0xFF) / 2, (int)((w >> 8) & 0xFF) / 2, (int)(w & 0xFF) / 2 };
                for (int k = 0; k < 3; k++) if (ix[k] <= 63 && live[ix[k]] == 2) return NDR_SLOT; /* a slot that the list did not load */
                g->ntri++;
            }
            tsv = 1;
            break;
        }
        case OP_GEOMETRYMODE: {
            uint32_t am = w0 & 0xFFFFFFu;
            cam &= am; com = (com & am) | w1;
            if (in) { if (ngop >= ND_DLC_MAXGOP) return NDR_LONG; tgop[ngop][0] = am; tgop[ngop][1] = w1; ngop++; g->ng++; }
            break;
        }
        case OP_NOOP: case OP_FULLSYNC: case OP_TILESYNC: case OP_PIPESYNC: case OP_LOADSYNC:
            break;
        default:
            return NDR_OP;
        }
    }
    if (i >= ND_DLC_MAXOPS) return NDR_LONG;
    if (g) { g->op1 = i; g->tsv_exit = (uint8_t)(tsv == 1); }
    for (unsigned k = 0; k < ng; k++) { tri_total += tg[k].ntri; vtx_total += tg[k].nvtx; }
    if (vtx_total > 65535u) return NDR_LONG;
    o->ngrp = (uint16_t)ng; o->nops = i + 1; o->exit_am = cam; o->exit_om = com; o->ngop = ngop;
    o->grp = ng ? (NdGrp*)malloc(ng * sizeof(NdGrp)) : NULL;
    o->gop = ngop ? (uint32_t(*)[2])malloc(ngop * 8) : NULL;
    o->gat = ngop ? (uint16_t*)malloc(ngop * 2) : NULL;
    if (env->valloc && env->ialloc) { /* the pools: the static vertex area and the static index area */
        o->pooled = 1;
        o->vtx = vtx_total ? env->valloc(vtx_total, &o->vfirst) : NULL;
        o->idx = tri_total ? env->ialloc(tri_total * 3u) : NULL;
    } else {
        o->vtx = vtx_total ? (uint8_t*)malloc(vtx_total * 16u) : NULL;
        o->idx = tri_total ? (uint16_t*)malloc(tri_total * 6u) : NULL;
    }
    if ((ng && !o->grp) || (ngop && (!o->gop || !o->gat)) || (vtx_total && !o->vtx) || (tri_total && !o->idx)) {
        int full = o->pooled; /* a pool refusal is NDR_FULL, a malloc refusal NDR_LONG */
        o->pooled = 0; free(o->grp); free(o->gop); free(o->gat); /* the pooled arrays are not freed, a malloc pair is */
        if (!full) { free(o->vtx); free(o->idx); }
        memset(o, 0, sizeof *o);
        return full ? NDR_FULL : NDR_LONG;
    }
    if (ng) memcpy(o->grp, tg, ng * sizeof(NdGrp));
    if (ngop) memcpy(o->gop, tgop, ngop * 8);
    /* pass 2: the vertex copies and the triangle indices (dlc_static_build) */
    unsigned nv = 0, ni = 0, go = 0;
    for (unsigned k = 0; k < ng; k++) {
        NdGrp* q = &o->grp[k];
        unsigned short smap[64];
        unsigned gnv = 0, nt = 0;
        memset(live, 0, sizeof live);
        q->vbase = o->vfirst + nv; q->ibase = ni; q->gbase = go;
        for (uint32_t j = q->op0; j < q->op1; j++) {
            uint32_t w0 = l[2 * j], w1 = l[2 * j + 1];
            switch (w0 >> 24) {
            case OP_VTX: {
                unsigned n = (w0 >> 12) & 0xFF, v0n = (w0 >> 1) & 0x7F, v0 = v0n >= n ? v0n - n : 0;
                const uint8_t* src = (const uint8_t*)env->vtx(w1, n);
                if (v0 + n > 64) n = 64 - v0;
                memcpy(o->vtx + 16u * (nv + gnv), src, 16u * n);
                for (unsigned t = 0; t < n; t++) {
                    memset(o->vtx + 16u * (nv + gnv + t) + 6, 0, 2); /* flag word 0: palette index 0 */
                    smap[v0 + t] = (unsigned short)(o->vfirst + nv + gnv + t);
                    live[v0 + t] = 1;
                }
                gnv += n;
                break;
            }
            case OP_TRI1: case OP_TRI2:
                for (int t = 0; t < ((w0 >> 24) == OP_TRI2 ? 2 : 1); t++) {
                    uint32_t w = t ? w1 : w0;
                    int ix[3] = { (int)((w >> 16) & 0xFF) / 2, (int)((w >> 8) & 0xFF) / 2, (int)(w & 0xFF) / 2 };
                    if (ix[0] > 63 || ix[1] > 63 || ix[2] > 63 || !live[ix[0]] || !live[ix[1]] || !live[ix[2]]) continue;
                    for (int c = 0; c < 3; c++) o->idx[ni + 3 * nt + c] = smap[ix[c]];
                    nt++;
                }
                break;
            case OP_GEOMETRYMODE:
                o->gat[go++] = (uint16_t)nt;
                break;
            default:
                break;
            }
        }
        q->ntri = (uint16_t)nt;
        nv += gnv; ni += 3 * nt;
    }
    o->nvtx = nv; o->nidx = ni;
    return NDR_OK;
}

/* S6-2a: the tile 0 state at the end of a display list, the walker rules of its texture ops (pm_gbi.c op_settimg, op_settile, op_loadblock, op_loadtlut,
 * op_settilesize) for the form an entity model uses. Any other tile, LOADTILE, a window that is not the whole image, or a CI bank without its LOADTLUT is a fallback. */
static int tex_decode_n(const uint32_t* l, unsigned nops, NdTexDec* o) {
    unsigned i, nbind = 0, ntexop = 0;
    int code = NDTX_NONE, have_timg = 0, have_set0 = 0, have_set7 = 0, have_lb = 0, have_pal = 0, have_ld = 0;
    const void* timg = NULL;
    unsigned timg_stride = 0, tfmt = 0, tsiz = 0, tmem0 = 0, tmem7 = 0, lb_tm = 0, pal0 = 0, cms = 0, cmt = 0, masks = 0, maskt = 0;
    unsigned uls = 0, ult = 0, lrs = 0, lrt = 0, tlut_n = 0, ld_tm = 0, ld_w = 0, ld_h = 0;
    const void* tlut = NULL;
    memset(o, 0, sizeof *o);
    for (i = 0; i < nops; i++) {
        uint32_t w0 = l[2 * i], w1 = l[2 * i + 1];
        unsigned op = w0 >> 24, tile = (w1 >> 24) & 7u;
        if (op == OP_ENDDL) break;
        switch (op) {
        case OP_SETTIMG:
            ntexop++;
            have_timg = 1; timg = (const void*)(uintptr_t)w1; timg_stride = (w0 & 0xfffu) + 1u; have_ld = 0; /* the walker: SETTIMG clears ld_valid */
            if (w1 == 0 && code == NDTX_NONE) code = NDTX_TIMG;
            break;
        case OP_SETTILE:
            ntexop++;
            if (tile == 0) {
                tfmt = (w0 >> 21) & 7u; tsiz = (w0 >> 19) & 3u; pal0 = (w1 >> 20) & 15u; tmem0 = w0 & 0x1ffu;
                cms = (w1 >> 8) & 3u; cmt = (w1 >> 18) & 3u; masks = (w1 >> 4) & 15u; maskt = (w1 >> 14) & 15u;
                have_set0 = 1;
            } else if (tile == 7) {
                tmem7 = w0 & 0x1ffu; have_set7 = 1;
            } else if (code == NDTX_NONE) code = NDTX_TILE, o->bad_op = (op << 8) | tile;
            break;
        case OP_SETTILESIZE:
            ntexop++;
            if (tile != 0) { if (code == NDTX_NONE) code = NDTX_TILE, o->bad_op = (op << 8) | tile; break; }
            uls = (w0 >> 12) & 0xfffu; ult = w0 & 0xfffu; lrs = (w1 >> 12) & 0xfffu; lrt = w1 & 0xfffu;
            if (!have_timg || !have_set0) { if (code == NDTX_NONE) code = NDTX_TIMG; break; }
            if (nbind++ > 0 && code == NDTX_NONE) code = NDTX_MULTI;
            break;
        case OP_LOADBLOCK:
            ntexop++;
            /* gDPLoadTextureBlock loads on the load tile 7 (its TMEM address is the tile 0 one). The load clears the stale LoadTile records of its range. */
            if (tile == 7 && have_set7) { lb_tm = tmem7; have_lb = 1; }
            else if (tile == 0 && have_set0) { lb_tm = tmem0; have_lb = 1; }
            else if (code == NDTX_NONE) code = NDTX_TILE, o->bad_op = (op << 8) | tile;
            break;
        case OP_LOADTLUT:
            ntexop++;
            if (tile != 7) { if (code == NDTX_NONE) code = NDTX_TILE, o->bad_op = (op << 8) | tile; break; }
            if (!have_timg || !have_set7) { if (code == NDTX_NONE) code = NDTX_PAL; break; }
            tlut = timg; tlut_n = ((w1 >> 14) & 0x3ffu) + 1u;
            /* the palette bank of tile 0 is the 16 entry block at TMEM 256 + 16 * bank (the walker: slot = (tmem - 256) >> 4) */
            if (tmem7 >= 256u && ((tmem7 - 256u) >> 4) == pal0) have_pal = 1;
            break;
        case OP_LOADTILE: /* the walker: ld_valid, the load window (ld_x, ld_y, ld_w, ld_h) and an ldrec record of (TMEM of the load tile, image) */
            ntexop++;
            if (tile != 7 || !have_set7 || !have_timg) { if (code == NDTX_NONE) code = NDTX_TILE, o->bad_op = (op << 8) | tile; break; }
            if (((w0 >> 12) & 0xfffu) >= 4u || (w0 & 0xfffu) >= 4u) { if (code == NDTX_NONE) code = NDTX_WIN; break; } /* a load window origin is not 0 */
            have_ld = 1; ld_tm = tmem7;
            ld_w = (((w1 >> 12) & 0xfffu) >> 2) + 1u; ld_h = ((w1 & 0xfffu) >> 2) + 1u;
            break;
        default:
            break;
        }
    }
    if (code == NDTX_NONE && ntexop == 0) return NDTX_NONE;
    if (code == NDTX_NONE && nbind == 0) return NDTX_NOBIND;
    if (code == NDTX_NONE || code == NDTX_MULTI) {
        unsigned w = ((lrs - uls) >> 2) + 1u, h = ((lrt - ult) >> 2) + 1u; /* the walker size, unsigned here: a negative window is caught below */
        unsigned stride;
        if (lrs < uls || lrt < ult || uls != 0 || ult != 0 || h <= 1u) { if (code == NDTX_NONE) code = NDTX_WIN; }
        else if (!((have_lb && lb_tm == tmem0) || (have_ld && ld_tm == tmem0))) { if (code == NDTX_NONE) code = NDTX_NOLB; } /* the load clears or overwrites the stale records of the tile 0 TMEM */
        else if (have_ld && ((tsiz == 0u ? 2u * ld_w : ld_w) != w || ld_h != h)) { if (code == NDTX_NONE) code = NDTX_WIN; o->w = w; o->h = h; o->stride = ld_w; o->pal_n = ld_h; } /* a 4 bit LoadTile counts bytes (the walker: ld_w * 2 for siz 0) */ /* a load window narrower than the tile makes the walker wrap or offset */
        else if (tfmt == 2u && !have_pal) { if (code == NDTX_NONE) code = NDTX_PAL; }
        else {
            stride = timg_stride > w ? timg_stride : w;
            if (masks && (1u << masks) < w && timg_stride > 0 && timg_stride < w) stride = timg_stride; /* the walker stride rule for a mask narrower than the window */
            if (have_ld && tsiz == 0u) stride = 2u * timg_stride; /* 4 bit with a LoadTile (ld_valid): the walker sets stride = 2 * timg stride */
            o->img = timg; o->fmt = tfmt; o->siz = tsiz; o->w = w; o->h = h; o->stride = stride;
            o->cms = cms; o->cmt = cmt; o->masks = masks; o->maskt = maskt;
            if (tfmt == 2u) { o->pal = tlut; o->pal_n = tlut_n; }
            if (code == NDTX_NONE && nbind == 1) code = NDTX_OK;
            if (code == NDTX_NONE) code = NDTX_MULTI;
            if (nbind > 1) code = NDTX_MULTI;
        }
    }
    if (code == NDTX_NONE) code = NDTX_NOBIND;
    return code;
}

int nd_dlc_tex_decode(const uint32_t* l, NdTexDec* o) { return tex_decode_n(l, ND_DLC_MAXOPS, o); }

/* S6-2c-1: the entity list segments (nd.h). */
/* S6-2c-2: G_PM_TEXI (gbi_custom.h, 0x0B) is a header whose n words run in place (pm_gbi.c tb_inline). The entity lists carry it in front of their texture loads (entcheck census: every refused list had it). Neutral: the linear scan sees the words after it. */
#define OP_PM_TEXI 0x0B
#define OP_PM_RM 0x09 /* gbi_custom.h G_PM_RM: a render mode list run in place */
static int ent_known(unsigned op) {
    switch (op) {
    case OP_VTX: case OP_TRI1: case OP_TRI2: case OP_PM_TEXI:
    case OP_NOOP: case OP_PIPESYNC: case OP_LOADSYNC: case OP_TILESYNC: case OP_FULLSYNC:
    case OP_GEOMETRYMODE: case OP_SETOTHERMODE_H: case OP_SETOTHERMODE_L: case OP_SETCOMBINE: case OP_TEXTURE:
    case OP_SETPRIMCOLOR: case OP_SETENVCOLOR:
    case OP_SETTIMG: case OP_SETTILE: case OP_SETTILESIZE: case OP_LOADBLOCK: case OP_LOADTLUT: case OP_LOADTILE:
    /* SGEN-1: ops the walker runs in place between the native triangle ranges (matrix, lights and segments, scissor, colours, images, the render mode list op): the live walker state is the state of the next range */
    case OP_MTX: case OP_POPMTX: case OP_MOVEMEM: case OP_MOVEWORD: case OP_SETSCISSOR: case OP_SETFOGCOLOR: case OP_SETBLENDCOLOR: case OP_SETFILLCOLOR:
    case OP_SETPRIMDEPTH: case OP_RDPSETOTHERMODE: case OP_SETCIMG: case OP_SETZIMG: case OP_SETKEYGB: case OP_SETKEYR: case OP_SETCONVERT: case OP_PM_RM:
        return 1;
    default:
        return 0;
    }
}

static void delta_clear(NdEntDelta* d) { memset(d, 0, sizeof *d); d->gand = 0xFFFFFFFFu; }

static void delta_apply(NdEntDelta* d, uint32_t w0, uint32_t w1) {
    unsigned op = w0 >> 24;
    switch (op) {
    case OP_GEOMETRYMODE: {
        uint32_t a = w0 & 0xFFFFFFu;
        d->gor = (d->gor & a) | w1; d->gand &= a; d->has_geom = 1;
        break;
    }
    case OP_SETOTHERMODE_H: case OP_SETOTHERMODE_L: {
        unsigned len = (w0 & 0xFFu) + 1u, field = (w0 >> 8) & 0xFFu, sft = 32u - field - len;
        uint32_t mask = len >= 32 ? 0xFFFFFFFFu : ((1u << len) - 1u) << sft;
        if (op == OP_SETOTHERMODE_H) { d->wh |= mask; d->vh = (d->vh & ~mask) | (w1 & mask); }
        else { d->wl |= mask; d->vl = (d->vl & ~mask) | (w1 & mask); }
        d->has_mode = 1;
        break;
    }
    case OP_SETCOMBINE: d->has_comb = 1; d->cw0 = w0; d->cw1 = w1; break;
    case OP_TEXTURE: d->has_tex = 1; d->tw0 = w0; d->tw1 = w1; break;
    case OP_SETPRIMCOLOR: d->has_prim = 1; d->prim_lod = w0 & 0xFFFFu; d->prim = w1; break;
    case OP_SETENVCOLOR: d->has_env = 1; d->env = w1; break;
    default: d->n_tex++; break; /* the texture state ops: their state is the tex desc of the segment */
    }
}

int nd_ent_segments(const uint32_t* l, NdEntSeg* seg, unsigned maxseg, unsigned* nseg, uint32_t* bad) {
    unsigned i, ns = 0, tri = 0, pend_n = 0, pend0 = 0;
    unsigned char live[64];
    NdEntDelta pend;
    *nseg = 0;
    if (bad) *bad = 0;
    memset(live, 0, sizeof live);
    delta_clear(&pend);
    for (i = 0; i < ND_DLC_MAXOPS; i++) {
        uint32_t w0 = l[2 * i], w1 = l[2 * i + 1];
        unsigned op = w0 >> 24;
        if (op == OP_ENDDL) break;
        if (!ent_known(op)) { if (bad) *bad = op; return NDR_OP; }
        if (op == OP_VTX || op == OP_TRI1 || op == OP_TRI2) {
            NdEntSeg* s;
            if (ns == 0 || pend_n > 0) { /* a geometry op after state ops starts a segment */
                if (ns >= maxseg) return NDR_LONG;
                s = &seg[ns];
                s->d = pend; s->sop0 = pend_n ? pend0 : i; s->op0 = i; s->op1 = i; s->tri0 = tri; s->ntri = 0;
                if (ns == 0 || pend.n_tex) tex_decode_n(l, i, &s->tex); /* the tile state of the list before op0 */
                else s->tex = seg[ns - 1].tex;
                ns++; pend_n = 0; delta_clear(&pend);
            }
            s = &seg[ns - 1];
            s->op1 = i + 1;
            if (op == OP_VTX) {
                unsigned n = (w0 >> 12) & 0xFFu, e = (w0 >> 1) & 0x7Fu, v0 = e >= n ? e - n : 0;
                if (n == 0 || n > 64 || v0 >= 64) return NDR_VTX;
                if (v0 + n > 64) n = 64 - v0;
                for (unsigned k = 0; k < n; k++) live[v0 + k] = 1;
            } else {
                for (int t = 0; t < (op == OP_TRI2 ? 2 : 1); t++) {
                    uint32_t w = t ? w1 : w0;
                    int ix[3] = { (int)((w >> 16) & 0xFFu) / 2, (int)((w >> 8) & 0xFFu) / 2, (int)(w & 0xFFu) / 2 };
                    for (int k = 0; k < 3; k++) if (ix[k] > 63 || !live[ix[k]]) return NDR_SLOT;
                    s->ntri++; tri++;
                }
            }
        } else if (op == OP_NOOP || op == OP_PIPESYNC || op == OP_LOADSYNC || op == OP_TILESYNC || op == OP_FULLSYNC || op == OP_PM_TEXI) {
            /* neutral: no segment, no delta */
        } else {
            if (pend_n == 0) pend0 = i;
            delta_apply(&pend, w0, w1);
            pend_n++;
        }
    }
    if (i >= ND_DLC_MAXOPS) return NDR_LONG;
    if (pend_n > 0) { /* state ops after the last geometry op: an empty segment keeps the exit state */
        if (ns >= maxseg) return NDR_LONG;
        NdEntSeg* s = &seg[ns];
        s->d = pend; s->sop0 = pend0; s->op0 = i; s->op1 = i; s->tri0 = tri; s->ntri = 0;
        if (ns == 0 || pend.n_tex) tex_decode_n(l, i, &s->tex);
        else s->tex = seg[ns - 1].tex;
        ns++;
    }
    *nseg = ns;
    return NDR_OK;
}

/* S6-2c-2: see nd.h. The state ops of an entity list stay out, the static compile takes the vertex loads and triangles in list order. */
unsigned nd_ent_strip(const uint32_t* l, uint32_t* out) {
    unsigned i, n = 0;
    for (i = 0; i < ND_DLC_MAXOPS; i++) {
        uint32_t w0 = l[2 * i], w1 = l[2 * i + 1];
        unsigned op = w0 >> 24;
        if (op == OP_ENDDL) break;
        if (op == OP_VTX || op == OP_TRI1 || op == OP_TRI2) { out[2 * n] = w0; out[2 * n + 1] = w1; n++; }
    }
    out[2 * n] = (uint32_t)OP_ENDDL << 24; out[2 * n + 1] = 0;
    return i;
}
