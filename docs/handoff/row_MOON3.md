# Row MOON3: moon jump hover by repeated A taps, slower rise

## User facts (2026-11-08, console)
"Moon jump isnt properly implemented, now we can only jump once and cant raise again while in the air. We should be able to hover by tapping A a few times. Decrease the speed that we raise up as well, its too fast right now."
Current code: 3ds/src/pm_dbg_game.c:226-255 (MOON2): only while A is HELD in ACTION_STATE_JUMP does Mario rise, at GravityParamsStartJump[0]; "a new press in the air does nothing" (the comment says so: that is the bug now).

## Goal
With the Moon Jump cheat on:
- Each A press (rising edge of BUTTON_A in ps->pressedButtons or curButtons edge) while Mario is in the air (jumping OR falling) gives an upward push, so tapping A a few times keeps him hovering or climbs slowly. If the action is a fall, put it back to the jump action through the game's own action call (set_action_state(ACTION_STATE_JUMP) or the call the jump code uses) so air control and landing work.
- Holding A still rises, but at a slower rate: one constant MOON_RISE (start at 0.5 x GravityParamsStartJump[0]; one constant, no switch), the same for the tap push.
- On release the normal arc resumes (keep the MOON2 release code).
- Cheat off: no change at all.
Update the comment block so it says what the code does.

## Start state
Worktree build3ds/wt/moon3, branch moon3 from nds1 (current). Copy assets/us and ver/us/build into it.

## Touch only
3ds/src/pm_dbg_game.c (the moon jump block). A scenario moon_tap (azt_scenarios.json + bench_defs.json, same commit): field map, `moonjump` switch, padscript: jump, then tap A 5 times 10 frames apart; until a log line after the last tap.

## Done criteria
1. Build; nd_host PASS.
2. moon_tap log: `cheat: moon jump` y values rise after each tap, also after a fall started (y goes down, then up again after a tap); peak climb per frame lower than the old rate (log the y step).
3. One shot during the hover shows Mario in the air above the floor.
4. spot_mac01_det byte-equal (cheat off).
Affected scenarios (no others): moon_tap, spot_mac01_det.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=moon3 PM_ROW=moon3 on every build.sh and bench call. No git stash. No toggles, no stubs.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=moon3 PM_LANE=moon3 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=moon3 PM_LANE=moon3 python 3ds/tools/bench.py run <bench> --row moon3 --lane moon3 --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "moon3 ready <hash>".
