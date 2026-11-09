# Row WARPQ: a warp during a map transition is queued, not refused

## User facts
User (2026-11-09, O3DS XL, build de415917f): "Warp not possible in game mode 5" when a warp is chosen in the middle of a map
transition (door or loading zone). "The warp should unpause and queue up the map change so we don't get a block like this."

## Start state
Worktree from `sh 3ds/tools/wt_new.sh WARPQ`, branch WARPQ. PM_ROW=WARPQ, PM_LANE=WARPQ. One Haiku agent.

## Code facts
- 3ds/src/pm_warp.c:1108 pm_warp_live: battle and pause go to bypass_begin; everything else to pm_warp_live_now (:1112), which accepts
  only GAME_MODE_WORLD (4), TITLE_SCREEN, INTRO, DEMO and refuses the rest with pm_log_w + pm_menu_msg (:1115-1116).
- Mode 5 is GAME_MODE_CHANGE_MAP (src/game_modes.h:23). Other transition modes: ENTER_WORLD 7, END_BATTLE 9, UNPAUSE 11.
- The same refusal exists for a boss launch (:1170).

## Fix
1. In pm_warp_live (and the boss launch path), when the mode is CHANGE_MAP, ENTER_WORLD, END_BATTLE or UNPAUSE: store the request
   (choice, entry; or boss id) in one pending slot (a newer request replaces it), close the Select menu and unpause the menu as a
   finished warp does, log "warp: queued in game mode <m>", and return started.
2. Each frame (the existing per-frame warp hook; find where pm_warp_pending is read), when the mode is GAME_MODE_WORLD again and the
   queued slot is set: run pm_warp_live_now with it, clear the slot, log "warp: queued warp runs".
3. Battle and pause keep bypass_begin. Other modes (game over, file select, language) keep the refusal message.

## Gates (REPRO first)
- New scenario warpq_door + bench warpq_door_pic (same commit): from mac_01, walk into a door or loading zone, open Select during
  the transition and choose a warp (padscript; the agent picks the frames from the log "game mode 5"). Run on the base first: the log
  shows "warp: not possible in game mode 5" (repro). Fixed build: "warp: queued in game mode 5", then "warp: queued warp runs" and
  "warp: live to choice ..." and the shot shows the destination map with no menu. Until-line: the destination map line. Cap 25 s.
- nd_host PASS. memcheck OK. No byte gates (no draw code change).
