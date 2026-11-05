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
