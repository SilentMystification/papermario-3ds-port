/* PBUF: the effect part buffer units (nd_at.h NDVAL_PRIM, NDXF_PUSH, NDXF_POP, NDCTL_CALL, nd_tx.h NDTX_TSZ into a local uint32_t buffer
 * through ND_NR_AT) equal the units the gbi macros write into a local Gfx buffer (nd_gbi.h ND_GX, nd_gx_w local packet: nd_rec.c:455).
 * The cases are the 7 nd_parts_add sites of src/effects with edge values. -DPBUF_SEED changes one argument (the seeded fault). */
#include "common.h"
#include "nd_at.h"
#include "nd_tx.h"
#include <stdio.h>
#include <string.h>
void nd_gx_w(Gfx* g, uint32_t w0, uint32_t w1) { g->words.w0 = w0; g->words.w1 = w1; } /* nd_rec.c:455: a local packet gets the unit */
static int bad, n;
static void cmp(const char* what, const Gfx* a, const uint32_t* b, unsigned m) {
    n++;
    if (memcmp(a, b, m * 8u) != 0) { bad++; printf("pbuf bad %s\n", what); }
}
static Mtx mt[4];
static Gfx l0[2], l1[2];
int main(void) {
    static const float fa[] = { 0.0f, 1.0f, 127.5f, 255.0f, 254.9f };
    static const s32 iv[] = { 0, 1, 15, 255, -1, -60, 4095, 4096, 0x7c };
    for (unsigned k = 0; k < sizeof(iv) / sizeof(iv[0]); k++) {
        const s32 v = iv[k];
        const float f = fa[k % 5u];
        int i = (int)k;
        { /* effect_3D.c, pink_sparkles.c: prim, push mul, call, pop */
            Gfx g[4]; uint32_t u[8], *nrp = u;
            gDPSetPrimColor(&g[0], 0, 0, 200, v, 255, v); gSPMatrix(&g[1], &mt[k & 3u], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            gSPDisplayList(&g[2], l0); gSPPopMatrix(&g[3], G_MTX_MODELVIEW);
            NDVAL_PRIM(ND_NR_AT, 0, 0, 200, v, 255, v); NDXF_PUSH(ND_NR_AT, &mt[k & 3u], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            NDCTL_CALL(ND_NR_AT, l0); NDXF_POP(ND_NR_AT);
            cmp("prim_push_call_pop", g, u, 4);
        }
        { /* shimmer_burst.c: push, prim with a float alpha, call, pop */
            Gfx g[4]; uint32_t u[8], *nrp = u;
            gSPMatrix(&g[0], &mt[1], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW); gDPSetPrimColor(&g[1], 0, 0, v, v, v, f * 0.5f);
            gSPDisplayList(&g[2], l1); gSPPopMatrix(&g[3], G_MTX_MODELVIEW);
            NDXF_PUSH(ND_NR_AT, &mt[1], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW); NDVAL_PRIM(ND_NR_AT, 0, 0, v, v, v, f * 0.5f);
            NDCTL_CALL(ND_NR_AT, l1); NDXF_POP(ND_NR_AT);
            cmp("push_prim_call_pop", g, u, 4);
        }
        { /* embers.c: push, 2 tile sizes (signed fields), call, pop */
            Gfx g[5]; uint32_t u[10], *nrp = u;
            gSPMatrix(&g[0], &mt[2], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            gDPSetTileSize(&g[1], G_TX_RENDERTILE, 0, (i % 4) * 16 * 4, 15 * 4, ((i % 4) * 16 + 15) * 4);
            gDPSetTileSize(&g[2], 1, (s32) f * 4, v * 4, ((s32) f + 15) * 4, (v + 15) * 4);
            gSPDisplayList(&g[3], l0); gSPPopMatrix(&g[4], G_MTX_MODELVIEW);
            NDXF_PUSH(ND_NR_AT, &mt[2], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            NDTX_TSZ(ND_NR_AT, G_TX_RENDERTILE, 0, (i % 4) * 16 * 4, 15 * 4, ((i % 4) * 16 + 15) * 4);
#ifdef PBUF_SEED
            NDTX_TSZ(ND_NR_AT, 1, (s32) f * 4, v * 4, ((s32) f + 15) * 4, (v + 16) * 4);
#else
            NDTX_TSZ(ND_NR_AT, 1, (s32) f * 4, v * 4, ((s32) f + 15) * 4, (v + 15) * 4);
#endif
            NDCTL_CALL(ND_NR_AT, l0); NDXF_POP(ND_NR_AT);
            cmp("push_tsz_tsz_call_pop", g, u, 5);
        }
        { /* snaking_static.c: prim, push, 2 tile sizes, call, pop */
            Gfx g[6]; uint32_t u[12], *nrp = u;
            gDPSetPrimColor(&g[0], 0, 0, v, 1, 2, (v * v) / 255); gSPMatrix(&g[1], &mt[3], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            gDPSetTileSize(&g[2], G_TX_RENDERTILE, v * 64, 0, (v * 64) | 0x3C, 0x007C); gDPSetTileSize(&g[3], 1, (v * 12), 0, ((v * 3) + 0x1F) * 4, 0x007C);
            gSPDisplayList(&g[4], l1); gSPPopMatrix(&g[5], G_MTX_MODELVIEW);
            NDVAL_PRIM(ND_NR_AT, 0, 0, v, 1, 2, (v * v) / 255); NDXF_PUSH(ND_NR_AT, &mt[3], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            NDTX_TSZ(ND_NR_AT, G_TX_RENDERTILE, v * 64, 0, (v * 64) | 0x3C, 0x007C); NDTX_TSZ(ND_NR_AT, 1, (v * 12), 0, ((v * 3) + 0x1F) * 4, 0x007C);
            NDCTL_CALL(ND_NR_AT, l1); NDXF_POP(ND_NR_AT);
            cmp("prim_push_tsz_tsz_call_pop", g, u, 6);
        }
        { /* snowfall.c: push load, nopush mul, call, pop */
            Gfx g[4]; uint32_t u[8], *nrp = u;
            gSPMatrix(&g[0], &mt[0], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW); gSPMatrix(&g[1], &mt[k & 3u], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            gSPDisplayList(&g[2], l0); gSPPopMatrix(&g[3], G_MTX_MODELVIEW);
            NDXF_PUSH(ND_NR_AT, &mt[0], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW); NDXF_PUSH(ND_NR_AT, &mt[k & 3u], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            NDCTL_CALL(ND_NR_AT, l0); NDXF_POP(ND_NR_AT);
            cmp("push_push_call_pop", g, u, 4);
        }
        { /* water_block.c: push, call, pop */
            Gfx g[3]; uint32_t u[6], *nrp = u;
            gSPMatrix(&g[0], &mt[k & 3u], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW); gSPDisplayList(&g[1], l1); gSPPopMatrix(&g[2], G_MTX_MODELVIEW);
            NDXF_PUSH(ND_NR_AT, &mt[k & 3u], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW); NDCTL_CALL(ND_NR_AT, l1); NDXF_POP(ND_NR_AT);
            cmp("push_call_pop", g, u, 3);
        }
    }
    printf("pbuf %d cases bad %d\n", n, bad);
    return bad != 0;
}
