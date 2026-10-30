# agent_drec1 (row DREC1)

State: BLOCKED on a decision. No source edit, no build, no Azahar start. Start count 0 of 8. The gate did not run.

Why no code (HEAD 20703d076):
- Plan section 5 says the apply recomputes each handler decision from live state. The handler work is the cost, not the dispatch.
  tex_op SETTILESIZE tile 0 (pm_gbi.c 3010 to 3218) derives src, stride, bpp, the ldrec ring scan, pm_tex_glyph_same, the tsz_last test and
  gp3_t0_set. It also calls suntile_t0 (3165). Plan 6 omits suntile_t0 from the apply list. An exact apply needs every one of these.
- tex_op SETTILE (2932 to 2957) calls pm_gpu_flush on a tile 0 change. The flush is inside the sp0c tex period (SPLIT0C2 flush 406 of tex 1912 us, Azahar).
- Dispatch share, from plan_mr2_record section 1 and agent_drec1 earlier note: about 50 to 100 us of tex per frame (3 to 5 percent of the 1912 us tex period).
- The 40 percent gate on sp0c tex needs about 765 us less. The flush (406 us) stays. The handler work (about 1500 us) must go. A snapshot that
  still runs the handler decisions does not remove that work. Therefore the gate is likely to fail as designed.
- The gate is not run. A drop needs a valid gate result (CLAUDE.md rule 5 and the row text). This is not yet a drop.

Decision needed (orchestrator or user):
- (a) Drop DREC1 now on this analysis, and go to the next path (TEXI1 or the batch-close row in plan_dl_design.md).
- (b) Build the full apply (plan section 5 plus suntile_t0, about 200 lines, tbcheck included) and run the gate. About 7 Azahar starts.

Diagnostic data kept: SPLIT0C2 Azahar (mac01t_cap1, 1 start): sp0c f483 tex 1912 dlc 902 rest 970 flush 406 skip 27773 grp 34513.
Caveat: tick reads cost about 1.3 us each in Azahar. The tex value is one run, not a like-for-like delta.

Commands (if (b) is chosen):
- Build: `python 3ds/tools/gen_map_restore.py`, `sh 3ds/build.sh`, restore 3ds/src/map_restore_table.c.
- Test: `PM_ROW=DREC1 powershell -File 3ds/tools/azt.ps1 mac01t_mr0 -Row DREC1 -Try`
