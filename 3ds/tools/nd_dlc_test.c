/* nd_dlc_test.c: host test of nd_dlc.c (plan_native_slices S3-1, gate G1) on map shape files (big endian, loaded at SHAPE_BASE 0x80210000).
 * Every G_DL target list of the shape is compiled. For each compiled object the invariants of the old compiler must hold:
 *   - every group box equals the box of the Vtx copies of the group (the copies hold the object coordinates the box was made from)
 *   - every index lies inside the vertex range of its group, every copy has flag word 0
 *   - the vertex, triangle and geometry op counts add up, the geometry op triangle index never exceeds the group triangle count
 *   - the triangle count of a group equals a naive count: triangles whose three slots hold a vertex that the group loaded
 * The refusals are counted by reason. Usage: nd_dlc_test <shape.bin> ... */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"
#include "nd_ops.h"

#define SHAPE_BASE 0x80210000u
static uint8_t* raw;
static size_t rawlen;
static uint32_t be32(const uint8_t* p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }

static uint8_t vcopy[64 * 16];
static const void* env_vtx(uint32_t w1, unsigned n) {
    if (w1 < SHAPE_BASE || (size_t)(w1 - SHAPE_BASE) + 16u * n > rawlen) return NULL;
    const uint8_t* s = raw + (w1 - SHAPE_BASE);
    for (unsigned i = 0; i < n; i++) {
        const uint8_t* v = s + 16 * i;
        uint8_t* d = vcopy + 16 * i;
        for (int k = 0; k < 6; k++) { d[2 * k] = v[2 * k + 1]; d[2 * k + 1] = v[2 * k]; } /* ob[3], flag, tc[2]: s16 big to host */
        memcpy(d + 12, v + 12, 4);
    }
    return vcopy;
}

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; if (fails < 20) { printf("FAIL: " __VA_ARGS__); printf("\n"); } } } while (0)

static unsigned n_ok, n_ref[5], n_grp, n_tri, n_vtx;

