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

#endif
