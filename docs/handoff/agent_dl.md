# agent_dl handoff (W1, P1 done; C1 next)

## State
- Commit 7bd0205f1: W1 op-class timer and P1 producer census (pm_gbi.c, pm_interp.c, docs/handoff/diag_opsplit.md). One commit for both rows, not two as ordered.
- W1 ops gate: pass (3576 vs 3575.6). W1 class-sum gate: fail (+35%). Cause: sample window includes svcGetSystemTick cost per op. Fix open: subtract a measured per-window constant.
- P1 gate: pass (prod sum equals ops exactly). Two producers above 800 ops per frame: fn keys 0015fee8 (1206 ops) and 00163914 (837 ops). Name them from the 3dsx .map file.
- Not done: C1, C2, volcano fog A, W4+W2, W3, W6, pause menu lag row.
- Uncommitted in tree: 3ds/src/map_restore_table.c (user file, never commit) and its .bak. Restore from .bak after each build.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`, then `PM_LANE=w1 sh 3ds/build.sh`, then `cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c`.
- Yard test: `PM_LANE=w1 PM_ROW=w1 powershell -NoProfile -File 3ds/run_azahar.ps1 -Timeout 90 -LockWait 180 -Capture -Until 'opsplit f(59[9]|[6-9][0-9][0-9]|[0-9]{4,})\b' -Debug "fast preset=yard phase fixedseed shotmode=4:80"`. Log: build3ds/lane-w1/last_log.txt.
- Budget: row w1 used 3 of 8. Row c1 starts at 0.

## Next steps
1. C1 (plan_dl_architecture.md section 4 row 3): src/model.c:1523,1527 gSPDisplayList(textureHandle->gfx) becomes one port op with a TexBind record. Guard with TARGET_3DS. Test PM_ROW=c1: class 2 ops per frame down by 1000 or more, shot ok, pause menu and file select shots equal. Revert if the picture changes or the walk saves under 1.5 ms (Azahar direction).
2. C2: name producers 0015fee8 and 00163914 from the .map file, then one native op for the next producer.
3. Continue batch_queue.md Lane 2 order.
