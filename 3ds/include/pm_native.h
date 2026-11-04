#ifndef PM_NATIVE_H
#define PM_NATIVE_H
/* NDS1 phase B layout (decided 2026-11-04): a template is the captured record of a static plain model minus its six per-draw
 * commands (palette 0x2C1, unit 0 texture 0x081, unit 1 texture 0x091, index address 0x227, index count 0x228, trigger 0x22F).
 * A draw is the 36-word group of those six commands, patched per draw. Pure C on u32 words, no citro3d, so host tests build it with pm_comb.c. */
#include <stdint.h>
#include "pm_gpu.h"

#define NAT_REC_MAX    512  /* records per frame */
#define NAT_TMPL_MAX   64   /* templates, id 0..63 (6 bits) */
#define NAT_TMPL_WORDS 320  /* state words of a template: 234 to 278 for one draw; a two-draw record (376 words) has 300 */
#define NAT_GRP_WORDS  38   /* per-draw group: uniform index 2, palette 18, tex0 6, tex1 6, index address 2, index count 2, trigger 2 */
#define NAT_MTX_MAX    96   /* palette slots per eye */
#define NAT_TEX_MAX    128  /* texture handles, id 0..127 */

/* 16 bytes. One record per draw. */
typedef struct {
    uint8_t  tmpl;     /* template id, 0..63 */
    uint8_t  tex0;     /* texture handle, unit 0 (0..127) */
    uint8_t  tex1;     /* texture handle, unit 1 (0..127) */
    uint8_t  flags;    /* reserved, 0 */
    uint16_t mtx;      /* palette slot, 0..95 */
    uint16_t nidx;     /* value of the index count command (0x228) */
    uint32_t idx_off;  /* byte offset of the first index, from nat_idx_base */
    uint32_t vtx_base; /* Vtx base address (not patched in this layout) */
} NatRec;

/* Where a palette patch landed in the emitted span (word index from the span start) and which slot it read. Used by nat_eye_copy. */
typedef struct { uint32_t at; uint16_t slot; uint16_t pad; } NatMtxLog;

typedef struct {
    uint32_t cw0, cw1, geom, oh, ol; /* key: combine words, geometry and other-mode words */
    uint8_t  z, decal, blend, atest, t1only, used, ordered; /* ordered: 1 for blend, decal or z off (keep game order) */
    uint16_t blen;                   /* state words in blk */
    uint32_t grp[NAT_GRP_WORDS];     /* per-draw group prototype (headers and the values of the capture) */
    uint32_t blk[NAT_TMPL_WORDS] __attribute__((aligned(32)));
} NatTmpl;

extern NatTmpl nat_tmpl[NAT_TMPL_MAX];
extern uint32_t nat_mtx[2][NAT_MTX_MAX][16] __attribute__((aligned(32))); /* palette words per eye */
extern uint32_t nat_tex[NAT_TEX_MAX][4];  /* unit texture block per handle: dims, param, border, address */
extern uint32_t nat_idx_base;             /* index arena base address, set per frame */

/* Find or add the template for (cw0, cw1, flags). Returns the id (0..63), or -1 when full. plan_out and rc_out receive the pm_comb plan and
 * return code when not NULL. */
int nat_tmpl_add(uint32_t cw0, uint32_t cw1, uint32_t geom, uint32_t oh, uint32_t ol, int z, int decal, int blend, int atest,
    int t1only, PmPlan* plan_out, int* rc_out);



/* E2: texture block (4 words) to id, keyed by content. Returns the id (0..NAT_TEX_MAX-1), or -1 when the table is full. */
int nat_tex_id(const uint32_t* blk);

/* Draw order for n records into ord (indices, 16-bit). Records whose template is ordered keep their place and break runs. Inside
 * each run of unordered records a stable counting sort by tex0 then by tmpl (LSD, two passes). Returns n. */
unsigned nat_order(const NatRec* rec, unsigned n, uint16_t* ord);

/* Emit the draws in ord for one eye into out (cap words). A template block is copied when the template changes, then each draw's group.
 * log receives one entry per palette patch. Returns words written, 0 when out is too small. */
unsigned nat_emit(const NatRec* rec, unsigned n, const uint16_t* ord, uint32_t* out, unsigned cap, unsigned eye,
    NatMtxLog* log, unsigned* nlog);

/* Eye 2 without a second walk: copy the eye 1 span, then write the eye 2 palette words at each logged position. */
void nat_eye_copy(uint32_t* dst, const uint32_t* src, unsigned nw, const NatMtxLog* log, unsigned nlog, unsigned eye);

uint32_t nat_gpu_addr(uint32_t phys); /* E3: GPU index address word of a phys address */

#endif
