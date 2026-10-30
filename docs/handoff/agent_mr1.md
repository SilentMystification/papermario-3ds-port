# agent_mr1: MR1 state (G_PM_MODEL, model group header)

Plan: docs/handoff/plan_mr1.md. Rules: CLAUDE.md 2, 5, 7; hw_guidelines.md.

## Code (uncommitted, in the tree)
- include/gbi_custom.h: G_PM_MODEL 0x0C.
- src/model.c appendGfx_model: header op before the pipe sync that starts the model; n patched at the end of the function (TARGET_3DS only).
- 3ds/src/pm_gbi.c: gbi_movemem, gbi_popmtx, gbi_moveword (moved case bodies); mg_run (runs the group words with the walker handlers); case G_PM_MODEL in the walker.

## Test starts (budget: MR1base 8, MR1 8; cut to mac01t and yard by the orchestrator)
- MR1base mac01t run 1 and run 2 (HEAD lane conlb1b2): tot ops 1215 both; top shots 2 of 2 equal.
- MR1 mac01t run 1 (lane mr1): tot ops 860 (down 355, gate 300 passes); top shots 2 of 2 equal to HEAD.
- MR1base yard (HEAD lane conlb1b2): run made before the cut. It is an overrun of the cut (HEAD runs: 3 of 2).
- Overrun before the cut (MR1base, same lane): forest_mim, sewer, ruins, sun_sbk, volcano. Not compared with MR1.
- MR1 yard run 1 (lane mr1): measures yard top shot against the HEAD yard shot (byte-equal test of the G_PM_MODEL path on a second map, the cut scope). Code path: the same header and mg_run path as mac01t. No other run answers it: yard is the second scene of the cut set, and its HEAD shot already exists.
