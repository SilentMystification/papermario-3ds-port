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

## dl1 result (2026-10-27, Azahar lane dl, build 1178-dl, PM_ROW=dl1, 5 of 5 starts used)
Step 1: RS_PDL / RS_PDLM now counted in pdl_tpl_gdl (pm_dlc.inc.c). Count-only edit, no change to the draw path.
Step 2: pdl_try counts its calls per dls_class (`cls` field in the pdl report line). Count-only.

Numbers (yard, preset=yard, 6091 frames in the 2nd run, log build3ds/lane-dl/last_log.txt):
- taskpl pdl and pdlm: 0 and 0 in every frame. Before (review, attract_s6 WORLD): pdl 0, pdlm 0. The count gap is real but it is not the cause: real hits are 0.
- pdl report (every 300 frames): hit 0, comp 0, refuse 0, full 0 in all windows after frame 299. m hit 0, comp 0. Frames 1 to 299: m refuse 37 in one window.
- pdl_try calls per class (frames 1 to 299 only): class 2 1, class 7 37. Classes 0, 1, 3, 4, 5, 6: 0. After frame 299: 0 calls in every window.
- dlc hit: 20400 per 300 frames after frame 299 (dlc_try runs before pdl_try in pdl_tpl_gdl, so dlc answers the G_DL before pdl sees it).
- walk per frame (6091 frames): bg 2356 us, main 9425 us, walk 11.78 ms. gbisplit: sync 735 us, tex 4817 us avg.
- Picture: build3ds/lane-dl/shots/0000.ppm (frame 120) shows Mario, ground and shadow. Frames 120 and 124 are byte-identical. Two runs are byte-identical. Pre-change picture not captured. Both edits are counters, so the draw path is unchanged.

Named blocker for step 2: no pdl key is compared in the run. pdl_try returns at the class gate (dls_class must be 2 or 3) for the 38 early calls, and dlc_try answers every G_DL after frame 299. There is no miss with key inputs to log. The next question is which lists the walk reaches in class 7 (outside all dls_class ranges) and why dlc_try does not take them.

Not run: statehash (toad, battle) and golden.py (toad, yard). golden.py fails with RUN FAILED when Python starts run_azahar.ps1 (-Turbo -FaultStop). The script trap calls Release-Lock, which is not found, and the TURBO line is not printed. The same script run from a PowerShell shell passes. Starts are at the cap of 5.
