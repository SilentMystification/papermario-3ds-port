# Bench harness handoff (epic BENCH, orchestrator tooling)

## STATE
- Branch nds1. Epic BENCH done (rows BENCH1 to BENCH5). No row file open. No agent running.
- Process: docs/handoff/test_catalog.md (situations A to F). Orchestrator rules: docs/handoff/orchestrator_rules.md.
- Tools: 3ds/tools/bench.py, bench_defs.json (16 benches), azt_scenarios.json, azt.ps1. Data: 3ds/logs/perf/runs.jsonl (append only), bench.db (untracked, `bench.py rebuild`).
- Console spots mac_01, mac_00, sbk_56 have normal-speed O3DS prof runs (`bench.py spots`).
- Open, low: boot_mem runs 21 and 22 have no linear_kb metric. Run 7 is labelled build 2020 but holds the 2099 log (left, history is append only).
- Open, peer area: game source still names texdiff and mdldiff (pm_gbi.c, model.c).
- Open: verify a subagent gets the SubagentStart caveman text (check the reply style of the next Haiku agent).
- Not this epic: M4-2 and S3F (native draw path) run in the peer session. Row files row_M4-2.md and row_S3F.md belong to it.
- Next command: python 3ds/tools/bench.py list