static void check_obj(const NdObj* o, uint32_t addr, const uint32_t* l) {
    unsigned sv = 0, st = 0, sg = 0;
    for (unsigned k = 0; k < o->ngrp; k++) {
        const NdGrp* g = &o->grp[k];
        int16_t lo[3] = { 32767, 32767, 32767 }, hi[3] = { -32768, -32768, -32768 };
        for (unsigned v = 0; v < g->nvtx; v++) {
            const int16_t* s = (const int16_t*)(o->vtx + 16u * (g->vbase + v));
            for (int q = 0; q < 3; q++) { if (s[q] < lo[q]) lo[q] = s[q]; if (s[q] > hi[q]) hi[q] = s[q]; }
            CHECK(s[3] == 0, "list %08x group %u: vertex flag %d", addr, k, s[3]);
        }
        CHECK(!memcmp(lo, g->lo, 6) && !memcmp(hi, g->hi, 6), "list %08x group %u: box differs from the copies", addr, k);
        for (unsigned t = 0; t < 3 * g->ntri; t++) {
            unsigned ix = o->idx[g->ibase + t];
            CHECK(ix >= g->vbase && ix < g->vbase + g->nvtx, "list %08x group %u: index %u outside %u..%u", addr, k, ix, g->vbase, g->vbase + g->nvtx);
        }
        for (unsigned q = 0; q < g->ng; q++) CHECK(o->gat[g->gbase + q] <= g->ntri, "list %08x group %u: geometry op at triangle %u of %u", addr, k, o->gat[g->gbase + q], g->ntri);
        /* naive triangle count: slots loaded by the group's own G_VTX ops */
        unsigned char live[64]; memset(live, 0, sizeof live);
        unsigned nt = 0;
        for (uint32_t j = g->op0; j < g->op1; j++) {
            uint32_t w0 = l[2 * j], w1 = l[2 * j + 1];
            if ((w0 >> 24) == OP_VTX) { unsigned n = (w0 >> 12) & 0xFF, e = (w0 >> 1) & 0x7F, v0 = e >= n ? e - n : 0; for (unsigned t = 0; t < n && v0 + t < 64; t++) live[v0 + t] = 1; }
            else if ((w0 >> 24) == OP_TRI1 || (w0 >> 24) == OP_TRI2)
                for (int t = 0; t < ((w0 >> 24) == OP_TRI2 ? 2 : 1); t++) {
                    uint32_t w = t ? w1 : w0;
                    unsigned a = ((w >> 16) & 0xFF) / 2, b = ((w >> 8) & 0xFF) / 2, c = (w & 0xFF) / 2;
                    if (a < 64 && b < 64 && c < 64 && live[a] && live[b] && live[c]) nt++;
                }
        }
        CHECK(nt == g->ntri, "list %08x group %u: %u triangles, naive count %u", addr, k, g->ntri, nt);
        sv += g->nvtx; st += g->ntri; sg += g->ng;
    }
    CHECK(sv == o->nvtx && 3 * st == o->nidx && sg == o->ngop, "list %08x: totals vertices %u/%u indices %u/%u gops %u/%u", addr, sv, o->nvtx, 3 * st, o->nidx, sg, o->ngop);
    n_grp += o->ngrp; n_tri += st; n_vtx += sv;
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: nd_dlc_test <shape.bin> ...\n"); return 2; }
    NdDlcEnv env = { env_vtx };
    for (int a = 1; a < argc; a++) {
        FILE* f = fopen(argv[a], "rb");
        if (!f) { printf("FAIL: cannot open %s\n", argv[a]); return 1; }
        fseek(f, 0, SEEK_END); rawlen = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
        raw = malloc(rawlen);
        if (!raw || fread(raw, 1, rawlen, f) != rawlen) { printf("FAIL: read %s\n", argv[a]); return 1; }
        fclose(f);
        /* G_DL targets inside the shape (op 0xDE, w1 an in-shape address) */
        static uint32_t seen[8192];
        unsigned ns = 0;
        for (size_t w = 0; w + 8 <= rawlen; w += 8) {
            uint32_t w0 = be32(raw + w), w1 = be32(raw + w + 4);
            if ((w0 >> 24) != OP_DL || w1 < SHAPE_BASE || (size_t)(w1 - SHAPE_BASE) + 8 > rawlen || (w1 & 7)) continue;
            unsigned k; for (k = 0; k < ns; k++) if (seen[k] == w1) break;
            if (k < ns || ns >= 8192) continue;
            seen[ns++] = w1;
            /* the list words in host order, up to G_ENDDL */
            static uint32_t lw[2 * (ND_DLC_MAXOPS + 1)];
            size_t off = w1 - SHAPE_BASE; unsigned i;
            for (i = 0; i < ND_DLC_MAXOPS && off + 8 * (i + 1) <= rawlen; i++) {
                lw[2 * i] = be32(raw + off + 8 * i); lw[2 * i + 1] = be32(raw + off + 8 * i + 4);
                if ((lw[2 * i] >> 24) == OP_ENDDL) break;
            }
            if (i >= ND_DLC_MAXOPS || off + 8 * (i + 1) > rawlen) { n_ref[NDR_LONG]++; continue; }
            NdObj o;
            int rc = nd_dlc_compile(&o, lw, &env);
            if (rc == NDR_OK) { n_ok++; check_obj(&o, w1, lw); nd_dlc_free(&o); } else n_ref[rc]++;
        }
        free(raw);
    }
    printf("nd_dlc: lists compiled %u (groups %u, vertices %u, triangles %u), refused op %u slot %u vtx %u long %u\n", n_ok, n_grp, n_vtx, n_tri, n_ref[NDR_OP], n_ref[NDR_SLOT], n_ref[NDR_VTX], n_ref[NDR_LONG]);
    CHECK(n_ok > 50 && n_tri > 1000, "too few lists compiled: %u lists, %u triangles", n_ok, n_tri);
    puts(fails ? "nd_dlc: FAIL" : "nd_dlc: PASS");
    return fails != 0;
}
