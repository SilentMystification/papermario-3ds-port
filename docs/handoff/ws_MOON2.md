# ws_MOON2: air control in a moon jump only (Haiku). Start after Merge MOON1: this page edits the MOON1 lines of pm_dbg_game.c, pm_moon.h, moon.sh.
Objective (user item 3a): while Mario is in the air in a moon jump, the stick sets his direction and speed. Counters C1..C12: no change.
A moon jump = the cheat is on and this air time had a moon push or hold (`pushed` or `rising`). Cheat off or no push or hold: no change.
FACT air move (src/7BB60.c:607-650): the step moves Mario by curSpeed along targetYaw plus stick magnitude * 0.03125 along the stick, capped at
runSpeed in the air. The jump keeps curSpeed and targetYaw of the take-off (no writer in src/world/action/jump.c). pm_cheat_tick runs after the step
(3ds/src/pm_warp.c:1999), so a value it writes moves Mario in the next step. FACT stick magnitude 0..70 (player_input_to_move_vector, src/7B440.c:143-162).
Files: 3ds/src/pm_moon.h, 3ds/src/pm_dbg_game.c, 3ds/tools/pm_moon_test.c, 3ds/tests/host.d/moon.sh. One commit.
## Edit 1, pm_moon.h: add at the end of the file
    #define PM_MOON_STICK_MAX 70.0f /* player_input_to_move_vector clamps the stick magnitude here (src/7B440.c:147) */
    /* MOON5: the air speed of a moon jump: the run speed times the stick share */
    static inline float pm_moon_air_speed(float mag, float run) { if (mag > PM_MOON_STICK_MAX) mag = PM_MOON_STICK_MAX; return run * mag * (1.0f / PM_MOON_STICK_MAX); }
## Edit 2, pm_dbg_game.c, the moon block of pm_cheat_tick. Before: `        static int moon_held;`
After: `        static int moon_held, moon_air; /* MOON5: moon_air = this air time had a moon push or hold */`
Before (the last line of the block): `        moon_held = rising;`  After:
        moon_held = rising;
        if (!air) moon_air = 0; else if (pushed || rising) moon_air = 1;
        if (moon && air && moon_air) { /* MOON5 (user 3a): the stick steers a moon jump; a normal jump is not changed */
            f32 yaw, mag;
            player_input_to_move_vector(&yaw, &mag);
            if (mag > 0.0f) { ps->targetYaw = yaw; ps->curSpeed = pm_moon_air_speed(mag, ps->runSpeed); }
        }
Add one sentence at the end of the MOON3 comment block: "MOON5: in a moon jump the stick sets targetYaw and curSpeed (run speed times the stick share)."
## Edit 3, pm_moon_test.c: before the line `printf("moon ok 7/7\n"); return 0;` add
        { static const float s[][3] = { {70.f, 4.f, 4.f}, {35.f, 4.f, 2.f}, {140.f, 4.f, 4.f} }; /* mag, run, expected */
          for (unsigned i = 0; i < 3; i++) { float d = pm_moon_air_speed(s[i][0], s[i][1]) - s[i][2]; if (d > 1e-4f || d < -1e-4f) { printf("moon FAIL speed %u\n", i); return 1; } } }
Then change `moon ok 7/7` to `moon ok 10/10`.
## Edit 4, moon.sh: before the line `gcc -m32 -Wall -I 3ds/src -o /tmp/moon_t 3ds/tools/pm_moon_test.c || exit 1` add
    sed 's/if (mag > PM_MOON_STICK_MAX) mag = PM_MOON_STICK_MAX; //' 3ds/src/pm_moon.h > /tmp/moon_seed/pm_moon.h
    gcc -m32 -Wall -I /tmp/moon_seed -o /tmp/moon_seed/t 3ds/tools/pm_moon_test.c || exit 1
    got=$(/tmp/moon_seed/t)
    if [ "$got" != "moon FAIL speed 2" ]; then echo "moon seed2 FAIL: got [$got]"; exit 1; fi
## Gates (1 build, 0 Azahar runs)
1. `sh 3ds/tests/host.d/moon.sh` prints `moon ok 10/10`. 2. `sh 3ds/tools/nd_host_test.sh`: PASS. 3. One build: rc 0, memcheck OK.
Fix a failing test: edit only the lines of Edits 1-4. MOON1 lines differ from the Before text: hand back the diff, change nothing.
Stop: gates 1-3 pass, one commit, delete this page and its INDEX.md entry in the commit.
Hand-back: build3ds/out/handback_MOON2.txt (commit, moon line, host, build rc). The user tests the feel on the console.
