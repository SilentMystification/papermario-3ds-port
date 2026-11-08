# Row MAGI: Magikoopa magic attack lags when it hits Mario (HOS formation 03)

## User facts (bug, 2026-11-08, console O3DS XL, build cand = main f6ebb9e58 + PERF2 steps 1-2, wide, 30 fps)
"Battle F03 SFF magikoopa's magic attack is laggy when it hits mario." The battle is "Magikoopa Flying", BTL_HOS_FORMATION_03 : BTL_HOS_STAGE_02 (3ds/src/pm_battle_table.h:72), launched from hos_00 (prof scene "hos_00 5:0 m8 b1027").
Console prof windows of that battle (build3ds/hw_live_log_131.txt #3819-3921, f40996-f41896): fps 30.0, ft 33.4, max 34-35.5 ms, o34 2-3; gbi 25.6-27.9; cls rec 3.1-4.2 ent 2.4-2.6 spr 1.2 sub 2.9-3.2 rest 16.0-16.8 (rest is 60% of the draw path in battle); 51-57 draws, 123-128 triangles; I-miss about 68k, D-miss about 37.5k. The hit itself is a short spike that the 300-frame average hides.
Reproducing scenario: add one (debug battle BTL_HOS_FORMATION_03, wait for the Magikoopa magic attack on Mario; until-line a log line at the hit; shots around the hit). A per-frame spike count is needed: log the frames over 34 ms with their frame number in a window around the hit (Azahar count of draws/misses per frame is valid; timing direction only).

## Goal
Trace by code first (CLAUDE.md rule 5): name the effect(s) the magic attack draws when it hits (src/effects fx_*, src/battle/actor magikoopa scripts; e.g. the spell beam, the hit sparkle/flash, a screen flash), their draw path on the port (entity G_DL, rect, sprite, off-screen/frame-buffer read such as a screen distortion or a captured frame), and the per-frame work at the hit (draws, model/entity misses, texture uploads, frame-buffer copies, effect list rebuilds). Also name what `rest` (16 ms) is in this battle. Fix each named cause.

## Start state
Worktree build3ds/wt/magi, branch magi from main (current). Copy assets/us and ver/us/build into it. PERF2 changes submit/entity/D-cache/I-cache paths and STARFX may change the model key: merge main at each step boundary.

## Touch only
The paths the trace names. src/ changes under #ifdef TARGET_3DS.

## Done criteria
1. Trace in the handback (effect, path, per-frame cost, file:line).
2. Build; nd_host PASS.
3. The new scenario: spike frames and their draw/miss counts at the hit before and after; one shot at the hit shows the effect drawn as before.
4. The 4 byte gates (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015) byte-equal unless the trace shows they use the changed path.
5. A console prof bench for this battle (the user pushes).
Affected scenarios (no others): the new Magikoopa scenario, the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11, docs/handoff/hw_guidelines.md. PM_LANE=magi PM_ROW=magi on every build.sh and bench call. No git stash. No toggles, no stubs. 2 tries per named cause. Scenario and bench in the same commit.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=magi PM_LANE=magi sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=magi PM_LANE=magi python 3ds/tools/bench.py run <bench> --row magi --lane magi --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "magi ready <hash>".
