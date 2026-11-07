# agent_hud: HUD1 status bar wide scale, then TX-4 final

STATE (2026-11-07)
- Branch hud1b (worktree build3ds/wt/hud1b), rebased on nds1; last commit: see git log (TX-4 final on top of 462becdfa).
- HUD1 code (src/inventory.c): done. battle_wide 1.25x bar: pixel scan ok (criterion 4).
- Open: yard_wide_hud and yard_hud (new scenarios, uishow=coins) show NO bar at shots 200 to 219. Hook sets alwaysShown at f200 (pm_warp.c), but no "hud f" debug line at f%4==0 in the log: draw_status_ui apparently not reached in the yard world. Next: trace the yard world UI path (state_drawUI_world caller, statusBar->disabled).
- TX-4 final: G_PM_TEXI gone from src, include, nd_walk, nd_tex (tb_inline), nd_dlc comment. Grep empty. spot_mac01 3 of 3 byte-equal. battle_wide HUD region same, scene differs in a small non-HUD area (frames 11, 12).
- Not done: HUD1 criteria 2 and 3 (yard), so row_HUD1.md is not deleted.
