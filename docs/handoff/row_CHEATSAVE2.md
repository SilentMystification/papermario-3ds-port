# Row CHEATSAVE2: the cheat save covers every Cheats tab change (user, 2026-11-09)

## User facts
"The dirty / stale state in the cheat menu should also be activated when in the submenus, i.e. toggling a key item should update the
dirty state." User decision: X saves those changes into the cheat file of the slot; loading the slot applies them again.

## Today (code read)
pm_cheatsave.c:62 pm_cheat_dirty() compares pm_cheat_persist_get() (pm_dbg_game.c:210: the 6 cheat bits + moon jump) with saved_bits.
Files cheats_slot<N>.ini (pm_cheatsave.c:25). The footer "Press X to save cheats" and the X handler: pm_options_ui.c:43-45, :192;
cheat_tab() already covers the Cheats sub-pages (home_tab 3). Key items, badges, stats, partners, coins, current partner, Mario
profile folders (equipment, inventory, merlee, position ...) change game state directly and are not in the file.

## Goal
Every value the Cheats tab can change is part of the saved cheat set: the switches as now, plus the values set through the Cheats
sub-pages (key items owned, badges owned/equipped, stats (HP/FP/BP/level/star power/star pieces), coins, partners owned and their
ranks, current partner, the profile folders). Dirty = any of these differs from the set loaded or saved last. X writes all of them
to cheats_slot<N>.ini (new keys; old files without them load as before). Loading the slot applies them after the game save loads.

## Start state
`sh 3ds/tools/wt_new.sh CHEATSAVE2`, branch CHEATSAVE2. PM_ROW=CHEATSAVE2, PM_LANE=CHEATSAVE2. One coder agent.

## Design rule
Do not hash game memory each frame. Mark dirty at the point where a Cheats sub-page row changes a value (one call per change in
pm_options.c / pm_dbg_game.c); keep a "changes since save" record of which fields the Cheats tab set and their values; X writes
that record; load applies it. Spawned items are not part of the set (one-off actions).

## Touch only
3ds/src/pm_cheatsave.c, pm_dbg_game.c, pm_options.c, pm_options_ui.c, azt_scenarios.json, bench_defs.json.

## Done criteria
nd_host PASS, memcheck OK. One Azahar run: Cheats > Key items, toggle one item -> footer shows "Press X to save cheats" (log line),
X -> file written (log the keys); reload the slot (or a second run that loads it) -> the item is owned (log). Budget: 4 Azahar starts.
Delete this row file in the completing commit.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
