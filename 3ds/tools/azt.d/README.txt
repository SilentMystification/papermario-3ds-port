A row adds its own <row>.json here; never edit another row's file or the main file.
Format: azt.d/<row>.json = {"scenarios": [ ... ]}. The scenarios of this directory are merged after azt_scenarios.json (files in name order); a name defined twice is an error that names both files.
Read by 3ds/tools/azt.ps1 (Get-AztScenarios, Set-ScenarioStatus) and 3ds/tools/bench.py (all_scenarios). Rule and history: docs/handoff/test_catalog.md (Files).
