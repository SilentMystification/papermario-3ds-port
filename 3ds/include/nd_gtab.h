/* nd_gtab.h: native tables of static display lists (NATIVEAPI S7, row S7CONV design note; plan_nativeapi.md section 6). A build-time host
 * tool (3ds/tools/nd_gfx_conv.c, run by 3ds/tools/gen_gfx_tab.sh) reads the typed units of each static NdW2 list of the converted set and
 * writes one NdGTab per list into 3ds/src/nd_gfx_tab.c (generated, never committed). A table is the list in unit order as 8-byte ops: vertex
 * loads with their source as a link address, triangle slot triples, S6T state defs analysed on the host (NdSDef, st 1), calls of other
 * tables, and the units that stay run-time values (pointers to the unit in the list). Sync and no-op units give no op. */
#ifndef ND_GTAB_H
#define ND_GTAB_H
#include <stdint.h>
#include "nd_scomp.h"
#include "nd_api.h"

enum {
    NDG_END = 0, /* the list end (the last op) */
    NDG_VTX,     /* a = n (1 to 64), b = v0 (the first slot), p = const Vtx* (symbol + offset) */
    NDG_TRI,     /* a, b, c = the three slots (the index / 2); ND_FS_TRI2 gives two ops */
    NDG_DEF,     /* a = units, p = const NdGDef* (its units run as ops, NREC5) */
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
#define NDG_F_MOVED 2u /* PMOVE: the device v1 table of a moved particle list: list = &ndd_<name>; the fallback runs its NdNDev units */
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
typedef const void* NdLst; /* FXTYPE: a list handle (NDN(x) or a list name); nd_draw_list, nd_rec_dl and gSPDisplayList take it */
#ifdef ND_GCONV_IN /* GSETB: host compile of the converter inputs (gen_gfx_tab.sh); the gfxin twin defines the list */
#define NDN_DECL(sym) extern Gfx sym[] /* DELGT: host only (ND_GCONV_IN), the gfxin twin defines Gfx */
#define NDN(sym) (sym)
#else
#define NDN_DECL(sym) NDN_DECL_(sym)
#define NDN_DECL_(sym) extern const NdNDev ndd_##sym
#define NDN(sym) NDN_(sym) /* GSETW: N(x) expands first */
#define NDN_(sym) ((NdW2*)&ndd_##sym) /* GSET: the key of a moved list (nd_draw_list, handles) */
#endif

/* S7RUN: the static list call from a table. nd_rec_dl(list) replaces gSPDisplayList(gMainGfxPos++, list) at a call site (push form). When the
 * list has a table (nd_gtab_find, memo of the last 8 lookups) it records one NDC_NAPI record {nd_gtab_run, table, state-only flag}; else it writes
 * the G_DL unit as the macro did (counted). Core 1. nd_gtab_run (nd_walk.c, core 0) draws the table: a state-only list (what rec_inline records in
 * line) as G_PM_LST and its state ops; any other list through the walker's G_DL arms (ent_gdl, nd_sg_try), else its ops. */
void nd_rec_ntab(const void* list); /* DEL5B2: was nd_rec_dl */
void nd_ntab_rec(const uint32_t* rec); void nd_ntab_rec_e(const uint32_t* rec); /* GKEY2: rec[0] = const NdNDev* */

/* CONV2F: nd_draw_list(list, ND_SELF) at the call site of a self-contained v2 table (no entry: the table has its own pipeline state).
 * ENTRY1: an entry is an NdPsoDesc (ND_PSO_DESC); ND_SELF is the null entry. nd_rec_dl_e (nd_gtab.c) passes it to the entcap record. */
#define ND_SELF (*(const NdPsoDesc*)0)
void nd_rec_dl_e(const void* list, const NdPsoDesc* e);
#define nd_draw_list(list, entry) nd_rec_dl_e((list), &(entry))
void nd_rec_dl_key(const void* list, unsigned k); /* FXENT3: capture form; key k 0..14 goes to bits 28-31 of the entcap site word as k + 1 */
#define nd_rec_dl_k(list, k, n) nd_rec_dl_key((list), (unsigned)(k)) /* n = the key count; entapply.py reads it */

/* CONV2A (plan v3 section 2): the native table of one (static list, entry pso) pair. No pointer into a NdW2 array, no typed unit.
 * The converter (CONV2C) writes the tables. nd_ntab_run (CONV2D) runs them on core 0 into the native emit. */
enum {
    NDN_END = 0,  /* end of the op list */
    NDN_PSO,      /* b = NdPso id of the pipeline state from here on */
    NDN_TEX,      /* a = tile (0 or 1), p = const NdBindDesc* */
    NDN_XF,       /* a = gSPMatrix flags (0xFF = pop one modelview matrix), p = const Mtx* (NULL: v = segmented address, input of the caller) */
    NDN_COL,      /* a = 0 prim, 1 env, 2 fog, 3 blend; v = RGBA; b = minlod << 8 | lodfrac for prim */
    NDN_DRAW,     /* p = const Vtx* base (NULL: v = segmented address), b = index count, a = 0 (p = vertex base), 1 (p = ndnv pointers; VSLOT3: an entry below 32 is a caller slot) or 2 (p NULL: indices are caller slots); the indices follow in the table's idx from idx_pos */
    NDN_CALL,     /* p = const NdNTab* of another (list, entry) pair */
    NDN_UNIT,     /* p = const uint32_t* the depth-0 typed units of the list (word pairs, NOPs kept, the end left out, a call stays a unit), b = their count; the first op of every table; CONV2D2 runs them with nd_walk_state */
    NDN_TSZ,      /* FXLT: a tile size on the caller's texture image (no SetTImg before it in the walk): a = tile (0 or 1), v = uls << 12 | ult, idx_pos = lrs << 12 | lrt; the bind of that tile stays the caller's */
    NDN_LIT       /* FXLT: a light unit: a = 0 a Lights1 light (gSPLight: v = the 24-bit unit argument, p = the light), a = 1 the light count (gSPNumLights: v = the 24-bit unit argument, idx_pos = the word) */
};
typedef struct NdNOp { uint8_t op, a; uint16_t b; uint32_t v; const void* p; uint32_t idx_pos; } NdNOp;
typedef struct NdNTab {
    const void* list;     /* the static list this table replaces (the key with entry) */
    NdPso entry;          /* the pipeline state the caller states at nd_draw_list */
    uint16_t nops;
    const NdNOp* ops;     /* nops entries, the last one NDN_END */
    const uint16_t* idx;  /* the index buffer of every NDN_DRAW of the table (absolute vertex indices, CONV2B resolver) */
} NdNTab;
/* CONV2D2 (core 0): the device index of the v2 tables, written by nd_gfx_conv.c after the tables (nd_gfx_tab.c): one entry per table, sorted by
 * list address at the first nd_ntab_find. nd_ntab_find: the table of a list, or NULL. nd_ntab_run: a table as its NDN_UNIT copy in nd_walk.c. */
/* CONV2D2, core 0 only. n = the word pairs of the list from unit 0 to the stop (NOPs counted): nd_walk_state(list, n) runs the unit copy. */
typedef struct NdNDev { const void* list; uint32_t fl; const uint32_t* u; uint32_t nu; uint32_t ntri; } NdNDev; /* GKEY1: fl = nd_gt_state_only of the list (converter) */
#define NDN_FL_PARTS 4u /* DEL5B1: fl bit 2, a flat particle list (NDG_F_PARTS); fl >> 16 = the vertices of all its loads (v1 nvtx) */
void nd_rec_ndev(const NdNDev* e); /* MESHST: record a built NdNDev (nd_mesh.h): it runs as nd_ntab_rec, no list key, no cache */
/* NTABFIND: the record of nd_rec_ndev holds a copy of the entry, not its address. Core 0 runs the record after core 1 goes on, and the owner
 * changes the entry first: imgfx.c imgfx_cache_instance_data and imgfx_load_anim set gfxDev[i].list to NULL (then gt_run gave ent_gdl(NULL) and
 * nd_eg_flat read 0x0..0x7FFC), imgfx_init allocates ImgFXInstances again, animator.c frees a mesh later. rec[0] 0, rec[1] 0 (not state-only),
 * rec[2] the call site, rec[3] 0 (no entry), rec[4..8] the NdNDev, rec[9] 0. nd_ndev_rec (nd_walk.c, core 0) runs the copy as nd_ntab_rec. */
#define ND_NDEV_REC_UNITS 6u /* NAT5T: rec[10], rec[11] the alt range bounds */
_Static_assert(sizeof(NdNDev) == 20u, "NTABFIND: the record holds the 5 words of an NdNDev");
static inline void nd_ndev_pack(uint32_t* rec, const NdNDev* e, uint32_t site) {
    rec[0] = 0u; rec[1] = 0u; rec[2] = site; rec[3] = 0u; rec[9] = 0u; rec[10] = 0u; rec[11] = 0u;
    __builtin_memcpy(rec + 4, e, sizeof *e); /* no <string.h>: include/gcc/memory.h (43F0.c) conflicts with it */
}
static inline void nd_ndev_unpack(NdNDev* d, const uint32_t* rec) { __builtin_memcpy(d, rec + 4, sizeof *d); }
void nd_ndev_rec(const uint32_t* rec);
extern const NdNDev* ndn_dev_idx[]; /* GKEY1: &ndd_<list> per entry, sorted by ->list at the first nd_ntab_find */
extern const unsigned ndn_dev_n;
const NdNDev* nd_ntab_find(const void* list);
unsigned nd_gt_state_only(const uint32_t* l); /* 1: the end within 64 units, every unit before it a state op or a sync */
int nd_ntab_flat(const NdNDev* e, void* out, unsigned* n, unsigned max, int depth, int (*call)(uint32_t w1, unsigned* n, int depth)); /* EMIT4: the nd_eg_flat words of a v2 list from e->u (1 done, 0 refused) */
#endif
