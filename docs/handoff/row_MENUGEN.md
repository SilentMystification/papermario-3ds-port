# Row MENUGEN: General tab layout and Resolution Mode (user, 2026-11-09)

## User facts
User: "General tab rearranged to this:" Frame Skip / Resolution Mode (replaces Widescreen; three values: Native, Fullscreen,
Widescreen) / 60FPS / Return to Title Screen / Exit to Homebrew Menu / Close Game / (remove Back).

## Goal
General tab rows in this order: Frame Skip, Resolution Mode, 60 FPS, Return to Title Screen, Exit to Homebrew Menu, Close Game.
No Back row on the General tab (B still closes the menu). Resolution Mode cycles Native, Fullscreen, Widescreen (left/right and A).

## Start state
`sh 3ds/tools/wt_new.sh MENUGEN`, branch MENUGEN. PM_ROW=MENUGEN, PM_LANE=MENUGEN. One Haiku agent.

## Premise and its check (code read first)
Today: pm_options.c:389-390 builds the General rows (RW_FPS, RW_WIDE, RW_DBG + 7 Frame skip; mode adds RW_TITLE, RW_CLOSE, RW_EXIT);
:415 adds RW_BACK. RW_WIDE flips option 1 (pm_wide, nd_int.h:421) On/Off. Find by code read what "Native" and "Fullscreen" are:
the port may already have a 4:3 path (pm_wide 0) and a stretch path. Name each mode's file:line. If the port has only two modes
(4:3 and widescreen), hand back at once with the facts and ask what Native vs Fullscreen means. Do not invent a render mode.
Return to Title / Close / Exit show only when `mode` is set today: keep that rule; ask in the handback if the user may want them always.

## Touch only
3ds/src/pm_options.c, pm_options_ui.c, the options save (option 1 becomes a 3-value field; old saves map Off->Native or the 4:3
mode found above, On->Widescreen). No render code unless a named mode already exists and only needs the option value.

## Done criteria
1. nd_host PASS. Build OK. memcheck OK.
2. One Azahar run (scenario that opens the menu on the General tab, cap 15 s) with one shot: the six rows in the user's order,
   no Back row. A shot of each Resolution Mode value set shows the expected screen (3 shots, same run if the menu allows).
3. Commit; delete this row file in the completing commit. Budget: 4 Azahar starts.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
