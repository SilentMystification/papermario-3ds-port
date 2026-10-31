# agent_dlx: DLX1 and DLX2 (dead and stale display-list code removed)

Plan: docs/handoff/plan_dl_cleanup.md, rows DLX1 and DLX2. Base: HEAD 9491c1222 (PF3).

## Commits (main)

- e85a85734 DLX1: delete dead RS_ counters, tb_probe, imglog, nusys tick pair.
  Files: pm_port.h (RS enum and macros), pm_gbi.c, pm_dlc.inc.c, pm_gpu.c, pm_nusys.c (gbi_task loses its kind argument), pm_phase.c.
- 6f56343ef DLX2: delete po3a, dlsrc census and mops.
  Files: pm_gbi.c, pm_dlc.inc.c (dls_hash moved here as dlc_hash, stale default-off comment replaced), azt_scenarios.json (mac01 until `opsplit f`, mops expected event removed), test_catalog.md (mac01 row).

## Gate

- Build: `gen_map_restore.py`, `PM_LANE=<lane> sh 3ds/build.sh`, then map_restore_table.c equal to its kept copy. Each build: no new warning (unique warning lines equal to the HEAD lane build).
- Picture: mac01t_cap1, top shots 0000_1_2 and 0001_1_2. HEAD run A and HEAD run B byte-equal. DLX1 (lane dlx1) 2 of 2 equal to HEAD. DLX2 (lane dlx2) 2 of 2 equal to HEAD.
- Log: mac01t_cap1 6 of 6 checks in each run. sp0c line prints in the DLX2 run.
- Azahar starts on row DLX: 4 of 8 (HEAD A, HEAD B, DLX1, DLX2). Justification lines in 3ds/logs/perf/status_DLX.txt.

## Open items

- mac01 (verified) is not run after the DLX2 until change. Its until is `opsplit f`. Run mac01 once before it is used as a gate.
- Tools 3ds/tools/dlsrc_report.py and 3ds/tools/perf_po3a.py parse log lines that no longer print. Kept (debug tools). Delete with the user's OK.
- In pm_gbi.c, the extern line for gMapShapeData, TextureHeap and the others is kept (pm_dlc.inc.c and other code use it). The Gfx mode externs of the dlsrc block are removed.
- PO3b (gpuvtx switch, graw paths in pm_gpu.c) is not in the DLX rows. Not touched.
- The working tree had uncommitted hunks in azt_scenarios.json (THUNDER-BOX frame 252 to 253) and test_catalog.md (path text). DLX2 staged only its own hunks. Those hunks stay uncommitted.

## Next

- SR1 (plan_dl_cleanup.md) starts after this row closes.
