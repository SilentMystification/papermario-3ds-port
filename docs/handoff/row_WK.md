# Row WK: walk, census, target and debug groups leave pm_gbi.c

## Goal
Stream WK of docs/handoff/plan_s10_path.md (read: What is left, Design, Streams, Rows WK-1 to WK-4, Decisions D1 to D13). WK-1 creates the skeleton the other streams (ST, TX, GM, LS) build on, so it lands first and fast.

## Start state
nds1 2ca8df902 (CORE-3 merged: nd_rect.c holds the G_TEXRECT/G_FILLRECT handlers; bridge and record layers deleted; pm_gbi_walk.h exposes walker helpers). Work in a new worktree: git worktree add build3ds/wt/wk -b wk nds1; copy assets/us and ver/us/build; build there (PM_LANE=wk); azt -Build <worktree 3dsx>; check each run's boot build line.

## Rows (in order, one commit each)
WK-1 SKELETON: nd_int.h (sections per module, empty except W), nd_walk.c with group W (the pc loop as nd_walk_run, stack, segments, host range, g1/g2 skip, cold bad-list handlers); pm_gbi_run becomes a one-line call. Decide with the existing pm_gbi_walk.h: fold it into nd_int.h (one internal header, D5). Gate: move gate (spot_mac01_pic byte-equal to nds1, iwa_10 and battle_start by eye, gbi-phase us within +3 percent at mac_01 via opsplit); grep proves the moved functions are gone from pm_gbi.c. Tell the orchestrator the WK-1 head at once (the other streams wait for it).
WK-2 CENSUS, WK-3 TARGET, WK-4 DEBUG: as the plan rows say.

## Rules
D12: each move is a pure cut and paste plus prototypes, no logic change (byte-equal proves it). Investigate by code first (CLAUDE.md rule 5). Commit on branch wk; take nds1 JSON as the base for any scenario; no attribution lines; never commit 3ds/src/map_restore_table.c; keep Azahar open. Status line in 3ds/logs/perf/status_WK.txt before each start. Budget: reset and continue. Two tries per named cause. Hand back after WK-1 with the head, then continue WK-2 to WK-4 when told. Delete this file in the WK-4 commit.
