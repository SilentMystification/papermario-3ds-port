/* nd.h: native draw record and frame driver (plan_native_system 2.2, S1-2). Only the LEGACY bridge kind exists so far. */
#ifndef ND_H
#define ND_H

#include <stdint.h>

enum {
    ND_DRAW_IDX = 0, /* later rows: indexed static mesh */
    ND_LEGACY = 7    /* bridge only: the walker runs the words of one list span (deleted in S10) */
};

/* 32 bytes, one cache line. The LEGACY fields use the first 8 bytes; the rest is reserved for the draw fields. */
typedef struct NdRec {
    uint8_t kind;
    uint8_t pad[3];
    uint32_t a; /* LEGACY: list address (32-bit ARM11 pointer) */
    uint32_t b; /* LEGACY: byte count */
    uint32_t rsv[5];
} NdRec;
_Static_assert(sizeof(NdRec) == 32, "NdRec is one 32-byte line");

void nd_frame_begin(void);                    /* empties the record array (once per gbi task) */
void nd_bridge_cut(void* list, unsigned nbytes); /* appends one LEGACY record for an N64 list span */
void nd_emit(void);                           /* runs the records in order; LEGACY calls the walker */

/* NDTMPL (S1-3): render state template per key. The key holds the inputs of flush_draw's state setters.
 * Stencil (zmark pass) and the blend_off path are not in the template yet. */
typedef struct NdTmplKey {
    uint8_t cull;         /* GPU_CULLMODE as the draw sets it (0 when not raw) */
    uint8_t z, zmark, zcmp, zupd, nocol, decal; /* flush_draw flags */
    uint8_t atest_on, aref;                     /* alpha test: on and ref */
    uint8_t blend;                              /* 1: source alpha blend, 0: add ONE/ZERO */
    uint32_t dbias;                             /* decal bias, float bits (used when decal is set) */
} NdTmplKey;
typedef struct NdTmplWord { uint16_t reg; uint32_t val; } NdTmplWord;
#define ND_TMPL_WORDS 7

/* returns the template id (0..255) of a key, building it on a miss; -1 when the table is full */
int nd_tmpl_build(const NdTmplKey* k);
/* the register writes of template id; returns the count (ND_TMPL_WORDS max) */
unsigned nd_tmpl_words(int id, const NdTmplWord** out);

#endif
