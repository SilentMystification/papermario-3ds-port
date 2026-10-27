# diag_dl: display-list walk rows dl1, dl2, dl3

## State (2026-10-27, stopped on user order)
- Branch `worktree-agent-a7f5ca1ffd3572248`. dlc-compiler-c1c2 is merged. No source edit. No build. No Azahar start (budget dl1, dl2, dl3: 0 used).
- Ignored build inputs are copied from the main folder (ver/us/build, assets, ver/us/asm/data, baserom.z64, papermario.ld, ver/current, 3ds/include/pm_build.h, 3ds/ld/map_restore.ld, build.ninja).

## Numbers so far
- None measured in this session. Baseline numbers are in docs/handoff/review_render_pipeline.md section 2 (Azahar attract_s6 WORLD: pdl 0, pdlm 0, tex closes 305 of which 228 empty, tex streak 6129 us).

## Findings from code reading (not yet tested)
- F1. The `pdl 0` in the `taskpl` line can be a count fault. The main list goes through `pdl_tpl_drive` (pm_gbi.c:2459). Its G_DL handler `pdl_tpl_gdl` (pm_dlc.inc.c:1298) calls `pdl_try` and `pdl_try_m` but does not add `RS_PDL` / `RS_PDLM`. Only G_DL ops that `pm_gbi_run` handles itself add to the count (pm_gbi.c:2547, 2553). The real hit count is in the `pdl f..` line (`pdl_report`, every 300 frames, pm_dlc.inc.c:1375). Read that line first.
- F2. `pdl_try` takes only state lists: class 2 or 3 and no VTX/TRI/MTX/MODIFYVTX (`pdl_kind_s`, pm_dlc.inc.c:578). Model lists with geometry never enter it. The key is addr + heap gen + hash of the read set + combiner (`GBIG_COMB` is always in the key, pm_dlc.inc.c:908). A list that does not read the combiner still misses when the combiner before it differs. This is a likely miss cause.
- F3. Inside `pdl_tpl_drive`, when sp > 0 each op runs as `pm_gbi_run(pc, 8u)` (pm_dlc.inc.c:1368). Each call repeats the list-entry work (gv_epoch++, PMDL("dlsrc"), sc_ok = sr_ok = 0 full sync, range lookup, streak flush). This is a likely per-op cost in the walk. Not measured.

## Open items and next steps
1. dl1: build with `PM_LANE=dl`, run the yard with `phase`. Read the `pdl f..`, `taskpl`, `gbisplit`, `rsplit` lines. Add RS_PDL/RS_PDLM in `pdl_tpl_gdl` (F1). Log the key parts under the phase switch (state_in per readset bit, heap gen, refuse). Then remove GBIG_COMB from the key when the read set does not hold it (F2), if the log names it.
2. dl2: in the TEX close path, skip the close only when the batch has no vertices and nothing is pending. Keep the combiner flush and the texture bind.
3. dl3: close on resolved state (review #3), only after dl1 and dl2 pass.
4. F3 is a candidate row: run the sp > 0 ops of `pdl_tpl_drive` in one `pm_gbi_run` call per child list.

## Commands
- Before a build: `python 3ds/tools/gen_map_restore.py`, then restore 3ds/src/map_restore_table.c. Never commit that file.
- Build (Git Bash): `PM_LANE=dl sh 3ds/build.sh`
- Run (PowerShell): `$env:PM_LANE='dl'; $env:PM_ROW='dl1'; powershell -File 3ds/run_azahar.ps1 -Timeout 90 -LockWait 180 -Debug "fast preset=yard phase"`
- Checks: statehash on toad and battle, `python 3ds/tools/golden.py toad yard` once per merge.
