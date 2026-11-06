# Row BENCHFIX: bench.py run ids, correction records, build-at binary

## Problems (agents, 2026-11-06)
1. runs.jsonl run_id 499 is used twice (sbk56_superblock and menu_frameadv_pic): two parallel runs appended with the same next id. bench.py rebuild fails "UNIQUE constraint failed: run.id"; new runs are in runs.jsonl but not in bench.db.
2. A correction record (kind correction, refers_run_id 480, no run_id) crashed rebuild_db with KeyError. An agent patched read_runs to skip records without run_id (uncommitted in the working tree: review and commit it or replace it).
3. bench.py run prints KeyError 'run_id' after ingest.
4. bench.py build-at does not run the 3dsx it built: azt reads build3ds/pm_3ds.3dsx or build3ds/lane-<L>/, so the run used an old binary while the ingest wrote the new build number and commit (runs 480, 497 are wrong; correction records exist for 480).

## Rows (one commit each)
BF-1 IDS: run ids come from one place under a lock (a file lock or the Azahar slot mutex is not enough: use an OS file lock on runs.jsonl for read-max-and-append), so parallel runs never share an id. Existing duplicates stay in the file (append-only): rebuild assigns the duplicate a derived key (run_id plus a suffix, or the line number) and prints a warning instead of failing. Gate: bench.py rebuild passes; two parallel ingests get different ids (host test).
BF-2 CORRECTIONS: records without run_id (kind correction) are read as notes: rebuild keeps them in a notes table and marks the referred run invalid in queries (compare, trend, where skip it). Append correction records for run 497 (FADV, old binary) like 480. Gate: rebuild passes, `bench.py show`/compare excludes 480 and 497.
BF-3 KEYERROR: the print after ingest. Gate: one bench run prints its summary.
BF-4 BUILD-AT: build-at runs the 3dsx it built (pass its lane or path to azt), and the ingest checks the boot line build number against the built one (refuse on mismatch). Gate: build-at of HEAD runs, the boot line build equals the record build.

## Rules
Files: 3ds/tools/bench.py, 3ds/tools/azt.ps1 (build-at path only), 3ds/logs/perf/runs.jsonl (append only, never rewrite or delete lines). Other agents run benches now: the tool must work after each commit. Pathspec commits, own hunks only. No push, no attribution. Hand back after BF-4.
