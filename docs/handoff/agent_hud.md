# agent_hud: HUD1 wide status bar, TX-4 final

STATE (2026-11-07)
- Branch hud1b (worktree build3ds/wt/hud1b). Head: see git log (HUD1 done, TX-4 final done).
- HUD1 gates: build ok; yard_wide_hud (criterion 2) bar 1.25x, 15 px top gap, contents inside; yard_hud nowide (criterion 3) 4:3 bar native; battle_wide (criterion 4) 1.25x.
- Bar in world needs uishow=bar (pm_warp.c, open_status_bar_slowly at f200). uishow=coins keeps the bar out (coin counter only). Hook frame f200 is about pm_frame_count 249 in yard; bar full height at ~270.
- nowide debug switch (pm_gpu.c) forces 4:3 when options.ini has wide=1.
- TX-4 final: G_PM_TEXI gone; grep empty; spot_mac01 3 of 3 byte-equal to TX-1 base; battle_wide HUD equal.
