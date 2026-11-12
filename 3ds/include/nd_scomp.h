/* nd_scomp.h: the state compiler (NATIVEAPI S6T, row_S6T.md design note). A producer range of typed state ops (a def: static units in stream
 * order) is compiled once into what it leaves for the next draw: the masked othermode and geometry writes, the texture fields, the combine words
 * and the combine decision (the pso of the range, keyed by the inherited decision inputs). Value units (scissor, viewport, matrix, colour image,
 * fill, fog, blend, prim depth, light and segment words) stay run-time ops and come with each record. Recorded on core 1 (nd_scomp_rec), run on
 * core 0 (nd_scomp_run, an NDC_NAPI runner). A def the compiler refuses is recorded as its ops (the old path). */
#ifndef ND_SCOMP_H
#define ND_SCOMP_H
#include <stdint.h>

typedef struct NdSDef {
    const void* u;          /* the units (Gfx, typed gs forms) in stream order; a value slot holds a placeholder of its kind */
    uint8_t n;              /* units, at most 32 */
    /* set by nd_scomp_analyse (core 1, the first record), read only after */
    uint8_t st;             /* 0 not analysed, 1 compiled form, 2 refused (recorded as ops) */
    uint8_t id;             /* the def id (0 to 15) of a compiled def */
    uint8_t nv, pre;        /* value units; value units before the first static unit */
    uint8_t has_geom, has_tex, has_comb, last_op;
    uint32_t vmask;         /* bit k: unit k is a value unit */
    uint32_t omh_m, omh_v, oml_m, oml_v; /* othermode H and L: the bits the def sets and their values */
    uint32_t geom_c, geom_s;             /* geometry: geom = (geom & geom_c) | geom_s */
    uint32_t tex_a, tex_b, cw0, cw1;     /* the last G_TEXTURE unit (a, b) and the combine words (NDO_COMB_W0 form) */
} NdSDef;

#define ND_SC_KEYW 30u /* key words: the def, 25 decision inputs, 4 walker inputs of the sync memo (sc_exec) */

/* Pure parts (host test nd_scomp_test.c). */
int nd_scomp_analyse(NdSDef* d);  /* 1 compiled form, 0 refused; sets the fields above */
struct NdCombIn;
/* The key of def d run on the inherited inputs in (the walker's NdCombIn before the range) and vprim (pm_comb_vprim): fields d sets take d's values. */
void nd_scomp_key(const NdSDef* d, const struct NdCombIn* in, uint32_t vprim, uint32_t key[ND_SC_KEYW]);
/* The compiled entry table (the ids): find key, or take a slot for it (*miss = 1). Returns the entry id. */
#define ND_SC_N 128u
typedef struct NdSTab { uint32_t key[ND_SC_N][ND_SC_KEYW]; uint8_t used[ND_SC_N]; uint8_t last[16]; unsigned next; } NdSTab;
unsigned nd_scomp_find(NdSTab* t, unsigned def_id, const uint32_t key[ND_SC_KEYW], int* miss);

/* Core 1: record def d with its nv value units v (typed units, made with the g macros into a local packet). */
void nd_scomp_rec(NdSDef* d, const void* v, unsigned nv);
/* Core 0: the NDC_NAPI runner. */
void nd_scomp_run(const uint32_t* rec);
int nd_scomp_exec(NdSDef* d, const uint32_t* v, unsigned nv); /* core 0: run d with nv value units; 0 = refused (run the ops) */
int nd_scomp_exec_own(NdSDef* d); /* core 0: run d whose value units are its own (static list data); 0 = refused */
#endif
