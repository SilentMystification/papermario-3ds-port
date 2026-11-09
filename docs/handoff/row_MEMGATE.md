# Row MEMGATE: know the console memory; stop a build that cannot boot before it is pushed

## User facts (2026-11-08)
Build 3299 (main d77987446) stopped on the O3DS XL (.131, Homebrew Launcher via Download Play, applet mode): "Not enough memory for the ROM: need 26 MB, free 25 MB" (3ds/src/pm_os.c:257; cap = pm_heap_bytes() - 3458 KB; need = PM_ROM_PACKED). Build 3212 (e01685a71) booted. Cause: +413,475 B of .bss (MAPX prefetch buffers 384 KB static, TIK19 mfr_k 11.9 KB); row MEMFIT fixes those. User: "we should be able to catch this ahead of time to prevent these kinds of pushes. I think you should know how much ram we have to work when we are in the homebrew menu applet mode."

## Goal
1. MEASURE: one boot line printed in every mode (also prof and log=off, through the path that survives prof mode, e.g. pm_log_evt or the prof line set): `mem boot: region <APPLICATION region size> used <osGetMemRegionUsed> free <osGetMemRegionFree> static <text+rodata+data+bss from linker symbols> linear <linear heap size> heap <pm_heap_bytes> rom_need <PM_ROM_PACKED> leave <3458 KB> cap <cap> margin <cap - need>` (KB). Read pm_heap_bytes and the heap/linear split code (3ds/src, the __system_allocateHeaps or linear heap size setting) and name how each number is derived.
2. BUDGET FILE: 3ds/tools/mem_budget.json with the console facts per device (O3DS XL .131 HBL/applet: region size, linear size, fixed costs) and the rule: margin = region - static - linear - other fixed - rom_need - leave >= safety (choose 256 KB, state why). Fill the O3DS XL values from (a) the code (constants), (b) today's failure (free 25 MB with build 3299 static size from build3ds/pm_3ds.map / 3ds/logs/builds/pm_3ds_3299.map), (c) the success of build 3212 (pm_3ds_3212.map). Mark which values are measured and which derived; the next console boot line replaces derived ones (the user pushes MEMFIT's build).
3. GATE: 3ds/tools/memcheck.py reads the map (section sizes) and the budget, prints `memcheck: static <KB> margin <KB> device o3ds_xl OK|FAIL`. build.sh prints it after every build (main and lanes); push_3ds.sh refuses a push with FAIL (clear message, exit non-zero) before 3dslink. docs/handoff/orchestrator_rules.md merge step: the memcheck line goes into the merge commit message; a FAIL blocks the merge. Also report the 10 largest .bss/.data symbols in the memcheck output (helps the next fix).
4. Self-test: memcheck on build 3299's map gives FAIL; on build 3212's map gives OK; on the MEMFIT build (merge main when MEMFIT lands) gives OK.

## Start state
Worktree build3ds/wt/memgate, branch memgate from main. MEMFIT runs in parallel (pm_prefetch.c, pm_gpu.c mfr_k): do not touch those.

## Done criteria
1. The boot line in the code (build, nd_host PASS); one Azahar boot shows it (Azahar values differ from the console: say so).
2. memcheck self-test as in 4.
3. push_3ds.sh refuses a FAIL build (test with build 3299's map: --dry).
4. mem_budget.json and a short section in docs/handoff/hw_guidelines.md (the memory budget rule, replacing any stale sentence) and the orchestrator_rules.md merge step.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=memgate PM_ROW=memgate on every build.sh and bench call. No git stash. No stubs.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=memgate PM_LANE=memgate sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "memgate ready <hash>" with the measured and derived O3DS XL numbers.
