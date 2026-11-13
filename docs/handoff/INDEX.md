# docs/handoff index

Live files only. Older logs are in archive/ (agent_*, diag_*, inv_*, plan_*, trace_*, research_*, HANDOFF_*). Do not read archive/ unless a row file names a file there.
- HANDOFF.md: orchestrator handoff (what runs, the queue, user checks); overwritten at each handoff.

## Rules and process
- ../../CLAUDE.md: shared rules (tests, code, platform).
- orchestrator_rules.md: orchestrator only (workstreams, roles, merge steps, console); its Core block is printed every turn by the hook in .claude/settings.json. subagent_style.txt, subagent_hook.js: style hooks. no_guess_hook.js, no_poll_hook.js: PreToolUse guards (no guess and check; no sleep, ping or tail --pid in Bash).
- ../../.claude/agents/coder.md, oracle.md: agent definitions.
- plan_endgame.md: the closed plan to remove the display list system (census, gates, row families, counters, end proof); row_TESTIDEAS.md: the gate ideas and the host golden-master evaluation.
- oracle_handoff.md: start file of a fresh oracle. ws_HUNIT.md, ws_A.md, ws_B.md, ws_C.md: workstream goal pages (deleted by the commit that completes each). design_state_calls.md: one native setter per state-call shape, readers, writers and edit order.
- row_TEMPLATE.md: template for the row file each new agent reads first.
- hw_guidelines.md: 3DS cache and ARM11 rules for all code (3 KB).
- git_branches.md: tip hash of every branch deleted or kept in the row GIT cleanup (recovery record).

## Native draw epic (current work)
- start_here_native_draw.md: current state and what is left (long-form handoff).
- handoff_native_epic_orchestrator.md: findings and pitfalls of the code session of 2026-11-05.
- plan_native_system.md: design (large; open only the sections a row names).
- plan_native_slices.md: slice table S0 to S11 and rows (large; open only the rows a row names).
- design_native_draw.md, design_producer_models.md: fidelity rule and producer record design.
- frame_budget.md: 16.7 ms stage budget and O3DS XL baseline.
- agent_nds_s0.md to agent_nds_s1e.md, agent_nds1_e6.md: logs of finished agents (state at their end).
- agent_direct.md: DIRECT epic handoff (state after D9: no task list; next D10, the walker removal).
- plan_direct.md: plan DIRECT (2026-11-09): the game records native commands into the frame stream (NdCmd) on the logic thread, core 0 replays them; LIST span bridge, producer API per class, slices D0 to D9, memory and thread rules.
- epic_ASSETFREE.md: PARKED, RELEASE BLOCKER, do not start (user): future epic that removes the game assets from the ELF; the console makes them on first boot.
- plan_nativeapi.md: native draw API plan (2026-11-11, user decision): the game's draw code records native commands (pipeline-state id, texture handle, rect/quad/glyph/mesh/particle calls) instead of N64 state ops; stages S0-S6, each merged before the next; the old path stays until the last call site moves. First row NAPI0.
- plan_datapath.md: data-path design study (2026-11-11): SD to RAM to CPU/GPU hops per asset class, transform-once principle, native-asset generation pass (links to epic_ASSETFREE.md), censuses DPCEN1 and DPCEN2.
- plan_hotcode.md: plan HOTCODE (2026-11-09): shrink the core 0 draw code and data to fit the ARM11 L1 (hot set by class, stale link group, stages HC0 to HC10, size and pmu gates).
- plan_s10_path.md: plan for S10 (oracle 2026-11-06): producers write NdRec records (streams CORE, UI, FXW, WBO), then pm_gbi.c is deleted; decisions D1 to D10.
- agent_S7.md: WBO-1 site list (custom gfx builders in src/world and src/battle: 57 sites, kinds, scenario candidates) and the WBO-2a to WBO-3 row groups.
- agent_s10.md: S10 epic handoff (orchestrator, 2026-11-07): merged rows, open rows, parked bugs, size and push state, next command.
- agent_wcen.md: WCEN walker producer census at 5 scenes (top 8 per scene, function, file, slice owner). Census tool: pm_gbi.c os_report.
- agent_boot.md: row BOOT (boot time): ROM stream at boot (pm_romload.c), boot ms line, boot ROM reads, the elf size listing.
- agent_bg.md: background image faults (BG1 flicker at mac_00, BG2 garbled row), how the background strips are drawn.
- map_e4.md, map_rtb1.md, diag_e53_*.md, diag_s1_1_picture.md, diag_uc3a_bad.md, inv_frame_cost.md, plan_prof_mode.md, nds1_*.patch, nds1_status_2026-11-05.md: findings the plan files cite.
- perf_history.csv: console prof history (python 3ds/tools/prof_record.py add|trend). Its rows are imported to runs.jsonl by `bench.py import-csv`.

## Tests
- 3ds/tools/bench.py: entry point for every test and profiling run (`bench.py list`, `show`, `run`, `ingest`, `compare`, `trend`). Benches in 3ds/tools/bench_defs.json, runs in 3ds/logs/perf/runs.jsonl.
- test_catalog.md: THE bench process (situations A to F: check a row, before/after, console run, area lookup, add a bench, rebuild). Read it before any test. Old scenario tables: archive/.
- route_test.md, sweep_420.md: tour and sweep how-to.
- audit_system.md: system audit of the 3DS runtime (row AUDIT, 2026-11-08): interface map with the 8 threads, findings A (console cost), B (correctness and seams), C (maintainability) with file:line, and the work list W1 to W10 with each one done or open.
- plan_core2.md: plan CORE2 (oracle 2026-11-08): more work off the game thread (audio one step ahead, walker thread on N3DS core 2, texture worker), rows CORE2-0 to CORE2-4.
- plan_tours.md: plan TOURS (oracle 2026-11-08): map tour and boss battle tour in one session with prof windows per warp, curated list, perfect action cheat; rows TOUR1, CHEATA, TOUR2, BT1, CUR1.
- audit_warm_runs.md: audit of warm restarts (stale ROM, AZT8 and AZT9c) and picture gates (stale shot, AZT7) for 2026-11-06 12:00 to AZT9c: per gate OK or SUSPECT with run lines, and the rerun list.
- work_queue.md: older queue (stale; check git log first).
