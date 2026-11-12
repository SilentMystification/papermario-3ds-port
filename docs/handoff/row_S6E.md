# Row S6E: entity draws (ent_run) run compiled state ranges, not ENT_GBI op replay (NATIVEAPI F2)

Model: Sonnet (user decides).
- Why not Haiku: ent_run (3ds/src/nd_ent.c:389-417) replays the state words between segments with ENT_GBI = nd_ops_run (:381),
  and the inherited state differs per call site. The segment compile (ent_compile, :195) must store a range id per gap, keyed as
  S6T defines. The interpreter path (ent_run_seg_interp, :386) stays the fallback until S8.
- Why not Opus: the form is S6T's.

## Blocker and base
S6T merged (read its design note first). `sh 3ds/tools/wt_new.sh S6E`; PM_ROW=S6E PM_LANE=S6E.

## Code read
ent_run per call:
- the content hash per frame;
- per segment: ENT_GBI of the state words before the segment (op0 > a0), then ent_seg_native or the interpreted segment;
- the trailing state words;
- tri_since_vtx, vert_live, dlc_dirty.
Readers of the gaps: EntSeg {a0, op0, op1, tri0, ntri} (3ds/include/nd_int.h:342). Writers: ent_compile. Also SaveBlock
(src/entity/SaveBlock.c:38-43, :60-68): matrix, render mode, combine, prim, then its list through the entity path.

## Design
1. ent_compile also compiles each gap [a0, op0) and the tail [op1, nops) into S6T range ids (cold). EntSeg gets `u16 rid` (and the
   tail its own). Compile under the inherited signature of the first call: a call under another signature compiles a second id
   (S6T key rule).
2. ent_run applies the range output (S6T runner) instead of ENT_GBI. The interpreted segment path is unchanged.
3. SaveBlock and the other src/entity custom renders: their state lines become one S6S-style range each.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- DIRECT byte set (3 starts) plus a save-block scene (pre-flight: grep the scenarios for "SaveBlock" or "save block"; else
  spot_mac01_det has entities and is enough). Every shot byte-equal. s6chk 0.
- entnat census (the ent_c counters in the census line): nat unchanged; ENT_GBI count (nd_ent_gbi) 0 in steady frames.
- listcen reader down. Budget 8.

## Stop
A byte difference or an s6chk mismatch: stop, commit, hand back. Delete this row file in the completing commit.
