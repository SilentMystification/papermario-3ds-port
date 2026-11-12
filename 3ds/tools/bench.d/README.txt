A row adds its own <row>.json here; never edit another row's file or the main file.
Format: bench.d/<row>.json = {"benches": [ ... ]}. The benches of this directory are merged after bench_defs.json (files in name order); a name defined twice is an error that names both files.
Read by 3ds/tools/bench.py (load_defs). Rule and history: docs/handoff/test_catalog.md (Files).
