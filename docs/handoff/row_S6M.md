# Row S6M: map custom gfx (src/world builders and their static lists) through compiled ranges and native records (NATIVEAPI F3)

Model: Haiku per file (oracle 4: split). Written: row_S6M1.md (mac_01 crystal ball), row_S6M2.md (flo_10 waterfx), row_S6M3.md
(mac_00 water_gfx), row_S6M4.md (isk_05/13/14 ambush setup). Next (oracle 5), one Haiku row per file group in the order of the
gbi counts: dro_02 npc_merlee (card draws: static lists plus a CI4 tile load per card), hos_05 intro, end Credits,
KnockDownPlayer, trd_05/06, mgm_00 scoreboard, kmr_20 records, then the small files (<= 8 calls). Each uses the S6M1 form for
its state lines; list draws stay ops until S7MAP. This file stays as the index until the last batch.

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
