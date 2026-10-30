# agent_split0c (row SPLIT0C)

State: SPLIT0C code is in pm_gbi.c and pm_dlc.inc.c. It is diagnostic only (no draw change). One Azahar run passed (1 start of 3).

Commit: see `git log --oneline -1 -- 3ds/src/pm_gbi.c`.

What it adds (phase on only):
- sp0c line per 300 phase frames, at the same point as opsplit and pos: `sp0c f<n> tex <us> dlc <us> rest <us> skip <n> grp <n>`.
  tex, dlc and rest are us per frame. skip and grp are counts over the window.
- Periods per G_PM_MODEL run: kind 0 rest, 1 texture op (tex_op, tb_run, G_PM_TEX), 2 dlc_try (G_DL, only when dlc_on).
  A tick is read at group begin and end and at each kind change. No per-op tick.
- sp0_grp and sp0_skip count next to dlc_c.grp and dlc_c.skip (pm_dlc.inc.c), only under phase. dlc_c and the dlc2 line are unchanged.

Azahar result (mac01t_cap1, build 14:57, 1 start): `sp0c f483 tex 1714 dlc 904 rest 975 skip 27773 grp 34513`.
- Top shot 0000_1_2 (frame 500): Mario and a Toad, HUD on.
- Caveat: each tick read costs 1.3 us in Azahar. The period count per frame is not printed. tex and dlc are upper bounds.

Open items:
- The scenario mac01t_fsplit has no phase flag. The run used mac01t_cap1 (phase fsplit, same tour). Catalog row not edited.
- Tick count per frame is not logged. Add it only if the split is used for a decision.
- Path 3 and path 4 in plan_dl_design.md wait for console numbers.

Next steps:
- Console push with phase (orchestrator decides, rule 6).

Commands:
- Build: `python 3ds/tools/gen_map_restore.py`, then `sh 3ds/build.sh`, then restore the user map file.
- Test: `PM_ROW=SPLIT0C powershell -File 3ds/tools/azt.ps1 mac01t_cap1 -Row SPLIT0C -Try`
