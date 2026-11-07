# Row ST: state and combine groups to nd_state.c and nd_comb.c

## Start state
nds1 5e5456e11 (WK-1 merged: nd_walk.c holds the pc loop, nd_int.h is the internal header). Branch st.

## Rows (plan_s10_path.md Stream ST)
ST-1 setters (group S) to nd_state.c. ST-2 combine (group C) to new nd_comb.c, 2 commits if over 150 lines. ST-4 RM: G_PM_RM back to G_DL plus the state-list memo (gate also battle_start byte-equal; grep gSPPmRenderMode|SPR_RM_LIST|G_PM_RM empty). ST-3 (cc_legacy, sync_legacy, set_leg delete) only after RECT-3 lands: skip it if not in nds1 and say so.

## Oracle decision ST-1
(1) Split the file, no host stubs. nd_state.c stays pure: NdWs nd_ws, nd_words, and the field decoders. It is host-built, so nd_core_test and nd_gbi_test link it unchanged. The target setters that call pm_gpu_* or pm_tex_* go to a new nd_set.c, compiled for the 3DS only: set_geom, set_fogcolor, set_scissor, set_primdepth, set_fillcolor, set_blendcolor, op_setprimcolor, op_setothermode, gbi_setenv, gbi_moveword, sync_render, and their globals. Stubs would grow with every move and hide link errors. Add nd_set.c to D5's module list; prototypes go in the nd_int.h S section.
(2) Ownership (D10):
- aud_prim_lod: delete the variable. It duplicates nd_ws.prim_lod (nd.h:341, "aud_prim_lod is the low byte"), so readers use (u8)nd_ws.prim_lod. op_setprimcolor writes only nd_ws.prim_lod. nd_comb_decide's dry-run save/set/restore (pm_gbi.c ~1131, ~1141) saves and restores nd_ws.prim_lod instead. Owner: nd_state (NdWs field); the restore is a temporary override, not a second writer.
- lt[] and lt_n: owned by group M (nd_mtx.c, with the light block, rec_lights and gbi_movemem lights). gbi_moveword's G_MW_NUMLIGHT case calls an M accessor `nd_light_count(unsigned w1)` that does lit_freeze plus the clamp to 7. Group S never writes lt_n directly.
Gate: host tests link and pass; mac_01 byte-equal; the grep for `aud_prim_lod` is empty.

## Rules
Plan: docs/handoff/plan_s10_path.md (What is left, Design, Streams, your Rows, Decisions D1 to D13). D12: each move is a pure cut and paste plus prototypes (nd_int.h, your section only), no logic change; byte-equal proves it. D10 globals: the writing module owns a global; a global read by 2 or more modules goes to NdWs or an nd_int.h extern. pm_gbi.c is shared: cut only your groups' functions and your ops' switch case bodies (WK owns the loop structure in nd_walk.c). Rebase on nds1 before each commit; after a rebase check `git grep '^<<<<<<<'` on every commit and fix markers before handing back. Investigate by code first (CLAUDE.md rule 5). Own worktree: git worktree add build3ds/wt/<id> -b <id> nds1; copy assets/us and ver/us/build; build there (PM_LANE=<id>); azt -Build <worktree 3dsx>. Move gate: spot_mac01_pic byte-equal to nds1 (3 shots), iwa_10 and battle_start by eye, gbi-phase us at mac_01 within +3 percent (opsplit), grep proof that the moved definitions are gone from pm_gbi.c. Commit on your branch, no attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. New scenarios: nds1 JSON as the base, add only yours. Status line in 3ds/logs/perf/status_<id>.txt before each start. Budget: reset and continue. Two tries per named cause. Hand back once with the branch head after your last row (or a failed gate); the orchestrator merges. Delete this file in your last commit.
