# agent_dlc — display-list compiler (C0–C2)

## State
- **Branch:** `dlc-compiler-c1c2` (not pushed).
- **C0:** `GbiState`, `gbi_state_get` / `gbi_state_apply` / hash, `pdl0` profiling (`switch pdl0`).
- **C1:** Static texheap/rodata kind-S lists compile once (`pdl_try` on `G_DL` when `dlc_enabled()`): EXIT snapshot, TEX slot + `pm_tex_slot_gen`, lazy bind in `ensure_tile`, `pm_pdl_heap_gen` on TextureHeap invalidation, `pdlcheck` exit compare (`switch pdlcheck`). Interpret path skipped on replay.
- **C2:** PSO table (`pso_tab`, capture at compile via `gv_epoch_enter`); shape replay uses `pdl_pso_cur` through `gv_epoch_enter` / `pdl_pso_use`; `sync_combine` bypasses OPT-SC memo when `pdl_ep_pso` is set.
- **Build:** `sh 3ds/build.sh` OK on this branch (2026-10-23).
- **Not run:** Azahar golden, statehash, `pdlcheck` draw path, `pdl0` rep gate.

## Commits
- None yet on branch.

## Open items
- Azahar: golden 10 scenes, `pdlcheck` zeros, `dlc` + `pdl` line, gbiops.
- C1 gate (`pdl0` rep ≥ 90%) still unmeasured.
- Hardening: refuse reasons, TEX gen mismatch recompile, full `pdlcheck` dual-capture (plan 2.7).
- `pm_gpu.c` unchanged; PSO packet pre-build is C6.

## Next steps
1. Azahar with `dlc` + `pdlcheck` on toad, yard, battle, route play1.
2. Fix regressions from exit/PSO/TEX replay if any.
3. Merge to main after checks; no `map_restore_table.c` in commits.

## Commands
```
git checkout dlc-compiler-c1c2
Copy-Item 3ds\src\map_restore_table.c 3ds\src\map_restore_table.c.bak -Force
python 3ds/tools/gen_map_restore.py
Copy-Item 3ds\src\map_restore_table.c.bak 3ds\src\map_restore_table.c -Force
bash 3ds/build.sh
$env:PM_ROW='C1'
powershell -File 3ds/run_azahar.ps1 -Turbo -FaultStop -Force -Timeout 150 -Until 'pdl f' -Debug 'fast preset=toad fixedseed dlc pdlcheck loglevel=debug'
```
