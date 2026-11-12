# Row S9DEL3: delete the typed-op stream (NDC_STATE, nd_rec_state, nd_op.h typed ops, the nd_gbi.h macro mapping, the walker G_* arms)

Model: Sonnet (user decides).
- Why not Haiku: nd_gbi.h maps every gDP/gSP macro to typed units. When it goes, any game call site still using a macro stops
  compiling. That is the proof, but the coder must tell a leftover call site (a fault of an earlier row: hand back) from a macro
  only used under `#else` (S9DEL4 deletes it).
- Why not Opus: deletion.

## Blocker and base
S9DEL2 merged. `sh 3ds/tools/wt_new.sh S9DEL3`; PM_ROW=S9DEL3 PM_LANE=S9DEL3.

## Pieces
- 3ds/src/nd_rec.c: NDC_STATE, st_put, nd_rec_state, rec_open for state runs.
- 3ds/include/nd_op.h: the whole file, if no native record uses an NDO_ id. Native records use their own kinds.
- 3ds/include/nd_gbi.h: the whole macro mapping (ND_GX, ND_GS, the typed macro overrides). The ND_W0_TR rect-word macros, if
  still used by a native caller, move to nd_api.h.
- nd_walk.c: the G_* and NDO_* arms of the stream walk. The frame stream keeps only NDC_NAPI and the native record kinds.

## Gate (expected values)
- Full build with the 3DS target: no game file includes nd_gbi.h, and no compile error from a leftover macro in a 3DS-compiled
  branch (a leftover: stop and hand back the file and line).
- nd_host PASS. The byte set of S9DEL1: every shot byte-equal.
- `grep -rn "NDC_STATE\|nd_rec_state\|NDO_\|ND_GX\|ND_GS" 3ds/src 3ds/include` returns nothing.
- listcen reader 0. Budget 8.

## Stop
A byte difference or a leftover call site: stop, commit, hand back. Delete this row file in the completing commit.
