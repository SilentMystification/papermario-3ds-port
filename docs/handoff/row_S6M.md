# Row S6M: map custom gfx (src/world builders and their static lists) through compiled ranges and native records (NATIVEAPI F3)

Model: Sonnet (user decides).
- Why not Haiku: src/world has 416 gbi calls in custom gfx builders (build_custom_gfx callbacks, model record R11 ranges,
  nd_range_run in nd_walk.c:438-453) across 45 map files. Each builder mixes state, values and draws.
- Why not Opus: S6T's form plus the direct helpers cover them. This row splits the 45 files into batches of about 10 files
  (S6M-1 to S6M-4, one worktree each, different files).

## Blocker and base
S6T and S6E merged. `sh 3ds/tools/wt_new.sh S6M1` (and S6M2, S6M3, S6M4 for the other batches).

## Code read
Custom gfx ranges are recorded by the model path (DIRECT D10a: nd_range_run runs a custom gfx range of the frame stream to its
NDC_END). Producers: grep `custom_gfx` and `gSP` in src/world. The largest: mac_01 crystal_ball_gfx.c (32 ops a frame in mac01),
kzn_08 and hos_01 (panner and shroud scenes).

## Design per builder
State lines become an S6T range id; values (prim, env, tile sizes of panners, matrices) become value units; draws of static lists
go through S6E's compiled entity form (a static list is an entity-like list); draws of quads or rects use the direct helpers.
The batch row lists its files in the commit.

## Gate (expected values, per batch)
- Build, memcheck OK; nd_host PASS.
- DIRECT byte set plus the batch's map scenes (kzn_08, hos_01 or the map of each file; pre-flight `bench.py spots --map <m>` for
  a det scene; add a det spot scene per map if missing). At most 4 starts per batch. Every shot byte-equal.
- listcen: src/world build count down by the batch's calls. Budget 8 per batch.

## Stop
A byte difference: stop, commit, hand back. Delete this row file in the commit of the last batch.
