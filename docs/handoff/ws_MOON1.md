# ws_MOON1: the super jump and ground pound window has priority over the moon jump (Haiku). Runs in parallel with all pages.
Objective: the moon jump (cheat, 3ds/src/pm_dbg_game.c:319-359, pm_cheat_tick after the game step, pm_warp.c:1999) never acts while the game's
midair window can open. Counters C1..C12: no change. Air control (user item 3a) is not in this page.
FACT window (src/7BB60.c:1092-1111 check_input_midair_jump): peakJumpTime >= 6, timeInAir < 18, A pressed, boots Super (1) or Ultra (2).
FACT peakJumpTime = timeInAir at the apex (src/7E9D0.c:243-246). FACT conflict: the moon hold sets timeInAir = 1 (pm_dbg_game.c:347), so a
later apex gets peakJumpTime below 6 and the window never opens. A moon tap before the window also sets timeInAir = 1 (:341-347).
Rule: the moon jump acts only when bootsLevel < 1 (no window) or timeInAir >= 18. The hold keeps timeInAir at 18 (window closed, below the
lastGoodPos reset at > 100, comment :323).
Files: 3ds/src/pm_moon.h (new), 3ds/src/pm_dbg_game.c, 3ds/tools/pm_moon_test.c (new), 3ds/tests/host.d/moon.sh (new). One commit.
## Edit 1, new 3ds/src/pm_moon.h:
    /* MOON4: the game's midair window (src/7BB60.c:1095-1097) has priority. The moon jump acts only when no window can open in this air time. */
    #pragma once
    #define PM_MOON_WIN_END 18  /* the window is timeInAir < 18 */
    #define PM_MOON_AIR_TIME 18 /* the moon hold keeps timeInAir here */
    static inline int pm_moon_allowed(int bootsLevel, int timeInAir) { return bootsLevel < 1 || timeInAir >= PM_MOON_WIN_END; }
## Edit 2, pm_dbg_game.c. Add `#include "pm_moon.h"` after line 302. Then replace lines 331-333. Before:
        const int pushed = moon && air && (ps->pressedButtons & BUTTON_A) != 0;
        int rising;
        if (pushed && ps->actionState != ACTION_STATE_JUMP) {
After:
        const int ok = moon && air && pm_moon_allowed(p->bootsLevel, ps->timeInAir); /* MOON4: the midair window first */
        const int pushed = ok && (ps->pressedButtons & BUTTON_A) != 0;
        int rising;
        if (ok && held && ps->actionState != ACTION_STATE_JUMP) { /* MOON4: a hold after the window enters JUMP as a tap does */
Line 341 before: `rising = moon && air && held && ps->actionState == ACTION_STATE_JUMP;`  after: `rising = ok && held && ps->actionState == ACTION_STATE_JUMP;`
Line 347 before: `ps->timeInAir = 1;`  after: `ps->timeInAir = PM_MOON_AIR_TIME; /* MOON4: the window stays closed */`
Edit the comment block :319-325 in place: replace "timeInAir is held at 1" by "timeInAir is held at 18 (MOON4)".
## Edit 3, new 3ds/tools/pm_moon_test.c (form of 3ds/tools/pm_vpsnap_test.c: prints one line, returns 1 on a fault):
    #include <stdio.h>
    #include "pm_moon.h"
    int main(void) {
        static const int c[][3] = { {1,17,0}, {1,18,1}, {2,5,0}, {2,40,1}, {0,1,1}, {-1,1,1} }; /* boots, timeInAir, expected */
        for (unsigned i = 0; i < sizeof c / sizeof c[0]; i++)
            if (pm_moon_allowed(c[i][0], c[i][1]) != c[i][2]) { printf("moon FAIL case %u\n", i); return 1; }
        if (PM_MOON_AIR_TIME < PM_MOON_WIN_END || PM_MOON_AIR_TIME > 100) { printf("moon FAIL airtime\n"); return 1; }
        printf("moon ok 7/7\n"); return 0;
    }
## Edit 4, new 3ds/tests/host.d/moon.sh (form of 3ds/tests/host.d/hwfix.sh):
    #!/bin/sh
    # MOON4: host test of pm_moon_allowed. A seeded fault (the window check removed) must fail at case 0.
    mkdir -p /tmp/moon_seed
    sed 's/timeInAir >= PM_MOON_WIN_END/1/' 3ds/src/pm_moon.h > /tmp/moon_seed/pm_moon.h
    gcc -m32 -Wall -I /tmp/moon_seed -o /tmp/moon_seed/t 3ds/tools/pm_moon_test.c || exit 1
    got=$(/tmp/moon_seed/t)
    if [ "$got" != "moon FAIL case 0" ]; then echo "moon seed FAIL: got [$got]"; exit 1; fi
    gcc -m32 -Wall -I 3ds/src -o /tmp/moon_t 3ds/tools/pm_moon_test.c || exit 1
    /tmp/moon_t || exit 1
## Gates (1 build, 0 Azahar runs)
1. `sh 3ds/tests/host.d/moon.sh` prints `moon ok 7/7`. 2. `sh 3ds/tools/nd_host_test.sh`: PASS. 3. One build: rc 0, memcheck OK.
Fix a failing test: edit only the lines of Edits 1-4. Stop: gates 1-3 pass, one commit, delete this page in the commit.
Hand-back: build3ds/out/handback_MOON1.txt (commit, moon line, host, build rc).
