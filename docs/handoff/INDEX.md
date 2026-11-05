# docs/handoff index

Live files only. Older logs are in archive/ (agent_*, diag_*, inv_*, plan_*, trace_*, research_*, HANDOFF_*). Do not read archive/ unless a row file names a file there.

## Rules and process
- ../../CLAUDE.md: shared rules (tests, code, platform).
- orchestrator_rules.md: orchestrator only. ORCH_RULES.txt, subagent_style.txt, subagent_hook.js: style hooks.
- row_TEMPLATE.md: template for the row file each new agent reads first.
- hw_guidelines.md: 3DS cache and ARM11 rules for all code (3 KB).

## Native draw epic (current work)
- start_here_native_draw.md: current state and what is left (long-form handoff).
- handoff_native_epic_orchestrator.md: findings and pitfalls of the code session of 2026-11-05.
- plan_native_system.md: design (large; open only the sections a row names).
- plan_native_slices.md: slice table S0 to S11 and rows (large; open only the rows a row names).
- design_native_draw.md, design_producer_models.md: fidelity rule and producer record design.
- frame_budget.md: 16.7 ms stage budget and O3DS XL baseline.
- agent_nds_s0.md to agent_nds_s1e.md, agent_nds1_e6.md: logs of finished agents (state at their end).
- map_e4.md, map_rtb1.md, diag_e53_*.md, diag_s1_1_picture.md, diag_uc3a_bad.md, inv_frame_cost.md, plan_prof_mode.md, nds1_*.patch, nds1_status_2026-11-05.md: findings the plan files cite.
- perf_history.csv: console prof history (python 3ds/tools/prof_record.py add|trend). Its rows are imported to runs.jsonl by `bench.py import-csv`.

## Tests
- 3ds/tools/bench.py: entry point for every test and profiling run (`bench.py list`, `show`, `run`, `ingest`, `compare`, `trend`). Benches in 3ds/tools/bench_defs.json, runs in 3ds/logs/perf/runs.jsonl.
- test_catalog.md: THE bench process (situations A to F: check a row, before/after, console run, area lookup, add a bench, rebuild). Read it before any test. Old scenario tables: archive/.
- route_test.md, sweep_420.md: tour and sweep how-to.
- work_queue.md: older queue (stale; check git log first).
