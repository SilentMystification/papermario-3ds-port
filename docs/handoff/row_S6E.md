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
- Speed rule (oracle 5): a bench list runs at one speed; spot_mac00_det is a turbo bench (bench_defs speed turbo), every
  other bench here is normal. Run it as its own list: `bench.py run spot_mac00_det --row S6E --why "..."` (1 start).
- `PM_ROW=S6E PM_LANE=S6E sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row S6E --why "S6E entity gaps compiled"`: PASS.
- Try 2 (oracle 5, after the fact check): try 1 ran spot_mac00_det alone at turbo against a turbo base, so the speed was not
  the cause. The base is valid: the branch merge-base is 8354d6335 and the stored base ed83069e6 is its docs-only child
  (`git diff --stat 8354d6335 ed83069e6` lists only docs/handoff files): the same code minus S6E. So the 0/6 is a code effect of
  12cc94a0e in mac_00 (S5SPR2 was 6/6 there at turbo).
- Named paths (code read; the one that fires in mac_00 is not proven by reading): sc_exec (3ds/src/nd_scomp.c:202-221) runs the
  value units in two groups: `pre` before the folded block, the rest after the block and after the decision and sc_ok/sc_sig
  (:216-217). The op path (ENT_GBI = nd_ops_run in order) runs each unit in place. So in a gap with a value unit after the first
  static unit: (a) a class-2 unit between static units (MOVEMEM lights, MW NUMLIGHT/FOG/SEG, MTX, FOG, FILL, SCISSOR) moves
  behind the later static units: a light write after a GEOM G_LIGHTING change runs lit_freeze (nd_geom.c:163) on another state,
  and a fog value after G_FOG runs on another state; (b) a class-4 unit after the last static (PRIM, ENV, texture loads) runs
  after the decision. s6chk compares 50 walker fields after the range but not the light, fog and GPU-side state of (a). An
  entity drawn with a different fog or light state, and the fog and light state left to every later draw (the tail range runs
  after the last entity segment), can change the whole frame.
- Fix (exact, nd_ent.c ent_gap only): take the compiled path only when every value unit precedes the first static unit (the
  op order; nd_scomp_analyse sets nv and pre). Replace the whole function ent_gap by:
  ```c
  static void ent_gap(NdSDef* d, const Gfx* l, u32 n) {
      if (d && !d->st) d->st = nd_scomp_analyse(d) ? 1u : 2u;
      if (d && d->st == 1u && d->nv == d->pre) {
          gv_epoch++; dlc_on = 1; pm_walk_side++; const int ok = nd_scomp_exec_own(d); pm_walk_side--;
          if (ok) { nd_ent_sc++; gv_streak_flush(); tex_streak_flush(); return; } }
      ENT_GBI((void*)l, n * 8u);
  }
  ```
- Gate: build, memcheck OK, nd_host PASS. `bench.py base spot_mac00_det --commit $(git merge-base HEAD main)` (turbo base),
  `bench.py run spot_mac00_det --row S6E --why "S6E try 2: in-order gaps"` alone (turbo), `bench.py cmp spot_mac00_det --row S6E
  --commit <the same merge-base>`: 6/6 byte-equal. Then one normal list `bench.py run spot_mac01_det,s6chk_mac01,ent_kmr03_pic
  --row S6E` (3 starts): spot_mac01_det 6/6 byte-equal, s6chk bad 0, entsync `sc` above 0 (hand back sc and walks; sc may drop
  from try 1's 2016: the gaps with late values now walk).
- If spot_mac00_det still differs: stop, commit, hand back the cmp lines. The cause is then not the value order; by CLAUDE.md 1
  the trace goes to the user.
- No `scomp: def` line in any log. Budget 8 after the orchestrator's reset; try 2 needs 5 or 6 starts.

## Stop and end
A byte difference, `bad` above 0, an edit that does not fit, or a test-tool fault: commit, hand back the exact output. Delete this
row file in the completing commit. Hand back: the ready hash, the gate values, the memcheck line, the entsync and entnat lines.
