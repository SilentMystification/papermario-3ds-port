# Row S4INST: one particle run per run of render tasks of the same effect (NATIVEAPI C4; order rule by oracle 3)

Model: Sonnet (user decides).
- Why not Haiku: the run crosses the render task loop in model.c and needs the order, reflection and setup-list rules below. The
  setup-list check (rule 3) is a read of two static lists, and its result decides the run form.
- Why not Opus: the order rule is written here; the run uses the S4PDIR setter form.

## Blocker and base
S4PDIR merged (per-instance PRIM, ENV, TILESIZE, MTX units through setters, nd_parts_run_v). Then `sh 3ds/tools/wt_new.sh S4INST`;
PM_ROW=S4INST PM_LANE=S4INST.

## Code read
- Effects queue render tasks (energy_shockwave_render, energy_shockwave.c:127-138: dist 10, RENDER_MODE_SURFACE_XLU_LAYER3, and
  RENDER_TASK_FLAG_REFLECT_FLOOR).
- execute_render_tasks (src/model.c:5181-5320) sorts by dist: ascending, with the >= 3M and <= 800k bands descending. Then
  bat1_sort_runs (:5149) groups plain opaque model tasks only (bat1_plain, :5110-5135); effects keep the distance order. Then each
  task's appendGfx runs in sorted order:
  - with GLOBAL_OVERRIDES_ENABLE_FLOOR_REFLECTION: each REFLECT_FLOOR task is wrapped in nd_range_begin / nd_range_repeat, so its
    range is drawn again flipped, right after it (:5271-5292);
  - else the plain loop (:5293-5305).
- energy_shockwave_appendGfx (:140-170), per instance: PipeSync, gSPSegment 0x09, setup list D_09000E60, prim, env, 2 SetTileSize
  (tiles 0 and 1), matrix (translate x scale), list D_09000DC0, pop, PipeSync. There are 11 live instances in fxtest 51.

## Order rule (the result must keep the draw order of today)
1. A run is a maximal sequence of consecutive entries of `sorted` (after bat1_sort_runs) whose tasks have the same appendGfx and
   whose appendGfx is in the run table (this row: energy_shockwave_appendGfx only). Never reorder tasks: a run only joins
   neighbours, so the draw order stays the same.
2. Floor reflection: when GLOBAL_OVERRIDES_ENABLE_FLOOR_REFLECTION is set and the tasks carry REFLECT_FLOOR, do not form runs.
   Today the order is A, A-flipped, B, B-flipped. A run would give A, B, A-flipped, B-flipped, which changes translucent order.
3. Setup list per instance:
   - Instance 2..n may skip D_09000E60 and gSPSegment only if D_09000DC0 sets none of the state that D_09000E60 sets. Read both
     lists (effect data, energy_shockwave graphics) and write the comparison into the commit.
   - If DC0 sets any such state, keep the setup list per instance as a unit. S4PDIR's run form allows one CALL at index pre: then
     extend the run form so a second CALL (the setup list) is allowed before the pre units. Name that in the commit.
4. Opening: the first instance of a run opens the NdParts run (nd_parts_begin) and the last one closes it (nd_parts_end).
   execute_render_tasks marks the run: a flag `gNdRunOpen` with the run's appendGfx, set before the first call of a run and
   cleared after the last. energy_shockwave_appendGfx then calls nd_parts_add for its instance and does not begin or end the run
   itself when the flag is set. Alone (a run of 1), it behaves as today.

## Files and places
src/model.c (execute_render_tasks, the plain loop :5293-5305 only), src/effects/energy_shockwave.c (energy_shockwave_appendGfx),
3ds/include/nd_api.h (the flag declaration at the end). Follow-ups (one row each, after this merges): explosion, snowman_doll,
ring_blast. radiating_energy_orb as 4 runs only where its parts share a shape in sequence.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- Scenario and bench `fxpic_51`, a copy of `fxpic_50` with `fxtest=51` (insert after fxpic_50). `bench.py base fxpic_51` at the
  merge-base. Then one list: `bench.py run fxpic_51,spot_mac01_det --row S4INST --why "C4: shockwave instances as one run"`
  (2 starts). `bench.py cmp` both: every shot byte-equal.
- Census: PM_FNCOV build, `bench.py run fxfncov_base,fxfncov_51 --row S4INST --lane S4INST_fncov` (2 starts, both on the
  row's census lane; never `bench.py base` on fxfncov). Expected effect minus base: nd_parts_run_v about 1 per frame (it was 11),
  and nd_ops_run below 20.
- Budget 6 Azahar starts. Console: CONSOLE C3 (XL fxtest=all) after this row.

## Stop
A byte difference: stop, commit, hand back the cmp and the setup-list comparison (2 tries per named cause). Delete this row file in
the completing commit.
