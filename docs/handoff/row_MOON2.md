# Row MOON2: Moon Jump as one held press (user, 2026-11-06)

## What the user wants
One press, not repeated hops: while A stays held in the air, Mario keeps rising at the normal jump rise speed; when A is released he falls at the normal fall speed ("it should increase at the jump rate up and down"). Repeated presses in the air must NOT start new hops. The player keeps full air control (stick moves Mario as in a normal jump) during the moon jump.

## Facts (MOON row, commits 5f0fdd28b, 7d9e42a42, 48e63aadc)
- 7d9e42a42 restarts a full jump on each new A press (set_action_state(ACTION_STATE_JUMP)): the user does not want this.
- The first version forced gravityIntegrator[0] >= 4 every step while A was held: Mario rose, but update_player (src/77480.c) snaps him back to lastGoodPos when timeInAir > 100.
- Jump physics: initialize_jump (src/world/action/jump.c), GravityParamsStartJump (src/world/actions.c), phys_init_integrator_for_current_state (src/7BB60.c), timeInAir counted in action_update_jump and the falling action.

## Design
In pm_cheat_tick (3ds/src/pm_dbg_game.c), with Moon Jump on, while the action state is the jump (or fall) and A is held: keep the vertical speed at the jump START rise rate (the first-step velocity of GravityParamsStartJump; read it from the integrator after initialize_jump), hold timeInAir below the snap-back limit (reset it each step while rising), and keep the action state JUMP (not FALLING) so the normal air control code runs. When A is released: stop overriding; the normal jump/fall physics takes Mario down at the normal rate. No restart on a new press in the air (remove the 7d9e42a42 restart). Ground press: the normal jump starts as usual.
Gate (Azahar, scenario menu_moon_on_log changed): hold A for 60 scans then release: y rises at the jump rate (log: y per step about constant up), no snap-back line, after release Mario falls and lands; a second press in the air does nothing (no restart line); with the stick held right during the rise, x changes (air control). menu_moon_off unchanged.

## Rules
Files: 3ds/src/pm_dbg_game.c, own hunks of azt_scenarios.json and bench_defs.json. Lane menu. Build in a scratch tree from HEAD plus your diff; stage by blob; clean build of the committed tree. No attribution lines. Delete this file in the completing commit.
