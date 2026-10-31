# agent_gen0: row GEN0 (drop unused PIPESYNC and repeated SETCOMBINE) - result: FAILED quantity gate, reverted, no commit

## State
- src/model.c: edit made (2 gDPPipeSync per model under #ifndef TARGET_3DS), built, tested, then reverted with Edit. git diff is empty.
- src/entity/Shadow.c: not changed.
- Nothing committed. map_restore_table.c restored from a copy after each build (not committed).

## Trace (walker, 3ds/src/pm_gbi.c)
- G_RDPPIPESYNC is in g1_skip (line 2774). The main loop skips it at 3650 before the prologue and before os_op_hook (3673).
- mg_run skips it at 3564 before os_nn. tb_inline (2817) and pm_texbind_set (2805) skip it too.
- pm_dlc.inc.c (141) breaks on it. The dispatch switch (3947) has an empty case.
- So the removed word has no effect on state. The walker never counts it.

## Measurements (mac01t_cap1, phase fsplit, PM_ROW=GEN0)
| run | lane | shot 0000_1_2 | shot 0001_1_2 | opsplit f483 ops |
|---|---|---|---|---|
| HEAD 1 | gen0h | baseline | baseline | 1186 |
| HEAD 2 | gen0h | equal HEAD 1 | equal HEAD 1 | not read |
| mod 1 | gen0m | equal HEAD 1 | equal HEAD 1 | 1186 |

- Log checks 6 of 6 in all runs. The opsplit sync class reads 0/0 at HEAD and in mod.
- Quantity gate failed: ops per frame did not fall. The removed words are skipped before the counter.
- yard not run (the change was reverted).

## Open items
- The row cannot show its quantity in opsplit. Its saving is a few skip iterations per frame, under 0.05 ms by the plan estimate.
- Plan error: Shadow.c header count. The 3DS circle block has 18 words after the header, with the 17-word square layout plus one repeated SETCOMBINE. The plan says 18 to 16. Removing the two repeated SETCOMBINE gives 17 (the header covers the word that is the first SETCOMBINE after the tile load).
- Shadow.c repeated SETCOMBINE: g2_same skips it before the counter, so it has the same blind spot.
- Decision for the orchestrator: keep GEN0 closed (no measurable quantity) or add a counter for g1/g2 skips (RS_SKN and RS_SKS exist but are not printed).

## Starts
- 3 of 8 used (gen0h x2, gen0m x1). Budget not reached.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`; `PM_LANE=<lane> PM_ROW=GEN0 sh 3ds/build.sh`; restore map_restore_table.c from a copy.
- Test: `PM_ROW=GEN0 powershell -File 3ds/tools/azt.ps1 mac01t_cap1 -Row GEN0 -Build <lane> -Try`.
