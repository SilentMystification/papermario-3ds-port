# Row S6E: entity list gaps run as S6T compiled ranges, not ENT_GBI op replay (NATIVEAPI F2, part 1)

(Model: Haiku. Exact code below. Two files. The src/entity custom renders (SaveBlock and others) are a later row, S6E2.)

## Blocker and base
SCOMP0 and SCOMP1 merged. `sh 3ds/tools/wt_new.sh S6E` from that main; PM_ROW=S6E PM_LANE=S6E on every build and bench call.

## Code read (oracle 4, main eacd96ed9)
- ent_run (3ds/src/nd_ent.c:393-417) runs the gap before each segment (:407) and the tail (:412) with ENT_GBI (:381) =
  nd_ops_run(l, n, 1): gv_epoch++, dlc_on = 1, pm_walk_side++/--, nd_walk_state, gv_streak_flush, tex_streak_flush (nd_walk.c:396-412).
- Gaps index k->fw (malloc :208-210; freed :202 and ent_reset :257). EntSeg/EntKey: nd_int.h:342-357, used in nd_ent.c only.
- A gap with a refused unit (VTX, RARE, a draw, a texture/PRIM/ENV unit between statics) is refused once (st 2) and runs as ops.
  A freed def's address reused by a new def is harmless (an entry's decision is a pure function of key words 1-25).

## Touch only
nd_int.h (EntSeg, EntKey); nd_ent.c (includes, ent_compile, ent_reset, ent_run, ent_log, new ent_sdef*, ent_gap). No other row.

## Exact edits
1. nd_int.h :342: `typedef struct { u32 a0, op0, op1, tri0, ntri; struct NdSDef* sd; } EntSeg;` (sd: the gap before the segment,
   or NULL). EntKey: after `Gfx* fw;` add `struct NdSDef* tail; /* S6E: the units after the last segment, or NULL */`.
2. nd_ent.c includes: add `#include "nd_scomp.h"` after `#include "nd.h"`.
3. nd_ent.c, before ent_compile (above :195), add:
   ```
   unsigned nd_ent_sc; /* S6E: gaps run as compiled ranges (entsync line) */
   static NdSDef* ent_sdef(const Gfx* l, u32 a, u32 b) { /* the gap [a, b) of l as an S6T def (1 to 32 units), else NULL */
       NdSDef* d = (b <= a || b - a > 32u) ? NULL : (NdSDef*)calloc(1, sizeof *d);
       if (d) { d->u = l + a; d->n = (uint8_t)(b - a); } return d; }
   static void ent_sdef_free(EntKey* k) { for (unsigned s = 0; s < k->nseg && k->seg; s++) free(k->seg[s].sd); free(k->tail); k->tail = NULL; }
   ```
4. ent_compile :201: put `ent_sdef_free(k);` right before `free(k->seg); k->seg = NULL; k->nseg = 0;`.
   In the seg loop (:241-244) add as its last statement `k->seg[s].sd = ent_sdef(l, k->seg[s].a0, k->seg[s].op0);`.
   After the loop add `k->tail = ent_sdef(l, k->seg[ns - 1].op1, nops);`.
5. ent_reset :257: put `ent_sdef_free(ent_tab[i]);` right before `free(ent_tab[i]->seg);`.
6. After the ENT_GBI macro (:381) add:
   ```
   static void ent_gap(NdSDef* d, const Gfx* l, u32 n) { /* S6E: a gap as a compiled range, with nd_ops_run's list start and end */
       if (d && d->st != 2u) {
           gv_epoch++; dlc_on = 1; pm_walk_side++; const int ok = nd_scomp_exec_own(d); pm_walk_side--;
           if (ok) { nd_ent_sc++; gv_streak_flush(); tex_streak_flush(); return; } }
       ENT_GBI((void*)l, n * 8u);
   }
   ```
7. ent_run :407 -> `if (e->op0 > e->a0) ent_gap(e->sd, fw + e->a0, e->op0 - e->a0);`
   :412 -> `{ const EntSeg* e = &k->seg[k->nseg - 1]; if (k->nops > e->op1) ent_gap(k->tail, k->fw + e->op1, k->nops - e->op1); }`
   ent_run_seg_interp (:387-392) keeps ENT_GBI.
8. ent_log (:309, the entsync line): add ` sc %u` with `nd_ent_sc` after `walks %u` (same static-delta form as walks: add a static
   l3). Census only.

## Gate (expected values)
- `PM_ROW=S6E PM_LANE=S6E sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row S6E --why "S6E entity gaps compiled"`: PASS.
- `PM_ROW=S6E PM_LANE=S6E python 3ds/tools/bench.py base spot_mac01_det`, the same for spot_mac00_det.
- One list: `python 3ds/tools/bench.py run spot_mac01_det,spot_mac00_det,s6chk_mac01,ent_kmr03_pic --row S6E --why "S6E: gaps
  compiled"` (4 starts). `bench.py cmp` spot_mac01_det and spot_mac00_det: every shot byte-equal. s6chk line: `bad 0`.
  ent_kmr03_pic (census, not a byte gate): hand back its `entsync ... walks <w> sc <s>` and `entnat ... native <n>` lines;
  expected sc above 0 (walks still counts refused gaps and interpreted segments).
- No `scomp: def` line in any log. Budget 6 starts.

## Stop and end
A byte difference, `bad` above 0, an edit that does not fit, or a test-tool fault: commit, hand back the exact output. Delete this
row file in the completing commit. Hand back: the ready hash, the gate values, the memcheck line, the entsync and entnat lines.
