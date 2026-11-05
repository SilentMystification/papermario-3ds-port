# agent_bat1: row BAT1 (sort opaque run by texture) - result: PASS, commit c4ebe5451

## State
- src/model.c: bat1_plain, bat1_key, bat1_sort_runs (before execute_render_tasks); one call before gLastRenderTaskCount.
- Plain = MR0/MS0 census test: appendGfx_model, renderMode SURFACE_OPA, no custom gfx, panner, billboard, no tint, no fog, no floor reflection.
- Only plain tasks are reordered. Each run is stable insertion-sorted by textureID + textureVariation (0 if textureID is 0).
- src/model.c: 54 lines added. No new warning (linker enum warning count equal at HEAD and BAT1, 3312 each).

## Measurements (mac01t_cap1, phase fsplit, window f483, one run each)
| run | lane | ms0 removable closes per frame | sp0c tex us per frame | pictures |
|---|---|---|---|---|
| HEAD | bat1h | 14 | 1907 | - |
| BAT1 | bat1m | 1 | 1813 | 0000 and 0001 byte-equal to HEAD |

- ms0 texture changes 46 -> 33 per frame. Opaque plain models 69, runs 12 (same).
- Log checks 6 of 6 in both runs.
- Yard gate PASSED: top shot 0000_0_11 equal to HEAD (e5802e650) for BAT1 c4ebe5451. HEAD repeat (start 5) also equal to HEAD. Bottom shots differ 60 of 60 for HEAD vs HEAD repeat too, so they are noise. Azahar starts used: 5 of 8.

## Open items
- Yard byte-equal check done (top shot). No open item for BAT1.
- Console judges speed. Azahar numbers are direction only.
- Next row: TBREP (DUP0 counts first, per plan).

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`; `PM_LANE=<lane> PM_ROW=BAT1 sh 3ds/build.sh`; restore 3ds/src/map_restore_table.c from a copy first (never commit it).
- Test: `PM_ROW=BAT1 powershell -File 3ds/tools/azt.ps1 mac01t_cap1 -Row BAT1 -Build <lane> -Try`.
- Quantity: `grep -E "ms0|sp0c"` in %APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt.
