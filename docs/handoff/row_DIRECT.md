# Row DIRECT: the game calls the native producers directly (no display lists)

## Goal
User goal: replace the N64 display list system. Today the game writes display lists and nd_walk_run (a dispatcher) reads them and calls
the native producers. DIRECT: the game code calls the native producers directly and writes no display lists; delete the dispatcher and
the list reads. Console data: the draw path is 79-94% of frame work; core 0 hot code about 23 KB vs 16 KB L1I; I-cache misses 33k-70k
per frame on the O3DS XL; the logic thread on core 1 waits for the core 0 walk (row C1WAIT line `core2 wait`).

## Start state
Worktree build3ds/wt/DIRECT, branch DIRECT from main (hash in the first commit). PM_ROW=DIRECT, PM_LANE=DIRECT. One Opus agent, no
subagents. Parallel rows on main touch the same area: BTLDIM (background.c, pm_tex.c key), CORE2I (nd_wq.c, pm_nusys.c, pm_main.c,
pm_logic.c, interp), KEYMISS (model/texture keys, after BTLDIM). Merge main often; resolve your own conflicts.

## Phase 1: plan (commit before code)
Write docs/handoff/plan_direct.md (add its INDEX.md line): read design_native_draw.md, plan_native_system.md, plan_s10_path.md,
plan_core2.md, frame_budget.md, hw_guidelines.md and the walker (nd_walk_run and the producers). The plan names: the producer API the
game calls, per content class (models, entities, sprites, HUD/rects, effects, messages, background, fog/shroud, battle UI); the thread
rule with the logic thread (the logic records native commands into the frame queue PmFrame instead of display lists, the walker on core
0 replays them); interp (CORE2I) and det/census paths; the order of slices (one class per slice, each removes its list path for that
class: record, check vs the old path byte-equal, switch, delete); the memory plan (O3DS XL margin 330 KB, no large static .bss); the
premise checks and gates per slice. Each slice is a commit.

## Gates per slice
nd_host PASS. Byte-equal against build-at base main on the scenes that use the changed class (spot_mac01_det, mdl5_iwa10_det,
battle_start_f182_cold, nok11_det 0000-0015, tour10_det, gameover_fps60 as relevant). One picture of the subject. memcheck OK. A console
prof bench list for the user (spot_mac01_prof, mac_00, sbk_56, hos_01) at the end of each group of slices. Full catalog only at epic end
with the user's approval.

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. No toggles: a finished slice replaces the old path in the same commit. No stubs. No git stash, no push,
no attribution. Never commit 3ds/src/map_restore_table.c. Report progress after each group of slices: SendMessage main "DIRECT <slice> <hash>".

## At the end
Delete this row file in the final commit. SendMessage main "DIRECT ready <hash>".
