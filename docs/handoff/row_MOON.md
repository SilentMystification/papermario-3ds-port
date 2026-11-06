# Row MOON: Moon Jump glitch (user report 2026-11-06)

## Bug
With Moon Jump on, pressing A while in the air makes Mario rise without end until the game snaps him back to his last position on the ground.
Code: 3ds/src/pm_dbg_game.c pm_cheat_tick (grep `CH6: Moon Jump`): while A is held during a jump or fall, gravityIntegrator[0] is forced to at least 4 and [1..3] to 0 every step, so the rise never ends; the game then treats Mario as out of bounds (or past a height limit) and restores the last safe ground position.

## Fix (design)
Classic moon jump: each NEW press of A in the air (rising edge: curButtons & BUTTON_A and not in prevButtons, or pressedButtons) restarts the jump arc: set the jump integrators to the values the game uses at a normal jump start (read the jump start in src/world/action/jump.c or the player action code: the initial gravityIntegrator values), and set the jumping flags as a jump start does. Between presses the normal gravity runs (no per-step override), so Mario rises one jump height per press and falls normally. No height limit is needed. First find the snap-back code (grep the last safe position: lastGoodPos or similar) and log when it fires, to confirm the cause.
Gate: scenario menu_moon_on changed: press A 3 times in the air (padscript, 20 frames apart): log shows 3 restarts, peak y about 3 normal jump heights above the ground, no snap-back line, Mario lands normally. menu_moon_off unchanged.

## Rules
Files: 3ds/src/pm_dbg_game.c, own hunks of azt_scenarios.json. Lane menu. Build in a scratch tree from HEAD plus your diff; stage by blob; clean build of the committed tree. azt -Build <lane> runs build3ds/lane-<lane>/pm_3ds.3dsx in the main folder. No git add -A, commit -a, reset, stash, checkout, restore. No push, no attribution. Delete this row file in the completing commit. Hand back with the gate lines.
