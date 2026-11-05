# agent_model: MODEL epic (E4): MR0, MR1, MS0, MS1

## State (build lane mr0, git fb9e8f0fc)
- MR0 done. Commit 38c25809b. Census only (phase on, cold). Gate NOT met: MR1 not run.
- MS0 done. Commit fb9e8f0fc. Upper-bound census. Gate NOT met: MS1 not run.
- MR1 and MS1 not coded. Reason: census gates failed (below). No draw change in this epic.

## MR0 result (mac01t_mr0, lane mr0, Azahar, window f599 to f899, 300 frames)
- Plain static model instances: 86 per frame (gate 50: met).
- Per model, us (Azahar): ops 17, bind 10 to 11, close 8 to 9, outside dlc 37, dlc 12.
- Console estimate (Azahar x 2.86): outside dlc 107 us. Gate 120 us: NOT met (11 % short).
- Two runs (runs 2 and 3, same binary) agree within 2 us per term.
- Run 1 (discarded): dlc hook was not set, so dlc time landed in ops. Fixed in run 2.
- Not done: the 4-way dlc split (box test, matrix id, ensure_tile hit, index write). The gate does not need it. Add it only if a later row needs the dlc terms.
- Unit check: ticks run at 268 MHz (TICKS_MS 268112). pm_fl_us already carries the x8 sample scale.

## MS0 result (same run 3, until ms0 f899)
- Opaque plain models: 70 per frame. Runs: 13 per frame. Texture changes: 46 per frame.
- Removable closes (upper bound): 15 per frame. Gate 20: NOT met. MS1 not run.
- The count does not check non-model ops between marks. A true count is at most 15.

## Why MR1 is not coded
- The census says the outside-dlc cost is 107 us console per plain model (gate 120). A per-model op would save at most the dispatch and part of the bind. The gain then depends on the 86 models x 107 us residue, not on a measured saving. The gate is the epic's rule; it failed. Do not code MR1 without a new plan.

## Open items
- Picture: the mac01t_mr0 shots are not compared byte-wise with HEAD. The census is phase-only, so no draw change is expected. A person must confirm the top shot (build3ds/azt/shots/mac01t_mr0/0000_1_2.ppm).
- Regression catalog on lane mr0: see FINAL_WRITEUP MODEL section and catalog_model.out (scratchpad).
- Budget: row MR0 used 3 starts (2 for MR0, 1 for MS0 run under row MS0 label). Row MODEL used for the catalog.
- No push. The epic is not complete for a console check (census only, no code path changed for the picture).

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_LANE=mr0 PM_ROW=MR0 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c from the user copy (never committed).
- Census: PM_ROW=MR0 powershell -File 3ds/tools/azt.ps1 mac01t_mr0 -Row MR0 -Build mr0 -Try (until ms0 f899; lines mr0 and ms0 both in the log).
- Log lines: `mr0 f899 plain N per frame | per model us: ops .. bind .. close .. outside dlc .. dlc .. | console x2.86 outside dlc ..` and `ms0 f899 opaque plain models N per frame, runs N per frame, removable closes N per frame (texture changes N per frame)`.
