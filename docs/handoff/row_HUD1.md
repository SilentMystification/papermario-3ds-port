# Row HUD1: status bar scales 1.25 in wide mode

## Goal
In wide mode (400 px top screen, game x -40 to 360) the status bar keeps its 4:3 size: left box 174 px, icons and digits 1:1. User decision: scale the WHOLE status bar 1.25 (boxes, icons, digits, star power orbs, x and y), so it takes the same part of the screen as at 320 px. 4:3 mode stays pixel-equal.

## State (2026-11-07, folded into this session: user decision)
The HUD1 peer session stopped: its worktree (.claude/worktrees/agent-ada36f387b16d4402, branch hud1) has no commit, only an uncommitted src/inventory.c change, saved as build3ds/hud1_wip.diff (449 lines): start from it (git apply in your worktree, then check it against the design below). Work in a new worktree from nds1 (branch hud1b). The old "peer session" collision notes below no longer apply: commit your own scenario/bench entries if needed (nds1 JSON as the base). After HUD1 passes, do TX-4 final in the same branch: remove the G_PM_TEXI headers in src/hud_element.c (HT1, two places ~365-403 and ~477-600), then delete the G_PM_TEXI case (nd_walk.c ~175), gsSPPmTexInline and G_PM_TEXI in include/gbi_custom.h, and the tb_inline walker entry; gate: HUD by eye in yard, yard_wide, battle_wide; spot_mac01_pic byte-equal; grep `G_PM_TEXI|gsSPPmTexInline` empty.

## Start state (original)
Branch: nds1, last commit f90d823b2. You work in a git worktree on branch hud1 (not nds1). Do not touch 3ds/src/nd_core.c (peer session).

## Design (do this)
- One scale k = pm_wide ? 1.25 : 1.0 for the status bar. Native bar spans x 12 to 308 (174 + 122 px). Wide origin: x = -25 (160 + (12 - 160) * 1.25), so the bar spans -25 to 345, 15 px from each screen edge.
- Every offset inside the bar (drawPosX + n, drawPosY + n, the fixed x 207, 212, 256, 259) becomes origin + n * k. Use integer math: (n * 5) >> 2 in wide, no division, no double (docs/handoff/hw_guidelines.md).
- Boxes: draw_box widths 174 and 122 and heights 35 and 25 times k. Remove the pm_wide_right(122) - pm_wide_left(0) width.
- Icons and digits: hud_element_set_scale(id, 1.25f) once at status bar init when pm_wide (src/hud_element.c:2108). Check that the digit and "x" elements drawn by status_bar_draw_stat and status_bar_draw_number scale too.
- Star power orbs and shimmer (src/inventory.c near line 914, x + limit * 25 / 10): scale those offsets by k.
- Delete PM_HUD_FP_DX, PM_HUD_SP_DX, PM_HUD_COIN_DX (src/inventory.c:8-10) and their uses. The uniform scale keeps the native spacing.
- Keep the y anchor FULLY_EXTENDED_Y as is. Only the inner offsets and heights scale.

## Touch only
- Files: src/inventory.c. src/hud_element.c only if scaled elements do not draw. Nothing else.
- No collisions: a peer session works on nds1 in the main folder. Do not edit or commit 3ds/tools/azt_scenarios.json, 3ds/tools/bench_defs.json, 3ds/logs/perf/runs.jsonl, docs/handoff/INDEX.md or any 3ds/src/nd_* file. Commit only the files named above plus agent_hud.md and this row file. The orchestrator copies your runs.jsonl lines at merge.
- Symbols: update_status_bar, status_bar_draw_stat, status_bar_draw_number, drawPosX, FULLY_EXTENDED_Y, PM_HUD_*_DX in src/inventory.c. pm_wide_left, pm_wide_right in include/pm_wide.h (read only).

## Done criteria (all must pass; handback only then)
1. Build passes.
2. No bench covers these scenarios, so run the existing scenario directly (no new bench, no json edit): powershell -File 3ds/tools/azt.ps1 yard_wide -Row HUD1. The shot shows: both boxes 1.25 taller and wider, edge gap about 15 px left and right, HP, FP, star points, coins and orbs inside the boxes, no cut digits.
3. One 4:3 shot: powershell -File 3ds/tools/azt.ps1 yard -Row HUD1 (status bar unchanged from before, same positions).
4. Battle check: powershell -File 3ds/tools/azt.ps1 battle_wide -Row HUD1, the status bar in battle shows the same scaled layout.
Affected scenarios (no others): yard_wide, yard, battle_wide.

## Rules that apply
CLAUDE.md rules 5, 7, 9. No toggles, no stubs, no git push, no attribution. Commit on hud1 after each passed criterion. Never commit 3ds/src/map_restore_table.c.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=HUD1 sh 3ds/build.sh; then restore 3ds/src/map_restore_table.c (git checkout it in the worktree).
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget HUD1

## At the end
Write a STATE block (10 lines or fewer) at the top of docs/handoff/agent_hud.md: branch, last commit, done criteria, open items. Delete this row file in the commit that completes the row.
