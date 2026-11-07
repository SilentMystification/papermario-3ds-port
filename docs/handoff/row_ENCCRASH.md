# Row ENCCRASH: crash after a standing encwarp encounter with interp off

## Facts (BTRANS agent, 2026-11-06)
Scenarios btrans_stand_enc / btrans_stand_noint on branch btrans (7b08de8d4): with interp off, standing encwarp runs end rc 1 "crash symbols" after the window end, on the base (caplog only) and the fixed build. Not in the capture code (unverified).

## Route (oracle, code first, CLAUDE.md rule 5)
1. Take the symbolized PC/LR and the last 30 log lines from the AZT crash report ("crash symbols").
2. Read the faulting function, and the frame, game mode and battle-load phase from the pm_wd phase line.
3. Compare with docs/handoff/archive/diag_encwarp.md and diag_battle_start_crash.md (earlier encwarp and battle-start faults: warp-into-battle state and load order).
4. Name the field or pointer at the faulting line from the code before any further run. One confirming run at most. Then fix; gate: 3 standing encwarp runs interp off with no crash, the battle starts.

## Rules
Own worktree from nds1 (branch enccrash) or the btrans worktree if the scenarios are needed (cherry-pick 90157a60e). No attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. Delete this file in the fix commit.
