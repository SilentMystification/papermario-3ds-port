#ifndef PM_NATIVE_H
#define PM_NATIVE_H
/* NDS1 phase A: native draw record, template table, palette matrix pool, draw order and the eye-2 patch.
 * Pure C on u32 words. No citro3d and no game header, so host tests build it with pm_comb.c. Not called by the game yet (phase B). */
#include <stdint.h>
#include "pm_gpu.h"

#define NAT_REC_MAX   512  /* records per frame */
#define NAT_TMPL_MAX  64   /* templates, id 0..63 (6 bits) */
#define NAT_TMPL_WORDS 96  /* state block, copied once per bucket (384 B) */
#define NAT_DRAW_WORDS 32  /* draw block, copied per record and patched (128 B) */
#define NAT_MTX_MAX   96   /* palette matrix slots per eye */
#define NAT_TEX_MAX   128  /* texture handles, id 0..127 */
#define NAT_PAT_MAX   8    /* patch entries in the draw block layout */

/* 16 bytes. One record per draw. */
typedef struct {
    uint8_t  tmpl;     /* template id, 0..63 */
    uint8_t  tex0;     /* texture handle, unit 0 (0..127) */
    uint8_t  tex1;     /* texture handle, unit 1 (0..127), 0 when unused */
    uint8_t  flags;    /* reserved, 0 */
    uint16_t mtx;      /* palette matrix slot, 0..95 */
    uint16_t nidx;     /* index count of this draw */
    uint32_t idx_off;  /* byte offset of the first index in the index arena */
    uint32_t vtx_base; /* Vtx base address (ROM or arena) */
} NatRec;

enum { NAT_P_TEX0, NAT_P_TEX1, NAT_P_IDX, NAT_P_NIDX, NAT_P_MTX };

/* One patch entry of the draw block. off is a word index into the 32-word draw block. NAT_P_MTX writes 16 words. */
typedef struct { uint16_t off; uint8_t kind; uint8_t pad; } NatPatch;

/* Where an MTX patch landed in the emitted span (word index from the span start) and which slot it read. Used by nat_eye_copy. */
typedef struct { uint32_t at; uint16_t slot; uint16_t pad; } NatMtxLog;

/* Template table. Built with nat_tmpl_add; the block is filled by the phase B capture. */
typedef struct {
    uint32_t cw0, cw1;             /* N64 combine words of the pair (pm_comb_build input) */
    uint32_t geom, oh, ol;         /* geometry mode and other-mode words */
    uint8_t  z, decal, blend, atest, t1only; /* flags: draw order, and the pm_comb t1only argument */
    uint8_t  used, ordered;        /* ordered: 1 for blend or decal, these keep game order */
    uint32_t blk[NAT_TMPL_WORDS] __attribute__((aligned(32)));
} NatTmpl;

extern NatTmpl nat_tmpl[NAT_TMPL_MAX];
extern uint32_t nat_drw[NAT_DRAW_WORDS] __attribute__((aligned(32)));
extern uint32_t nat_mtx[2][NAT_MTX_MAX][16] __attribute__((aligned(32))); /* encoded palette words per eye */
extern uint32_t nat_tex_word[NAT_TEX_MAX];  /* texture address word per handle, set per frame */
extern uint32_t nat_idx_base;               /* index arena base word, set per frame */

/* Set the draw block layout: n patch entries and the 32-word draw block with its fixed words. */
void nat_layout_set(const NatPatch* pt, unsigned n, const uint32_t* drw);

/* Find or add the template for (cw0, cw1, flags). Returns the id (0..63), or -1 when full. The pm_comb plan is built once per new id;
 * when plan_out is not NULL it receives a copy of that plan. rc_out receives the pm_comb_build return code (0 is success). */
int nat_tmpl_add(uint32_t cw0, uint32_t cw1, uint32_t geom, uint32_t oh, uint32_t ol, int z, int decal, int blend, int atest,
    int t1only, PmPlan* plan_out, int* rc_out);

/* Draw order for n records into ord (indices, 16-bit). Records whose template is ordered keep their place and break runs. Inside
 * each run of unordered records a stable counting sort by tex0 then by tmpl (LSD, two passes). Returns n. */
unsigned nat_order(const NatRec* rec, unsigned n, uint16_t* ord);

/* Emit the draws in ord for one eye into out (cap words). A template block is copied when the template changes. Each draw copies the
 * draw block and writes its patch entries. log receives one entry per MTX patch. Returns words written, 0 when out is too small. */
unsigned nat_emit(const NatRec* rec, unsigned n, const uint16_t* ord, uint32_t* out, unsigned cap, unsigned eye,
    NatMtxLog* log, unsigned* nlog);

/* Eye 2 without a second walk: copy the eye 1 span, then write the eye 2 palette words at each logged MTX position. */
void nat_eye_copy(uint32_t* dst, const uint32_t* src, unsigned nw, const NatMtxLog* log, unsigned nlog, unsigned eye);

#endif
