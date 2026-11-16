# ws_MOON7 (Haiku): moon jump with no stop at the apex, spin jump kept
Designer to ask: the oracle (SendMessage to oracle). Branch from main. Worktree build3ds/wt/MOON7, branch MOON7.
PM_ROW=MOON7 PM_LANE=MOON7 on every bench call. No counter (C1..C12) moves.
Causes, FACT (code read, USER FACT build 5852: "double tap gives no spin, hesitation when held"):
1. Stop: pm_moon.h:7 lets the cheat act only after the apex (peakJumpTime > 0). The held arc slows to 0 (apex, src/7BB60.c:258-262),
   falls 1 step, then the cheat sets the speed to 7.88 (pm_dbg_game.c:346). That is the hesitation.
2. No spin: the spin needs A held through the apex, then a new press (7BB60.c:1095: peakJumpTime >= 6, timeInAir < 18). After the apex
   the cheat puts Mario back in JUMP (pm_dbg_game.c:336-342). The release then gives HOP (7BB60.c:243-247). HOP runs initialize_jump
   (actions.c:151, jump.c:18-19): timeInAir and peakJumpTime go to 0, the new peak is 1 to 3, so the window stays closed.
FACT: in the game a fast tap (release before the apex) also gives no spin: HOP resets peakJumpTime the same way.
State machine (pm_moon_tick, block A), per air time:
- HOLD: A held since a JUMP take-off. The game arc runs unchanged until its next speed is below 2.0 (step 5 of a held jump).
  Then the speed is 2.0 and grows by 1.0 a step up to 7.88 (half the jump start). No stop, no fall.
- RELEASE while the cheat lifts: fall of a normal apex (fall integrator, FALLING, peakJumpTime = timeInAir), no HOP.
  A press before timeInAir 18 gets the spin (super boots) or the tornado jump (ultra boots), as in the game.
- A release before the lift is the game's own jump (HOP). A later press or hold waits until timeInAir >= 18 (normal boots: no wait).
- STEER: the stick steers only while A is held, the cheat lifts, and timeInAir >= 18 (window over).
Files: 3ds/src/pm_moon.h, 3ds/src/pm_dbg_game.c, 3ds/tools/pm_moon_test.c, 3ds/tests/host.d/moon.sh. Do not edit any other file.
## Edit shape (example, pm_dbg_game.c:303)
Before: `extern f32 GravityParamsStartJump[]; /* src/world/actions.c */`
After:  `extern f32 GravityParamsStartJump[], GravityParamsStartFall[]; /* src/world/actions.c */`
## Edits: blocks A to D of docs/handoff/ws_MOON7_code.md, exact text.
## Loop (1 build, at most 2 Azahar starts)
1. Do blocks A to D. Run `sh 3ds/tools/nd_host_test.sh > build3ds/out/moon7_host.txt 2>&1`.
   Pass: `ND HOST TESTS PASS` and `moon ok 10/10`. Commit `MOON7 1: moon jump lift with no apex stop, release keeps the spin window`.
2. Run `sh 3ds/build.sh > build3ds/out/moon7_build.txt 2>&1`. Pass: rc 0 and memcheck margin above 0.
3. Gate (the cheat is off in this bench, 3ds/tools/bench.d/HOSTCORE2.json): `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`,
   then `python 3ds/tools/bench.py run spot_mac01_drec --row MOON7 --why "MOON7 gate"`,
   then `python 3ds/tools/bench.py cmp spot_mac01_drec --row MOON7 --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal.
## Fix scope
A failing host test or seed: compare your files with the blocks byte for byte (LF line ends). Do not change an expected text or a seed.
A compile error in pm_dbg_game.c: keep the (s16) casts and the extern line. Do not edit src/.
## Stop and hand-back
Stop: host PASS, build rc 0, gate 1/1 byte-equal. Final commit `MOON7 2: gate` with the values. It deletes ws_MOON7.md and ws_MOON7_code.md.
Hand-back: build3ds/out/handback_MOON7.txt (line 1 commit hash, then host line, build number, memcheck margin, gate value).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
User check on the console: hold A: Mario rises with no stop. Hold A past the top, release, press A: spin jump. Fast tap-tap: as the game.
