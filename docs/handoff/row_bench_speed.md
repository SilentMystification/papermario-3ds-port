# Row BENCH3: record the Azahar speed mode of every run (turbo or normal)

## Goal
Azahar turbo and normal speed give different frame times. A mean over mixed rows is wrong. Each Azahar run records its speed mode, and queries never mix the two modes.

## Start state
Branch nds1. Python: /c/Python314/python. Tools and docs only. No game source. No console push. Never touch or commit 3ds/src/map_restore_table.c. BENCH2 is committed (c62660a42). Another session commits game source in this folder: stage and commit ONLY the files you change (git add <file>, then git commit; check git status before the commit and unstage files that are not yours). Do not commit src/model.c, 3ds/src/pm_gbi.c, include/gbi_custom.h.
Read first: 3ds/tools/bench.py, 3ds/tools/bench_defs.json (top), 3ds/tools/azt.ps1 (where it calls run_azahar.ps1), 3ds/run_azahar.ps1 (parameters -Turbo and -TurboLimit), the usage section of docs/handoff/test_catalog.md.

## Do
1. Fields: add `speed` ("turbo" or "normal") and `frame_limit` (percent, number or null) to the run line in 3ds/logs/perf/runs.jsonl and to the run and sample tables of bench.db. Console and host runs: speed "normal", frame_limit null. Existing runs: speed "unknown", frame_limit null (do not guess).
2. Source of the values: read them from how the run was started. azt.ps1 passes -Turbo and -TurboLimit to run_azahar.ps1: make azt.ps1 hand the two values to bench.py ingest (for example env vars PM_SPEED and PM_FRAME_LIMIT or two arguments), and let bench.py ingest-last and ingest store them. If run_azahar.ps1 defaults to turbo, the default is recorded. If the Azahar log prints the speed mode, prefer that.
3. bench_defs.json: add a per-bench `speed` default (kind azahar_*: the speed the bench is meant to run at; others: "normal"). `bench.py run` starts Azahar at that speed and records it. A run that used another speed is recorded with the speed it really had.
4. Queries: `where`, `compare`, `trend`, `spots` take `--speed turbo|normal|any`. Default: the bench default speed. Never mix turbo and normal rows in one mean: with `--speed any`, print one block per speed. A `compare` of two builds with different speeds prints a warning line and no delta.
5. `bench.py rebuild` reproduces the new columns from runs.jsonl alone. The history stays append-only (CLAUDE.md rule 11): do not rewrite old run lines, only new runs carry the fields; the importer fills "unknown" for old ones.
6. Update the usage section of docs/handoff/test_catalog.md (keep it 20 lines or fewer) with the speed rule.

## Done criteria (all must pass; handback only then)
1. `bench.py rebuild` clean. Every run in the db has a speed value ("unknown" for old runs).
2. One azahar bench run (boot_mem) at turbo and one at normal, via `bench.py run` with the speed set, both recorded with the right speed and frame_limit. Paste `bench.py where` for both.
3. `bench.py compare` between a turbo run and a normal run prints the warning and no delta. `--speed any` prints two blocks.
4. Commit only your files (tools, defs, docs, runs.jsonl with git add -f). No attribution. No push.

## At the end
Overwrite the BENCH2 STATE block at the top of docs/handoff/agent_bench1.md with a BENCH3 STATE block (10 lines or fewer). Do not append. Delete this row file in your last commit.
