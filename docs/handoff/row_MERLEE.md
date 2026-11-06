# Row MERLEE: Merlee's upgrade spell in dro_02 soft-hangs (the card effect does not play)

## Facts (user, O3DS console, 2026-11-06, build of nds1 around b09ac893c or later)
In dro_02 (Merlee), casting the upgrade spell: the card effect does not play, then the spell animation sequence never ends (soft hang: the game runs, the script waits forever).

## Step 1: READ-ONLY trace (no edit, no build)
- Find the spell script: src/world/area_dro/dro_02 (Merlee scripts, grep `Merlee` / `PlayEffect` / `EFFECT_` in that folder) and the effect it plays for the card (grep the effect id in src/effects/, the effect's update and render functions, and the effect table in src/effects.c).
- Find what the script waits on (the effect's end, a flag the effect sets, a timer, a battle-state or a sprite animation end).
- Check the port side for that effect: is it compiled in (3ds/CMakeLists.txt, any port stub or skip list, effect overlay loading in 3ds/src), does its spawn return NULL on the 3DS (allocation, a missing overlay, a refused data pointer), does its update run (effect_update list), does its render go through a path that the native draw work changed (G_PM_* words, the record arms in pm_gbi.c, nd_sprite or nd_model fallbacks).
- Write docs/handoff/diag_merlee.md: the wait condition with file:line, the reason it is never met on the 3DS (or the 2 most likely reasons), and the Azahar scenario that reproduces it (warp=dro_02:<entry> story=<value with Merlee available>, padscript to talk and pick the spell, until-line the effect spawn log or the hang). Commit only that file.

## Step 2: fix (a code agent, after step 1)
The fix named in diag_merlee.md, with the gate: the scenario shows the card effect in a shot and the script reaches its end (log line). Lane merlee: `PM_LANE=merlee`, azt `-Build merlee`.

## Rules
Main folder, branch nds1. Other code agents work here (S6: entities, MENU: menu files): do not edit their files. Pathspec commits only, own hunks only, JSON files stay valid. Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. No toggles, no stubs, no git push, no console push, no attribution. Never commit 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). Game source only inside `#ifdef TARGET_3DS`.
