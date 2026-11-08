# Row GREY: top screen stays colour when the menu opens after quick Select presses (MN2)

## User facts (2026-11-08)
Trigger: on the console, any scene, press Select quickly several times (menu open, close, open). First wrong frame: the menu is open (bottom screen) but the top screen shows the colour game picture. Expected: the top screen is grey while the menu is open (MN2). Seen on the console only (Azahar not checked). Reproducing scenario: add one first (menu_seltap_det, below).

## Goal
Every menu open shows a grey top screen, also on a quick reopen.

## Start state
Worktree build3ds/wt/grey, branch grey from nds1 4b8f2896c (+ this row). Copy assets/us and ver/us/build into it. Row CFG-1/CFG-2 runs in parallel in pm_main.c (lines ~700-730) and pm_gpu.c frame_draw (~1708-1722): do not edit those ranges.

## Oracle trace (2026-11-08, nds1 110ac50b3)
Select closes the menu (pm_options_ui.c:130 pm_menu_set(0)). The game draws frames; each ends with C3D_FrameEnd(GX_CMDLIST_FLUSH) (pm_gpu.c ~2395), which queues the display transfer and swap and returns. Select again: pm_input.c:263 select_pending, the menu opens 5 scans later (pm_input.c ~333). The same loop pass reaches pm_main.c:422 (menu_frozen 0) and calls pm_gpu_menu_grey() at once. pm_gpu_menu_grey (pm_gpu.c:1626-1642) waits with gspWaitForPPF() (1632), which can return on the transfer of a still-queued frame before citro3d's swap. Then (a) the queued transfer overwrites the grey buffer with colour, or (b) the grey swap (1640) runs first and citro3d's queued swap flips back to colour. The later presents (pm_gpu_menu_present 1608-1620) run only when menu_dirty.

## Premise and its check
Premise: at a bad pause the GX queue still holds the last game frame. Check: add the debug line at pm_main.c:422 `menu: grey qpend %u lastdraw %d frame %u` (qpend = gxCmdQueue numEntries - curEntry, read as pm_gx_room does, pm_gpu.c ~2042-2053; lastdraw = draw flag of the previous loop pass). In the repro: qpend > 0 (or lastdraw 1) on the opens where the top stays colour, 0 on good opens. A bad open with qpend 0: the premise is wrong, hand back with the lines.

## Reproduction (add first, one commit with scenario + bench)
- Debug switch `seltap=<start>:<n>` in pm_input.c, injected into d before line 263 (like taptest): Select for one scan at start + 12k and start + 12k + 6, k < n. Log `seltap: done` after the last cycle.
- Shot of the DISPLAYED top buffer one vblank after each pause: `top_after%04u` via pm_gpu_shot_top on the frozen loop pass; read the active top framebuffer index from the GSP framebuffer info in shared memory (not the back buffer). shotgrey's top_grey shot is not proof (it is grey in the bug too).
- Scenario menu_seltap_det: `warp=mac_01:0 pos=0:0:200 seltap=400:6 shotgrey loglevel=debug det fixedseed fast fs1off`, cap 20 s, until `seltap: done`. Add to azt_scenarios.json and bench_defs.json in the same commit.
If Azahar never shows a colour top_after shot (emulator timing differs from the console), record that, keep the qpend check, and continue with the fix; the console check is the final gate (orchestrator asks the user).

## Fix (one point)
pm_main.c:422: replace the direct pm_gpu_menu_grey() with a forced menu present: set menu_dirty = 1 (small exported setter in pm_gpu.c) and call pm_gpu_menu_present(), which starts with C3D_FrameBegin(C3D_FRAME_SYNCDRAW) (waits for the previous frame's queue, transfer and swap) and greys after its own transfer (1620). No second path stays.

## Gate
Build; menu_seltap_det: `menu: game paused` count = 12 (2n), every top_after shot grey, qpend 0 at every pause line after the fix; spot_mac01_det byte-equal to base (no game picture change). 2 tries per named cause.

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. PM_ROW=GREY PM_LANE=grey on every build.sh and bench call. Budget 8 starts. No git stash. Never commit runs.jsonl or 3ds/src/map_restore_table.c. Wait for runs in the foreground. The fix commit deletes this row file.
