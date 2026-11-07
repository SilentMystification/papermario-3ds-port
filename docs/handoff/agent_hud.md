# agent_hud: HUD1 status bar wide scale, then TX-4 final

STATE (2026-11-07)
- Branch hud1b (worktree build3ds/wt/hud1b), base nds1 41b41a747, last commit 462becdfa (HUD1 inventory.c).
- Done: HUD1 code (wip diff applied, PM_HUD_*_DX deleted). Build passes. battle_wide: bar scaled 1.25 (pixel scan: boxes, top gap 15 px, contents inside).
- Open: yard_wide and yard shots show no status bar at frames 200 to 205 (bar auto-hides in world, inventory.c hidden/showTimer). Criteria 2 and 3 not verified. Needs a scenario that keeps the bar up (json edit: user decision).
- Open: TX-4 final (hud_element.c HT1 headers, then G_PM_TEXI delete in nd_walk.c, gbi_custom.h, tb_inline) not started; waits on HUD1 criteria.
- Next: user decides on the yard scenario; then gates per row_HUD1.md.
