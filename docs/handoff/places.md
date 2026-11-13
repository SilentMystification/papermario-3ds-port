# Places of open rows (orchestrator edits this file in place; a row with an equal or overlapping place waits)

One line per open row: row | model | file: function or region names | merge state. Delete the line when the row is merged or closed.
A shared registry (gen_gfx_tab.sh GCONV_SRC lines, azt_scenarios.json, bench_defs.json) is never a place of a row: see HANDOFF.md
"Parallel work without merge conflicts". Plan v3 (plan_endgame.md) is accepted (user, 2026-11-12). FPS60FL is merged (d2b2b3b2a).

| row | model | places | state |
|---|---|---|---|
| PUSHEND2 | Haiku | 3ds/go.ps1 :188 and the `finally` block (:246-253); 3ds/tests/push/run.sh (d) :71-73 | running |
| PUSHLANE | Haiku | 3ds/go.ps1 :236-238; 3ds/push_3ds.sh :176; new 3ds/tests/push/lane.sh | waits for PUSHEND2 |
| HOSTDIR | Haiku | 3ds/tools/nd_host_test.sh one new line after `/tmp/kcc` (:73); new 3ds/tests/host.d/README.txt, 00_hostd.sh | ready (first: the 5 rows below wait for it) |
| NOLIST | Haiku | new 3ds/tools/gen_nolist.py, 3ds/include/nd_nolist.h, 3ds/tests/host.d/nolist.sh | ready after HOSTDIR |
| HOSTCORE1 | Haiku | new 3ds/include/nd_drec.h, 3ds/tools/nd_drec.c, nd_drec_test.c, 3ds/tests/host.d/drec.sh | ready after HOSTDIR |
| FRAMEHASH1 | Haiku | new 3ds/include/nd_fhash.h, 3ds/src/nd_fhash.c, 3ds/tools/nd_fhash_test.c, 3ds/tests/host.d/fhash.sh | ready after HOSTDIR |
| FRAMEHASH4 | Haiku | 3ds/tools/bench.py: new block before `def shots_diff`, the 10 `*.ppm` glob lines, cmd_cmp compare loop, cmd_ingest_last shot line; new 3ds/tests/fhashcmp/ | ready (no blocker) |
| NPSO1 | Haiku | 3ds/include/nd_api.h new lines after `nd_pso_get` (:21); new 3ds/tools/nd_psodesc_test.c, 3ds/tests/host.d/psodesc.sh | ready after HOSTDIR |
| CONV2B | Haiku | new 3ds/tools/nd_vslot.h, nd_vslot.c, nd_vslot_test.c, 3ds/tests/host.d/vslot.sh | ready after HOSTDIR |
| FPS60VAL | Sonnet | 3ds/src/pm_gpu.c end of kc_build, end of pm_gpu_key_show; new 3ds/tools/kcval.py | not started (user decides) |
| LISTCEN2 | Haiku | 3ds/tools/listcen.py; bench_defs.json listcen entry (per-row file) | plan phase H, first rows |
| N64DEL1 / N64DEL2 / N64DEL3 | Haiku | new 3ds/tools/unifdef3ds.py (N64DEL1); TARGET_3DS conditional lines of src/effects / src/world, src/entity, src/battle, src/pause, src/filemenu, include / src/*.c | waits for LISTCEN2 |
