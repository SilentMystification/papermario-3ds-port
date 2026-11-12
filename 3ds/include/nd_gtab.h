/* nd_gtab.h: native tables of static display lists (NATIVEAPI S7, row S7CONV design note; plan_nativeapi.md section 6). A build-time host
 * tool (3ds/tools/nd_gfx_conv.c, run by 3ds/tools/gen_gfx_tab.sh) reads the typed units of each static Gfx list of the converted set and
 * writes one NdGTab per list into 3ds/src/nd_gfx_tab.c (generated, never committed). A table is the list in unit order as 8-byte ops: vertex
 * loads with their source as a link address, triangle slot triples, S6T state defs analysed on the host (NdSDef, st 1), calls of other
 * tables, and the units that stay run-time values (pointers to the unit in the list). Sync and no-op units give no op. */
#ifndef ND_GTAB_H
#define ND_GTAB_H
#include <stdint.h>
#include "nd_scomp.h"

enum {
    NDG_END = 0, /* the list end (the last op) */
    NDG_VTX,     /* a = n (1 to 64), b = v0 (the first slot), p = const Vtx* (symbol + offset) */
    NDG_TRI,     /* a, b, c = the three slots (the index / 2); NDO_TRI2 gives two ops */
    NDG_DEF,     /* a = units, p = const NdGDef* (nd_scomp_exec_own(&def->d) runs it) */
    NDG_UNIT,    /* a = 1, p = const uint32_t*: the unit in the list, run as a typed op (values, rare and rect units, unmapped pointers) */
    NDG_CALL     /* a = 1 for the branch form (G_DL_NOPUSH), p = const NdGTab* (the callee) */
};
typedef struct NdGOp {
    uint8_t k, a, b, c;
    const void* p;
} NdGOp;

/* Inherited inputs of a def's combine decision (nd_scomp_key groups a static list can set; a set bit = not set by the def before its
 * decision) and the context inputs no list sets (keyed by the record kind). The S9DEL2 condition: every def inherit 0. */
#define NDG_IN_COMB 0x01u  /* comb words, tst[16], use_t1, box_prod, box_folded, comb_set, comb_alpha_shade: a COMB unit */
#define NDG_IN_CYC 0x02u   /* the omode_h cycle bits: OMH units that cover both bits */
#define NDG_IN_TEXON 0x04u /* G_TEXTURE */
#define NDG_IN_PRIM 0x08u  /* prim, prim_lod low 8: a PRIM unit before the first static unit */
#define NDG_IN_ENV 0x10u   /* env: an ENV unit before the first static unit */
#define NDG_IN_KEY 0x20u   /* key_c, key_s, conv_k4: never set by a def (RARE units are refused) */
#define NDG_CTX_T1V 0x1u   /* td[1].valid */
#define NDG_CTX_DUAL 0x2u  /* dual_on */
#define NDG_CTX_VPRIM 0x4u /* pm_comb_vprim (1 only in particle records) */

typedef struct NdGDef {
    NdSDef d;          /* made on the host by nd_scomp_analyse (st 1); d.u = the list + k0 units */
    uint32_t inherit;  /* NDG_IN_* */
    uint16_t ctx;      /* NDG_CTX_* */
    uint16_t k0;       /* the first unit of the def in the list */
} NdGDef;

#define NDG_F_PARTS 1u /* a flat particle list (the parts_ok rule): VTX at slot 0 first, then only VTX and TRI to END, every load and index inside the loaded slots */
typedef struct NdGTab {
    const uint32_t* list; /* the source list (its typed units) */
    const NdGOp* op;      /* nop ops, the last NDG_END */
    uint16_t nop, flags;
    uint16_t nvtx, ntri;  /* vertices of all loads, triangles */
} NdGTab;

typedef struct NdGIdx {
    const void* list;
    const NdGTab* tab;
} NdGIdx;
extern NdGIdx nd_gtab_idx[]; /* generated: one entry per table; sorted by list address at the first nd_gtab_find */
extern const unsigned nd_gtab_n;

/* The table of a list, or NULL. Core 1 only (records carry the table pointer to core 0). */
const NdGTab* nd_gtab_find(const void* list);

/* A converted list by symbol: NDG_DECL(D_09001000_3583A0); then NDG(D_09001000_3583A0) is its const NdGTab*. */
#define NDG_DECL(sym) extern const NdGTab ndg_##sym
#define NDG(sym) (&ndg_##sym)
#endif
