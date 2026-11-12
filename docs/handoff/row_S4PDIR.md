# Row S4PDIR: particle instances as typed values, no op dispatch per instance (NATIVEAPI C3)

Model: Sonnet (user decides). Why not Haiku: a new record form with a decoder on core 1 and a runner on core 0, a fallback to the old
form for every shape it cannot decode, and all merged S4 callers must keep their pictures. Why not Opus: the setters exist (named
below); the order of their calls is the order of the units.

## Blocker and base
S4E3D, S4SNAK and S4WBLK merged (all S4 callers in place) and NAPI2B merged (nd_direct.c). Base `git merge-base HEAD main`.
`sh 3ds/tools/wt_new.sh S4PDIR`; PM_ROW=S4PDIR PM_LANE=S4PDIR.

## Readers and writers (grep before the edit; list them in the commit)
- NdParts writers: nd_parts_begin, nd_parts_add, nd_parts_end (3ds/src/nd_rec.c:300-324), called by snowfall.c, pink_sparkles.c,
  shimmer_burst.c, embers.c, effect_3D.c, snaking_static.c, water_block.c (src/effects).
- Reader: nd_parts_run (3ds/src/nd_walk.c) and nd_part_draw (3ds/src/nd_geom.c:348). The struct NdParts (3ds/include/nd_api.h).
- Unit forms the instances use (nd_op.h): NDO_MTX (load_mtx(a, b, 0), nd_walk.c:288-290), NDO_POPMTX (gbi_popmtx(b)), NDO_PRIM
  (nd_set_prim(a, b), nd_set.c:165), NDO_ENV (gbi_setenv(b)), NDO_TILESIZE (nd_set_tilesize(a, b), nd_set.c:178), NDO_CALL (the
  list). The callers' units: snowfall MTX MTX CALL POP (pre 2); pink and shimmer MTX PRIM CALL POP (pre 2); embers MTX TILESIZE
  TILESIZE CALL POP (pre 3); effect_3D PRIM MTX CALL POP (pre 2); snaking_static PRIM MTX TILESIZE TILESIZE CALL POP (pre 4);
  water_block MTX CALL POP (pre 1). Check each against the merged code before the edit (pre-flight).

## Design
1. nd_rec.c nd_parts_end: when every unit of every instance in the run is one of NDO_MTX, NDO_PRIM, NDO_ENV, NDO_TILESIZE, NDO_POPMTX,
   NDO_CALL (the list at index pre), record the run with a new runner `nd_parts_run_v` and the same payload. Otherwise keep
   nd_parts_run. The payload does not change (the decode is a check only), so no new record layout and no new reader.
2. nd_walk.c nd_parts_run_v: the same frame as nd_parts_run (PM_WALK_OWN, pm_walk_side, gv_epoch, dlc_on, gp_prev_op, pm_comb_vprim
   and sc_ok handling, the streak flushes). Per instance, per unit in order, call the setter directly by the unit's id (a switch of
   six arms, no nd_walk_state, no op table). The list: nd_part_draw, else nd_ops_run as now. Each setter is the same function the op
   table calls, so the state after each unit is the same.
3. Keep nd_parts_run for undecodable runs. Count runs per runner in a census line every 300 frames (`partsrun v <n> old <n>`).

## Risks
- The op table may do work around a handler (the DIP2 skip test at nd_set.c:131-135, the gp_prev_op bookkeeping, PH counters).
  Read nd_walk_state's loop and copy only what changes state. Leaving out a sync invalidation gives a stale combine. The byte gate
  catches it.
- NDO_MTX: the cold bad-matrix check (gbi_cold_bad_mtx) stays.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- One list: `bench.py run fxpic_63,fxpic_105 --row S4PDIR --why "C3: particle units by setter"` after `bench.py base` of each at
  the merge-base: every shot byte-equal (snowfall: MTX MTX; embers: TILESIZE x2).
- Census (PM_FNCOV lane): `bench.py run fxfncov_base,fxfncov_63 --row S4PDIR --lane S4PDIR_fncov`. Expected: nd_walk_state calls
  from the run go to 0 (effect minus base at most 2); load_mtx, nd_set_tilesize and gbi_popmtx counts unchanged; partsrun old 0.
- Budget 6 Azahar starts. Console: CONSOLE C3 (XL `fxtest=all`) after S4SPLASH and S4INST, not here.

## Stop
A byte difference or `old` above 0 for a merged caller: stop, commit, hand back the values (2 tries per named cause). Delete this
row file in the completing commit.
