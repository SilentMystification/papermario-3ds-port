# agent_texd3: row TEXD3 (branch native), wrap table and switches

## State
- Commit 3e030aed7: switches wrapov, texnearest, padov (pm_tex.c, default path unchanged), texwrap_wall scenario, catalog rows, docs/handoff/texwrap_semantics.md (case table, file:line).
- Default byte-equal to HEAD: yard 0000_0_11, mac_01 0000_1_2 and 0001_1_2.
- Wall 0x0100169c binds at frame 200 with no switch: wrap s 3 t 3 (MIRRORED), filter 1.
- Wall with wrapov=repeat: td3 line programmed ws 2. Picture change inside run-to-run noise (17355 and 17755 px).
- No fix committed. The default mapping for the chunk #27 tile (cms 0, masks 5) is REPEAT and exact. The bound tile is not that tile. Cause not found.

## Open
1. Log tile index and tst1.tile_wrap[n] at pm_tex_bind for slot 8 (0x0100169c). Decides stale wrap (pm_tex.c:1616, pm_gbi.c:2583) or another draw.
2. texreg_coin: no position with coins drawn yet. Gate not run.
3. Clamp with mask window (semantics case 4, case 7): port folds by mask. Not tested.
4. texreg_wall and texreg_coin: still unverified in the catalog.

## Budget
- TEXD3: 8 of 8 starts used (1 yard base, 2 mac01 base, 3 hos01 default, 4 accidental repeat of hos01 without a status line, 5 and 6 wrapov, 7 yard, 8 mac01).
- Reset with 3ds/run_azahar.ps1 -ResetBudget TEXD3 before more starts.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py` (/c/Python314/python), then `PM_LANE=native PM_ROW=TEXD3 sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from a copy.
- Test: `PM_ROW=TEXD3 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 texwrap_wall -Row TEXD3 -Build native -Try`
