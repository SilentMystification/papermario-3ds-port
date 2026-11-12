# Rows S7EFX, S7ENT, S7MAP, S7MDL: list data and its callers through the S7CONV tables (NATIVEAPI S7; one row id each, one worktree each)

Blocker for all: S7CONV merged. First step in each: read S7CONV's design note and its table API. Its host test is the proof tool
of each row.

## S7EFX (oracle 5: written as row_S7EFX1.md to row_S7EFX8.md, Haiku, after row_S7RUN.md; the table runner and nd_rec_dl are
## the design core, S7RUN, Sonnet)

## S7ENT (Sonnet; oracle 5: write after S7RUN merges)
- Design question first (in the row): entity lists read vertices and sub-lists by segment address (useseg, ent_segkey; data
  loaded from ROM at run time: 3ds/tools/gen_entity_rom.py), so the S7CONV tool maps none of their pointers. The row adds a host
  load of the entity data at the device link addresses (or relocatable tables keyed by segment) before any call site moves.
- Scope: src/entity (169 calls) and the entity data in assets. Entity static lists through the tables; S6E's compiled ranges come
  from the converter instead of ent_compile at run time.
- Gate: DIRECT byte set; a block, chest or spring scene (pre-flight `bench.py spots`); listcen src/entity 0. Budget 8.

## S7MAP (Sonnet; oracle 5: after S7RUN and S7ENT's segment answer)
- nd_shape_conv already runs on the host (build.sh gen_shapes): the row makes it emit NdGTab-form ops into the .nds files.
- Scope: the map shape lists. nd_shape.c converts N64 shape lists at map load today, and pm_map.c reads .nds shapes. The
  conversion moves into the build (the shape files hold native meshes and table ids). Also the remaining map custom gfx data of
  S6M: dro_02 npc_merlee, hos_05 intro (image strips need a scissor-clamped NdTRect form), end Credits (row_S6M.md).
- Oracle 5: these sections become Haiku or Sonnet rows per effect group, file group and map group when the S7CONV design note
  (row_S7CONV.md) and its table API exist; the census expectation of each row is the listcen data/build counts of its own files.
- Gate: the DIRECT byte set (all three are maps) plus kzn_08 and hos_01 det scenes; the map-load census (DPCEN2 compus) shows no
  list compile at load; listcen reader: nd_shape conversion code gone from the runtime. Budget 8.

## S7MDL (Opus; oracle 5: after S7RUN and S6KEY; the RM defs of S5EMDL1 (Gfx_RM*_def) are the start of its pso descriptors)
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
