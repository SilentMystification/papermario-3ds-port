# Row STARFX: hos_01 star spirit pop-in star effect lags

## User facts (bug, 2026-11-08, console O3DS XL, build cand = main f6ebb9e58 + PERF2 steps 1-2, wide, 30 fps)
"in hos_01 when you climb to the top of shooting star summit and begin the cutscene where you talk to the star spirits (area 5 map 1), when the 7 star spirits pop in, the star effect that pops up around them causes the game to lag."
Console prof windows at the summit (build3ds/hw_live_log_131.txt #2462-2583, f27013-f28213, Mario at -53 250 -161, story -107): fps 29.2-29.4, ft 33.9-34.1, max 48 ms, o34 118-128 of 300; gbi 30.5-32.2; cls rec 9.8-11.4 ms, ent 6.3, spr 0.07, sub 2.5, rest 11.2-11.9; only 31-32 draws and 114-116 triangles; pmu I-miss about 70,600, D-miss about 53,700, 8.3 M cycles. rec at about 0.33 ms per draw for 31 draws suggests model cache misses every frame (a varying key word), as MERLEE found for the shroud fade (5401 misses, 7 clears) and MAPX step 2 plans for door fades (colour-only words in the key).
Reproducing scenario: add one (hos_01 with the story value of the star spirit scene, the cutscene trigger at the summit; until-line a log line when the spirits appear; or a warp + story + script start).

## Goal
Trace by code first (CLAUDE.md rule 5): during the pop-in, name per frame (a) the model cache misses and the key word that changes (ndm/ndmdiff lines, mg_cls, w<n>; Azahar counts are valid), (b) the effect that draws the stars (fx_* in src/effects, its draw path on the port: entity/G_DL/rect, draws per frame, and whether its list is rebuilt or rehashed each frame), (c) what `rest` is. Fix each named cause. If the cause is a colour/alpha word in the model key: MAPX (row MAPX step 2, branch mapx) changes the same key rule for door fades; merge main and check whether MAPX step 2 is merged; build on it (one key rule, no second path) or coordinate by message.

## Start state
Worktree build3ds/wt/starfx, branch starfx from main (current). Copy assets/us and ver/us/build into it.

## Touch only
The paths the trace names. src/ changes under #ifdef TARGET_3DS.

## Done criteria
1. Trace in the handback (key word or effect path, file:line, value).
2. Build; nd_host PASS.
3. The new scenario: model misses per frame and clears during the pop-in before and after (Azahar counts); one shot shows the spirits and the star effect drawn as before (picture unchanged in kind).
4. The 4 byte gates (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015) byte-equal unless the trace shows they use the changed path.
5. A console prof bench for the scene (the user pushes).
Affected scenarios (no others): the new hos_01 scenario, the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=starfx PM_ROW=starfx on every build.sh and bench call. No git stash. No toggles, no stubs. 2 tries per named cause. Scenario and bench in the same commit.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=starfx PM_LANE=starfx sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=starfx PM_LANE=starfx python 3ds/tools/bench.py run <bench> --row starfx --lane starfx --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "starfx ready <hash>".
