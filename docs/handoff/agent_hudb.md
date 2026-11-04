# agent_hudb: HUD bars (row HUDB1), handoff

Branch hudb (from native 02e901772). Worktree C:\Users\Andrew\Documents\GitHub\papermario-3ds-port-hudb.
Azahar starts: 6 of 8 (3ds/logs/test_budget.csv, row HUDB1; the 4 starts of the native worktree were copied in).

## State
- Run-away now starts from the player menu state (pm_warp.c battle_hooks, debug switch `runaway`).
  It sets moveCategory, selectedMoveID, actionCommandMode = AC_MODE_LEARNED, then btl_set_state(BATTLE_STATE_RUN_AWAY).
  Per-frame auto flee contact at BEGIN_TURN removed (the menu never reaches BEGIN_TURN while it waits for input).
- The mash meter draws (mashlog, shot frame 222: meter and "Press repeatedly!" visible).
- texrlog now logs only changed early-return rects (pm_gbi.c texr_early).
- NOT committed: no sign-extension fix. The shot did not show a change.

## Proven (run 6, frame 206)
- draw_mash_meter first call: posX -48 (before the -28 shift). Fills: ul x 948, lr x 1008 and 1002.
  10-bit raw 948 = -76, 1008 = -16, 1002 = -22 (gDPFillRectangle fields, gbi.h:4325). The wrap is real.
- Both wrapped fills are entirely left of the screen (x -76..-16). Nothing is drawn in this run.
  The long horizontal line is NOT in this run's shot. Later mashlog calls use posX 32 and 50 (no wrap).
- A long line needs a wrapped left edge and a right edge at x >= 0 (pm_gpu_fill_rect swaps an inverted span, pm_gpu.c:2734).
  That needs posX - 28 < 0 with posX - 28 + width >= 0. Run 6 did not produce it.

## Open
1. Find the case that draws the long line: Bombette charge bar (action_cmd.c draw_mash_meter caller, posX -48 path) or another
   draw_mash_meter caller with negative posX. Shot must show the line before any fix.
2. Fix at the cause, once a shot shows it: sign-extend the 10-bit fields in gbi_fillrect (FILL_SX10, pm_gbi.c ~3384),
   same as gbi_texrect already does for 12-bit fields (`if (xl & 0x800) xl |= ~0xfff;`). Then shot again.
3. Enemy HP bar (battle_menu_bar): texrlog shows NO HES_HPBar texrect early return in run 6. The bar does not reach gbi_texrect
   through the early returns. Check its draw path (gbi_fillrect or a sprite path) before computing the rect.
4. Budget: 2 starts left for HUDB1. Use them only for open item 1 or the fix shot.

## Commands
- Build: PM_LANE=hudb PM_BUILD_CAP=1500 sh 3ds/build.sh (after py 3ds/tools/gen_map_restore.py; map_restore_table.c is copied from the main worktree, never committed).
  Do not start two builds at once: tar errors in the sync step.
- Run (run 6): PM_ROW=HUDB1 PM_LANE=hudb powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/run_azahar.ps1 -Debug "warp=kmr_03:0 story=ch0_toad_town battle battlef=100 battlemenu runaway fillog mashlog texrlog loglevel=debug shots shotwin=150:120" -Until "shot \d+ frame 268\b" -Timeout 60 -Force
- Shots: build3ds/lane-hudb/shots/NNNN_0_2.ppm (400 x 240 ppm, screen rotated). Shot k = frame 150 + 4k.
