# Row S4INST: CLOSED by ORACLE6 (2026-11-12): superseded by S7RUN + S7EFX3. Do not merge ee695cbbc / faef77191.

## Why the census line is absent (code read, main 8301c42fb)
- nd_parts_add (3ds/src/nd_rec.c:296-304) takes the parts path only when parts_tab (:286-287) finds a table with NDG_F_PARTS
  for the called list. Since S7CONV (07d0d9104) a list has a table only when its gfx source is in GCONV_SRC
  (3ds/tools/gen_gfx_tab.sh:7: sparkles effect_3D snowfall snaking_static water_block embers). energy_shockwave is not in it.
- Even with its source in the set, D_09000DC0_3762D0 (assets/us/effects/gfx/D_09000DC0_3762D0.gfx.inc.c) starts with
  PipeSync, ClearGeometryMode before its first gsSPVertex: op 0 is a state def, so the table never gets NDG_F_PARTS
  (plan_nativeapi.md section 6 item 3, the parts_ok rule).
- So every instance takes the stream form (nd_rec_state per unit, :301-302): nd_parts_run_v never runs and pr_cen
  (nd_walk.c:417-421) prints nothing. The byte gates pass because the stream form is the old path.
- The row's goal (the shockwave list drawn without the walker list run) is S7EFX3's: energy_shockwave joins GCONV_SRC there,
  and its gSPDisplayList(D_09000DC0_3762D0) becomes nd_rec_dl, drawn from the table by nd_gtab_run (S7RUN), which runs the
  ClearGeometryMode def and the loads and triangles from ops.

## Orchestrator
Stop the coder, delete branch S4INST and its worktree (wt_gc), and delete this file and row_S4INST2.md in a docs commit.
The fxpic_51 scenario and bench of ee695cbbc are not merged (S7EFX3 adds its own fxpic_20 gate).
