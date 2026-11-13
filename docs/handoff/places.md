# Places of open rows (orchestrator edits this file in place; a row with an equal or overlapping place waits)

One line per open row: row | model | file: function or region names | merge state. Delete the line when the row is merged or closed.
A shared registry (gen_gfx_tab.sh GCONV_SRC lines, azt_scenarios.json, bench_defs.json) is never a place of a row: see HANDOFF.md
"Parallel work without merge conflicts". Plan v3 (plan_endgame.md) is accepted (user, 2026-11-12). FPS60FL is merged (d2b2b3b2a).

| row | model | places | state |
|---|---|---|---|
| PUSHLANE | Haiku | 3ds/go.ps1 :236-238; 3ds/push_3ds.sh :176; new 3ds/tests/push/lane.sh | ready |
| HOSTDIR | Haiku | 3ds/tools/nd_host_test.sh one new line after `/tmp/kcc` (:73); new 3ds/tests/host.d/README.txt, 00_hostd.sh | ready (first: the 5 rows below wait for it) |
| FRAMEHASH2 | Haiku | 3ds/include/nd_fhash.h after nd_fh_words; 3ds/src/nd_fhash.c new functions after nd_fh_words; 3ds/tools/nd_fhash_test.c new cases at the end | waits for FRAMEHASH1 |
| HOSTCORE2 | Sonnet | new 3ds/tools/host_c3d.c, host_core.c, host_core.h, 3ds/tests/host.d/hostcore.sh | ready (HOSTCORE1 merged) |
| NPSO2 | Sonnet | new 3ds/src/nd_decide.c, 3ds/tools/nd_decide_test.c, 3ds/tests/host.d/decide.sh; nd.h one struct after NdTevOut; nd_comb.c nd_comb_decide and the code it moves; nd_tmpl.c only if needed | waits for NPSO1 |
| FPS60VAL | Sonnet | 3ds/src/pm_gpu.c end of kc_build, end of pm_gpu_key_show; new 3ds/tools/kcval.py | not started (user decides) |
