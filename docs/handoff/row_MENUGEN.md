# Row MENUGEN: General tab layout and Resolution Mode (user, 2026-11-09)

## User facts
User: "General tab rearranged to this:" Frame Skip / Resolution Mode (replaces Widescreen; three values: Native, Fullscreen,
Widescreen) / 60FPS / Return to Title Screen / Exit to Homebrew Menu / Close Game / (remove Back).

## Goal
General tab rows in this order: Frame Skip, Resolution Mode, 60 FPS, Return to Title Screen, Exit to Homebrew Menu, Close Game.
No Back row on the General tab (B still closes the menu). Resolution Mode cycles Native, Fullscreen, Widescreen (left/right and A).

## Start state
`sh 3ds/tools/wt_new.sh MENUGEN`, branch MENUGEN. PM_ROW=MENUGEN, PM_LANE=MENUGEN. One Haiku agent.

## Mode definitions (user answers, 2026-11-09; premise check done by the first agent)
- Native = today's 4:3 path (pm_wide 0: pm_gpu.c:172-173 else branch, :941, :2127; 40 px bars).
- Widescreen = today's pm_wide 1 (wider 3D projection in cam_main.c:84, cam_math.c:398 and others; HUD stays 4:3).
- Fullscreen = NEW: the Native picture (pm_wide 0, same projection, same 2D) stretched 1.25x in X over 400 px, HUD and menus
  included, no bars. Do it only in the final viewport/scissor of pm_gpu.c (one new flag, e.g. pm_stretch, beside pm_wide at
  :153; the x scale at :172-173 and the scissor at :941-950 and :2127 for every draw). Do not change pm_wide or any src/ file.
- Return to Title / Exit / Close: keep the `mode` rule (only in the in-game Select menu).
- Old saves: wide=0 -> Native, wide=1 -> Widescreen.

## Part 2 (user, 2026-11-09, same row; do after part 1 is committed)
Menu behaviour:
- B goes back one level but never closes the Select menu. Only Select closes it. The help text at the bottom says Select, not X.
- The warp warning shows once per session (first menu open after boot), not at every menu open.
- Remove the Back row from every menu and sub-page (B goes back).
Debug tab, in this order:
1. Warp Area
2. Story Progress
3. Debug HUD
4. Logging (pulled out of its sub-page to a top row of the Debug tab)
5. Profiling (pulled out of the same sub-page)
6. Disable Fog, 7. Disable Light, 8. Texture Filtering: the Render options as top rows with these names. The user reports they
   seem to do nothing. Code read first: name for each the flag (file:line) and every reader of it in the current native draw path
   (nd_*.c, pm_gpu*.c). A flag with no reader in the draw path, or one read only at init, is a bug: wire it so a change in the menu
   takes effect at the next frame (hot toggle). Show each with one shot pair (off, on) in one Azahar run.
9. Fixed Seed (pulled out of the Game sub-page)
10. Start a Battle (renamed from Boss and Battles), a folder with two rows: Enemy Encounters, Boss Battles.
Rows not named here that sit in those sub-pages today: keep them in their sub-page; list them in the handback.
Done for part 2: one Azahar run with shots of the Debug tab, one sub-page without a Back row, the B/Select behaviour (log lines),
and the render shot pairs. Budget for the whole row: 8 Azahar starts.

## Touch only
3ds/src/pm_options.c, pm_options_ui.c, the options save (option 1 becomes a 3-value field; old saves map Off->Native or the 4:3
mode found above, On->Widescreen). pm_gpu.c only for the Fullscreen flag above; for part 2, the readers of the render flags (nd_*.c, pm_gpu*.c) only where a flag is not wired.

## Done criteria
1. nd_host PASS. Build OK. memcheck OK.
2. One Azahar run (scenario that opens the menu on the General tab, cap 15 s) with one shot: the six rows in the user's order,
   no Back row. A shot of each Resolution Mode value set shows the expected screen (3 shots, same run if the menu allows).
3. Commit; delete this row file in the completing commit. Budget: 4 Azahar starts.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
