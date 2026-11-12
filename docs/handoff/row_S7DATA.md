# Rows S7EFX, S7ENT, S7MAP, S7MDL: list data and its callers through the S7CONV tables (NATIVEAPI S7; one row id each, one worktree each)

Blocker for all: S7CONV merged. First step in each: read S7CONV's design note and its table API. Its host test is the proof tool
of each row.

## S7EFX (Sonnet; 4 worktrees S7EFX1 to S7EFX4, about 32 effect files each, in src/effects alphabetical order)
- Why not Haiku: each effect's appendGfx mixes its own state lines, per-instance values and calls into its gfx data, and some
  effects build vertices per frame (they stay dynamic: nd_mesh).
- Why not Opus: the table form is S7CONV's.
- Scope per file: the effect's D_* gfx data comes from the tables. appendGfx calls nd_particles, nd_mesh, nd_quad or
  nd_draw_table with the instance values. No gSP/gDP call stays in the file.
- Gate per batch: the converter host test for the batch's lists; the fxpic scenes of the batch's effects that exist (add
  `fxpic_<idx>` copies of fxpic_50 for the batch's most expensive effects: at most 4 new, from the fxcen table); DIRECT byte set;
  listcen src/effects build and data for the batch 0. Budget 8 each. Console: C4 (XL fxtest=all).

## S7ENT (Sonnet)
- Scope: src/entity (169 calls) and the entity data in assets. Entity static lists through the tables; S6E's compiled ranges come
  from the converter instead of ent_compile at run time.
- Gate: DIRECT byte set; a block, chest or spring scene (pre-flight `bench.py spots`); listcen src/entity 0. Budget 8.

## S7MAP (Sonnet)
- Scope: the map shape lists. nd_shape.c converts N64 shape lists at map load today, and pm_map.c reads .nds shapes. The
  conversion moves into the build (the shape files hold native meshes and table ids). Also the remaining map custom gfx data of
  S6M.
- Gate: the DIRECT byte set (all three are maps) plus kzn_08 and hos_01 det scenes; the map-load census (DPCEN2 compus) shows no
  list compile at load; listcen reader: nd_shape conversion code gone from the runtime. Budget 8.

## S7MDL (Opus)
- Why Opus: model.c (125 calls) holds the render-mode lists Gfx_RM1 to Gfx_RM3 (about 32 lists, used by models, entities and
  animators), the custom gfx builders and the env tint paths (shroud, depth, remap: model.c:2071-2083, :1577). The model record
  cache (KEYMISS keys) depends on them. A wrong pso key here changes every map.
- Scope: the render-mode lists become pso descriptors from the converter. The model setup records pso ids and values, not lists.
  The tint paths become values in the record.
- Gate: DIRECT byte set; kzn_08 and hos_01 (tint and panner); the keymiss census unchanged (KEYMISS bench); listcen model.c 0.
  Budget 8. Console: C4.

## Stop (all)
A byte difference or a host test mismatch: stop, commit, hand back. Each row deletes its own section of this file in its
completing commit; the last one deletes the file.
