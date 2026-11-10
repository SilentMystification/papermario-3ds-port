# Row HC4e1: compile-time typed op macros

(Model: Haiku. Oracle batch 2, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh HC4e1`; PM_ROW=PM_LANE=HC4e1 on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Goal
ND_X0(w0,w1) and ND_X1(w0,w1) in 3ds/include/nd_op.h. They give the same unit as nd_op_xlate (3ds/src/nd_op.c:20-79), as integer constant expressions. nd_op_xlate passes typed units through.
## Touch only
3ds/include/nd_op.h, 3ds/src/nd_op.c, 3ds/tools/nd_cmd_test.c.
## Exact edits
1. nd_op.h: `#define ND_OPB(w0) (((uint32_t)(w0) >> 24) & 0xffu)`. ND_X0 is a ternary chain on ND_OPB, one arm per case of nd_op_xlate, in the same order, with the same a/b expressions.
   - OTHERMODE: a = ((32 - field - len) & 0xff) | len << 8, with len = (w0 & 0xff) + 1 and field = (w0 >> 8) & 0xff.
   - MOVEWORD LIGHTCOL: test offset == 0, 24, ..., 192 by a chain (no `/`, no `%`).
   - Syncs (G_RDPLOADSYNC..G_RDPFULLSYNC), G_SPNOOP, G_NOOP: NDO_W0(NDO_MISC, NDO_MISC_NOP << 16).
   - G_ENDDL: NDO_W0(NDO_END, 0).
   - A typed or unknown word stays as it is.
   ND_X1 is the matching b chain.
2. nd_op.c nd_op_xlate, first line: `if (nd_op_typed(w0 >> 24)) { out[0] = w0; out[1] = w1; return 1; } /* HC4e1: a typed unit passes */`.
3. nd_cmd_test.c: for every op id in nd_op_xlate's switch, plus syncs, G_SPNOOP, G_ENDDL, a typed unit and an unknown op, use 8 values of w0/w1 (fixed seeds). Check ND_X0/ND_X1 == nd_op_xlate output. For syncs/NOOP/ENDDL, check the units above. Also check `static const uint32_t k[2] = { ND_X0(c0,c1), ND_X1(c0,c1) };` compiles.
## Done
nd_host PASS with the new cases. Build OK and memcheck OK. No Azahar start. Byte-equal by construction.
