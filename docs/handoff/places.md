# Places of open rows (orchestrator edits this file in place; a row with an equal or overlapping place waits)

One line per open row: row | model | file: function or region names | merge state. Delete the line when the row is merged or closed.
A shared registry (gen_gfx_tab.sh GCONV_SRC lines, azt_scenarios.json, bench_defs.json) is never a place of a row: see HANDOFF.md
"Parallel work without merge conflicts". Open rows at the handoff of 2026-11-12: none running; the passed rows S1TSTEN, S7ENT,
S7EFX7 and S7EFX8 wait for the user's decision (their places: src/screen_overlays.c pm_tsten_run; 3ds/src/nd_ent.c, nd_walk.c
nd_ent_pieces; src/effects/*.c effect files of groups 7 and 8).

| row | model | places | state |
|---|---|---|---|
