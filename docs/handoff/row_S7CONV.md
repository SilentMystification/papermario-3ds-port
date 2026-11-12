# Row S7CONV: build-time converter of static display-list data into native tables (NATIVEAPI S7, design and tool)

Model: Opus (user decides).
- Why not Sonnet: this is the core design of the data side of the end state. All static list data (853 asset files and 100 src
  files with `Gfx X[]`, about 8053 gs macros: effects gfx, entity data, map data, ui) must leave the runtime as N64 words and
  become native tables. Its key and its handling of run-time state decide whether S7EFX, S7ENT, S7MAP and S7MDL can be exact.
- Opus may choose its steps inside the gate. Write the design note into this row first.

## Blocker and base
S6T, S6E and S6M merged (the compiled range form and its key exist; the converter emits them at build time).
`sh 3ds/tools/wt_new.sh S7CONV`; PM_ROW=S7CONV PM_LANE=S7CONV.

## What exists
- nd_shape_conv: the build-time typed conversion of map shapes (host side; build.sh step).
- The gs macros already compile to typed units (nd_gbi.h ND_GS).
- nd_host builds parts of the state model (nd_comb, nd_tmpl) for the host tests.
- S6T's compiler (nd_scomp.c) compiles a typed range under an inherited signature on core 0, cold.

## Design questions (the design note)
1. Input: the typed units of each static Gfx array (link the game data objects into a host tool, or parse the generated
   .gfx.inc.c sources).
2. Output per list: native records. Choose between C tables linked into the ELF and a binary blob. Each table holds: pso
   descriptors (pso ids made at boot), NdBindDesc (texture address relative to the segment), vertex buffers (the list's Vtx data
   as is, or converted to the native layout), index lists, matrix and segment slots (filled at draw), and the draw order. The
   per-frame form of a draw from a table: `nd_draw_table(table id, segment bases, matrix, values)`.
3. Run-time dependence:
   - segments (0x09 effect graphics base per effect instance, the entity vertex segments);
   - inherited state (a list that does not set all state it reads: the compiled key must carry the inherited part, as S6T keys
     it);
   - lists the game edits at run time (palettes, imgfx meshes, vertex colour edits): these stay dynamic (S5IMGFX form). List them.
4. Proof: a host test runs each converted list through the old walker (the host build of nd_walk and the state model) and through
   the native table executor, and compares the emitted draw words (pso, texture, vertices, matrices) per draw.
5. Build: a build.sh step after the asset generation. Never committed output, like map_restore_table.c.

## First use in this row
The effects with merged NdParts callers (snowfall, pink_sparkles, shimmer_burst, embers, effect_3D, snaking_static, water_block):
their static lists come from tables. nd_part_draw then reads the table, not the list words.

## Gate (expected values)
- nd_host PASS with the converter host test: every converted list of the first-use effects equal on both paths.
- fxpic_63, fxpic_105 and fxpic_124 byte-equal (one list, 3 starts), plus the DIRECT byte set (3 starts).
- listcen: data for the converted effects 0 in the runtime build (the tables replace the arrays). Budget 8.

## Stop
A byte difference or a host test mismatch: stop, commit, hand back (2 tries per named cause). The design note stays in this row.
Delete this row file in the completing commit, and list the S7EFX batches in the hand-back.
