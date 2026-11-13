# Places of open rows (orchestrator edits this file in place; a row with an equal or overlapping place waits)

One line per open row: row | model | file: function or region names | merge state. Delete the line when the row is merged or closed.
A shared registry (gen_gfx_tab.sh GCONV_SRC lines, azt_scenarios.json, bench_defs.json) is never a place of a row: see HANDOFF.md
"Parallel work without merge conflicts". Plan v3 (plan_endgame.md) is accepted (user, 2026-11-12). FPS60FL is merged (d2b2b3b2a).

| row | model | places | state |
|---|---|---|---|
| PUSHKEY | Haiku | 3ds/build.sh the 3 generator call lines (:57, :79, :101); 3ds/tests/push/run.sh after case (b2) (:52) | running |
| PUSHSTALL | Haiku | 3ds/push_3ds.sh after `memflag=""` (:50), option parser (:52-64), poll loop (:407-423); 3ds/tests/push/run.sh after :26 | running |
| PUSHEND1 | Haiku | 3ds/push_3ds.sh :37, :121, :179, :201, :271-306, :333, :347, :491-504; 3ds/tests/push/run.sh after (c) (:60) | running |
| PUSHWARP | Haiku | 3ds/go.ps1 :66, :139-140, :173; 3ds/tests/push/run.sh after (f) (:94) | running |
| PUSHEND2 | Haiku | 3ds/go.ps1 :188 and the `finally` block (:246-253); 3ds/tests/push/run.sh (d) :71-73 | waits for PUSHEND1 |
| PUSHLANE | Haiku | 3ds/go.ps1 :236-238; 3ds/push_3ds.sh :176; new 3ds/tests/push/lane.sh | waits for PUSHKEY, PUSHEND1, PUSHEND2 |
| FPS60VAL | Sonnet | 3ds/src/pm_gpu.c end of kc_build, end of pm_gpu_key_show; new 3ds/tools/kcval.py | not started (user decides) |
| LISTCEN2 | Haiku | 3ds/tools/listcen.py; bench_defs.json listcen entry (per-row file) | plan phase H, first rows |
| ELFGATE | Haiku | new 3ds/tools/elfgate.py, elfgate_patterns.txt | plan phase H, first rows |
| N64DEL1 / N64DEL2 / N64DEL3 | Haiku | new 3ds/tools/unifdef3ds.py (N64DEL1); TARGET_3DS conditional lines of src/effects / src/world, src/entity, src/battle, src/pause, src/filemenu, include / src/*.c | waits for LISTCEN2 |
