/* nd_pica_decode.c: decode a PICA command list (u32 words, little endian) into a text draw report.
 * Usage: nd_pica_decode <words.bin>. One line per draw trigger (GPUREG_DRAWELEMENTS 0x022F or DRAWARRAYS 0x022E):
 *   draw <i> cull <c> tw0 <dim> <param> <lod> <addr> tw1 <dim> <param> <lod> <addr> ibuf <word> tris <n>
 * Header (3dbrew, not on disk, S0-3 checks it against ndref): bits 0..15 register, bits 16..19 write mask,
 * bits 20..30 extra parameter count (parameters = field + 1), bit 31 consecutive writes.
 * A list with an odd parameter count is followed by one padding word.
 * Register numbers: 3ds/gpu/registers.h of libctru (docker pm3ds-build:1). Not decoded yet: TEV, blend, depth, alpha,
 * uniforms, vertex layout (S1). Vertices are not resolved here. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define R_CULL 0x040
#define R_T0_DIM 0x082
#define R_T0_PARAM 0x083
#define R_T0_LOD 0x084
#define R_T0_ADDR 0x085
#define R_T1_DIM 0x092
#define R_T1_PARAM 0x093
#define R_T1_LOD 0x094
#define R_T1_ADDR 0x095
#define R_IBUF 0x227
#define R_NUMV 0x228
#define R_DRAWARR 0x22E
#define R_DRAWELEM 0x22F

typedef struct { uint32_t cull, t0[4], t1[4], ibuf, numv; } PState;

/* Walks the list and prints one line per draw trigger. Returns the draw count, or -1 on a truncated list. */
int nd_decode_words(const uint32_t* w, size_t n, FILE* out) {
    PState s = {0};
    int draws = 0;
    for (size_t i = 0; i < n;) {
        uint32_t h = w[i++];
        uint32_t reg = h & 0xFFFFu, inc = h >> 31;
        unsigned cnt = ((h >> 20) & 0x7FFu) + 1u;
        if (i + cnt > n) return -1;
        for (unsigned k = 0; k < cnt; k++) {
            uint32_t v = w[i + k];
            uint32_t r = inc ? reg + k : reg;
            switch (r) {
            case R_CULL: s.cull = v; break;
            case R_T0_DIM: s.t0[0] = v; break;
            case R_T0_PARAM: s.t0[1] = v; break;
            case R_T0_LOD: s.t0[2] = v; break;
            case R_T0_ADDR: s.t0[3] = v; break;
            case R_T1_DIM: s.t1[0] = v; break;
            case R_T1_PARAM: s.t1[1] = v; break;
            case R_T1_LOD: s.t1[2] = v; break;
            case R_T1_ADDR: s.t1[3] = v; break;
            case R_IBUF: s.ibuf = v; break;
            case R_NUMV: s.numv = v; break;
            case R_DRAWARR: case R_DRAWELEM:
                fprintf(out, "draw %d cull %u tw0 %08x %08x %08x %08x tw1 %08x %08x %08x %08x ibuf %08x tris %u\n", draws,
                        s.cull, s.t0[0], s.t0[1], s.t0[2], s.t0[3], s.t1[0], s.t1[1], s.t1[2], s.t1[3], s.ibuf, s.numv / 3u);
                draws++;
                break;
            default: break;
            }
        }
        i += cnt;
        if (cnt & 1u) i++; /* padding word of an odd parameter list */
    }
    return draws;
}

#ifndef ND_DECODE_NO_MAIN
int main(int argc, char** argv) {
    if (argc != 2) { fprintf(stderr, "usage: nd_pica_decode <words.bin>\n"); return 2; }
    FILE* f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", argv[1]); return 2; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    size_t n = (size_t)sz / 4u;
    uint32_t* w = malloc(n * 4u + 4u);
    if (!w || fread(w, 4, n, f) != n) { fprintf(stderr, "read failed\n"); return 2; }
    fclose(f);
    int d = nd_decode_words(w, n, stdout);
    free(w);
    if (d < 0) { fprintf(stderr, "truncated command list\n"); return 1; }
    return 0;
}
#endif
