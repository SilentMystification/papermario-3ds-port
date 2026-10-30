# agent_con: epic CON state (from plan_con_gate.md, console push build 1579, HEAD f56fc77ad)

Rules: CLAUDE.md 2, 5, 7; docs/handoff/hw_guidelines.md; plan_con_gate.md sections 4 and 5.

## Commits
- 029a952fd LB1b: run the background gbi task with the swap task (pm_nusys.c only, includes the FS_GBI label fix).

## State
| Row | State | Change | Azahar mac01t_fsplit | Shots |
|---|---|---|---|---|
| LB1b | DONE, committed 029a952fd | 3ds/src/pm_nusys.c: bg task (no swap, pm_interp_on <= 0) recorded in bg_list, run at the swap task after the main list build; pm_gfx_bg_flush at the end of pm_gfx_retrace and pm_gfx_retrace_skip. FS_GBI mark after pm_gpu_begin_wait in gbi_task (label only). | LB1b run 1 (lane conlb1b2): log 4 of 4; gbi 6.9 ms, rest 23.4 ms (label fixed: was gbi 0.0, rest 23.4 before the fix); tk 2.00; wall 33.4 ms | 2 of 2 top shots byte-equal to HEAD |
| HEAD check | DONE | conhead2 run 1 (lane conhead, HEAD f56fc77ad): log 4 of 4, gbi 6.7, rest 23.6, wall 33.4 | HEAD run 2 byte-equal to HEAD run 1 (2 of 2) |
| MR1 | NOT CODED, needs a plan | G_PM_MODEL (plan_con_gate section 5) | n/a | n/a |

## LB1b gate (passed)
- HEAD reproduces: conhead2 run 2 equals conhead run 1 for 0000_1_2 and 0001_1_2.
- LB1b equals HEAD for 0000_1_2 and 0001_1_2 (copies: build3ds/azt/shots_conhead2_A, build3ds/azt/shots_conlb1b2_A).
- Log checks 4 of 4 on both runs. tk 2.00 unchanged.
- Azahar gwait stays 0.0. The console gwait gate (2.3 to 2.6 ms before) is measured at the epic push, not here.
- Budget used after the orchestrator reset: LB1b 1 of 8, conhead2 1 of 8.

## MR1: not coded (open)
- plan_con_gate section 5 gives one line of design: "one G_PM_MODEL op with a per-model record for plain static models; a differing key runs the original list". It does not give the record layout, the key fields, or how the op count is taken. Those details decide the gate result.
- Facts from the code:
  - appendGfx_model (src/model.c 1384 to 1657 body, tail to 1576 of the tail block) emits about 8 to 12 ops per plain model: pipe sync, texture bind (MODEL_TEX_OP, G_PM_TEX), combine mode, render mode (MODEL_RM_OP, G_PM_RM), G_MTX with model->finalMtx (a pointer, changes per frame by value), G_DL to modelNode->displayData->displayList, pop matrix, pipe sync.
  - The walker (3 ds/src/pm_gbi.c G_DL case 3523, G_MTX 3655, G_PM_RM 3516 with rm_apply 2805) already runs the C1 and C2 records. A G_DL to a host list runs inside the same walker loop, so its ops are still counted. A record run as G_DL would not lower the op count. A fused op needs a new walker case that runs the record in one dispatch.
  - MR0 census (agent_model.md): outside dlc 37 us Azahar per plain model (ops 17, bind 10 to 11, close 8 to 9). The op part is the target. Console x2.86: about 107 us outside dlc per model, 86 models.
- Gate (plan): mac01t and 6 maps byte-equal; prod model ops down 600 or more. Scenario mac01t. Revert on a changed shot or ops not down by 600.
- Next step: Opus writes the record layout and key (model pointer plus modelID, textureHandle gfx, renderMode, renderClass, tintCombineType, flags, customGfxIndex, lights, finalMtx pointer, map change flush), and the op counter rule, then a code agent codes it. The change touches pm_gbi.c (shared draw code), so the other scenes need a byte-equal check.
- Not started: no code, no starts, 0 of 8 MR1 starts used.

## Open items
- MR1 plan from Opus (above).
- Epic push (plan_con_gate order item 3) needs the console fsplit and pmu run. It is not requested yet. Do not give the push command before the epic is complete.
- map_restore_table.c is the user file. Backup: C:\Users\Andrew\AppData\Local\Temp\claude\c--Users-Andrew-Documents-GitHub-papermario-3ds-port\d6d00d1c-7fdf-4fe7-b586-9f6c1655ea55\scratchpad\map_restore_table.user.c. It was restored after the LB1b build. Never commit it.
- Other uncommitted files in the tree belong to other work. Not mine.

## Commands
- Build: `/c/Python314/python 3ds/tools/gen_map_restore.py`, then `PM_ROW=LB1b sh 3ds/build.sh`, then restore map_restore_table.c from the backup above. Copy build3ds/pm_3ds.3dsx to build3ds/lane-<name>/.
- Lanes: build3ds/lane-conhead (HEAD f56fc77ad), build3ds/lane-conlb1b (LB1b before the label fix, superseded), build3ds/lane-conlb1b2 (LB1b 029a952fd).
- Test: `PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 mac01t_fsplit -Row <row> -Build <lane> -Try`.
- Compare: `cmp` each top shot.
