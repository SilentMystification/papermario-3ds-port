# ws_MOON6 (Haiku): the moon jump acts again with super and ultra boots
Branch from main. Worktree build3ds/wt/MOON6, branch MOON6. PM_ROW=MOON6 PM_LANE=MOON6 on every bench call. No counter (C1..C12) moves.
Cause, FACT (oracle code read): pm_moon_allowed (pm_moon.h:5) lets the moon jump act with bootsLevel >= 1 only at timeInAir >= 18.
A flat-ground jump with A held lands at timeInAir 15 (FACT: arithmetic over src/world/actions.c:45-46 and src/7BB60.c:202-270, 448-461).
So with super or ultra boots the cheat never acts. The old moon.sh tested the gate table only, not the jump length.
Fix: the moon jump also acts after the apex (peakJumpTime > 0, set at src/7E9D0.c:245, 0 at each take-off and landing: jump.c:19, 7BB60.c:388).
The hold caps timeInAir at 18 and does not raise it, so the window (7BB60.c:1095-1097) stays open from the apex to 18 as in a normal jump.
A press in the window goes to the game first (update_player calls check_input_midair_jump, src/77480.c:733, before pm_cheat_tick).
Files: 3ds/src/pm_moon.h, 3ds/src/pm_dbg_game.c, 3ds/tools/pm_moon_test.c, 3ds/tests/host.d/moon.sh. Do not edit any other file.
## Edit 1: blocks A, B and C of docs/handoff/ws_MOON6_code.md, exact text (whole files).
## Edit 2: 3ds/src/pm_dbg_game.c (3 lines)
Before (line 333): `const int ok = moon && air && pm_moon_allowed(p->bootsLevel, ps->timeInAir); /* MOON4: the midair window first */`
After: `const int ok = moon && air && pm_moon_allowed(p->bootsLevel, ps->timeInAir, ps->peakJumpTime); /* MOON6: the window opens at the apex */`
Before (line 350): `ps->timeInAir = PM_MOON_AIR_TIME; /* MOON4: the window stays closed */`
After: `ps->timeInAir = (s16)pm_moon_air_cap(ps->timeInAir); /* MOON6: a normal count, capped at 18 */`
Before (line 324, part): `and timeInAir is held at 18 (MOON4: the window stays closed; update_player`
After: `and timeInAir is capped at 18 (MOON6: it counts as in a normal jump, so the window closes at 18; update_player`
## Loop (1 build, at most 2 Azahar starts; base runs not charged)
1. Edit 1 and 2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/moon6_host.txt 2>&1`. Pass: `ND HOST TESTS PASS` and `moon ok 16/16 (arc first 8)`.
   Commit `MOON6 1: moon jump after the apex` (changed files only).
2. One build (`sh 3ds/build.sh > build3ds/out/moon6_build.txt 2>&1`). Pass: rc 0, memcheck margin above 0.
3. Gate (the cheat is off in this bench: no moonjump switch, 3ds/tools/azt.d/HOSTCORE2.json:8). Run:
   `python 3ds/tools/bench.py base spot_mac01_drec`, then `python 3ds/tools/bench.py run spot_mac01_drec --row MOON6 --why "MOON6 gate"`,
   then `python 3ds/tools/bench.py cmp spot_mac01_drec --row MOON6 --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal (one bit fails).
## Fix scope
A failing host test: compare your files with the blocks byte for byte (LF line ends). Do not change an expected text or a seed.
A compile error at line 350: keep the `(s16)` cast. Do not edit src/.
## Stop and hand-back
Stop: host PASS, build rc 0, gate 1/1 byte-equal. Final commit `MOON6 2: gate` with the values. It deletes ws_MOON6.md and ws_MOON6_code.md.
Hand-back: build3ds/out/handback_MOON6.txt (line 1 commit hash, then host line, build number, memcheck margin, gate value).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
User check on the console (not an agent step): with super boots, jump and hold A: Mario rises from the apex. Release A, then press A
before he lands: the spin jump and the ground pound come as in a normal jump.
