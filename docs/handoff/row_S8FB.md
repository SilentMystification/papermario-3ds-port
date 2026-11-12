# Row S8FB: delete every old-op fallback of the direct executors (NATIVEAPI S8)

Model: Sonnet (user decides).
- Why not Haiku: each fallback guards a walker state the direct path refuses (nd_direct_ready, a NULL pso decision, a bind
  refusal). Removal is safe only when its counter is 0 in every scene, which the coder proves per helper before deleting it and
  the state it guarded.
- Why not Opus: deletion only.

## Blocker and base
S5 (all rows), S6 and S7 merged. `sh 3ds/tools/wt_new.sh S8FB`; PM_ROW=S8FB PM_LANE=S8FB.

## Fallbacks to delete (grep each before the edit; list them in the commit)
- hud_rect_emit + nd_walk_state in src/hud_element.c (NAPI2B);
- the msg_draw fallback (NAPI3B);
- the fill and quad fallbacks (S1FILL, S5PQUAD, S5CHUD);
- the stencil fallback (S1TSTEN);
- every `napi* fallback` counter site;
- nd_part_draw's nd_ops_run fallback in nd_parts_run (nd_walk.c);
- ent_run_seg_interp (nd_ent.c:386);
- nd_parts_run (the old runner), once S4PDIR's nd_parts_run_v covers every caller.

## Steps
1. The counters are the gate input: the byte scenes of the S5 to S7 rows already print each fallback counter. One list of those
   scenes, at most 8 starts. Every counter must be 0. A counter above 0: stop and hand back its scene and why value; that path
   needs its own row.
2. Delete the fallbacks and their counters. nd_direct_ready and the NULL decision become an assert that prints
   `napi refused <why>` once (cold), not a silent fallback.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- One list (7 starts): DIRECT byte set, battle_wheel_nowide_det, pause_tab_det_pic, the talk det scene and fxpic_63. Every shot
  byte-equal. No `napi refused` output.
- listcen reader down by the deleted fallbacks.
- Budget 8 for step 1 and 8 for the gate. The orchestrator splits the row into two worktrees.

## Stop
A counter above 0 or a byte difference: stop, commit, hand back. Delete this row file in the completing commit.
