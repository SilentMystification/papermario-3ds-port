# Row CONV2: the converter writes native draws (no pointer into a Gfx array, no typed unit at run time)

Model: Opus (design core of row_ENDGAME.md section 2; needs the user's yes).
- Why not Sonnet: it replaces the S7 v1 table form (units in place, defs run by the walker) with a closed native form, including
  the entry state of each list and the vertex slot model resolved on the host; every static list of the game goes through it.
- Why not smaller: the form, the converter, the host compare and the device runner prove one thing together.

## Facts (ORACLE7 code read, main 235bb8721)
- S7 v1 (plan_nativeapi.md section 6): NdGTab ops NDG_VTX / NDG_TRI / NDG_DEF / NDG_UNIT / NDG_CALL; NDG_DEF and NDG_UNIT point into
  the list (`&list[k]`, nd_gfx_conv.c:153) and run on the walker (gt_ops, nd_walk.c:516) or nd_scomp_exec_own. The generated
  nd_gfx_tab.c holds 612 Gfx references. Lists enter through nd_rec_dl (270 call sites) and ent_gdl / dlc for entities and shapes.
- A def's decision depends on inherited state (inherit / ctx masks): the list does not state it; the caller's state does.
- The converter already links every set source on the host with the device defines (gen_gfx_tab.sh, gen_gfx_syms.py).

## Design (fixed; the coder chooses the inner steps)
1. Entry state: a list is drawn by `nd_draw_list(list, entry)` where entry is an NPSO description id (the caller's complete
   state). The converter emits one native table per (list, entry) pair. Pairs come from (a) a scan of `nd_draw_list(` call sites
   in src (the entry is a named ND_PSO_DESC), (b) the static call graph (a list calling a list passes its own state at the call),
   (c) record producers (models, entities, shapes) that name their entry in their record. A pair the scan cannot name statically
   is an error of the generator (it prints the call site): the family row of that file states it.
2. Native op form `NdNOp` (new nd_gtab.h v2): NDN_PSO (a table pso id from NPSO, decided at build time from the entry plus the
   list's state units), NDN_TEX (an NdBindDesc, from S7TEX2's load-group decode, now the only texture form), NDN_XF (push/load/mul
   of a matrix pointer, pop: NXF's transform ops), NDN_COL (prim, env, fog, blend colour values as uniforms), NDN_DRAW (a vertex
   base pointer + an index range into a generated u16 index buffer: the host resolves the N64 vertex slots of VTX/TRI into absolute
   indices of the Vtx arrays), NDN_CALL (another table, its own entry). Value units the host cannot know (a run-time segment base,
   a colour image switch) become explicit record inputs named in the op (the caller fills them); a unit outside the form is a
   generator error with the list name, never a unit kept for the walker.
3. Runner on core 0: `nd_ntab_run(const NdNTab*, inputs)`: one switch over NdNOp into the native emit (pso block, bind, transform,
   uniforms, indexed draw). It reads no Gfx word and calls no walker function.
4. Host compare (nd_gtab_test.c v2): for every table, run the S7 v1 path on the host model (the walker's decision code from
   nd_decide.c and the v1 ops) from the entry state and compare per draw: the pso output, the bind desc, the transform op sequence,
   and the triangle list as absolute vertex triples. Prints `ntab tables <n> draws <d> bad <b>`.
5. Pilot set (this row moves counters): the 6 base effects of GCONV_SRC (sparkles, effect_3D, snowfall, snaking_static,
   water_block, embers): their effect files call nd_draw_list (C10 down), their tables are v2, their nd_rec_dl and the v1 tables of
   these lists are gone. The other sets move in the GSET rows with the same tool.

## Readers and writers
Writers: nd_gfx_conv.c (v2 output), gen_gfx_syms.py (unchanged scan), call sites (nd_draw_list). Readers: nd_ntab_run, nd_gtab_find
(v2 key: list and entry), nd_part_draw (particles: its parts form becomes NDN_DRAW ranges; keep nd_parts working on v2), the
host compare, ent_gdl and dlc (until GSET-ENT and GSET-MAP move them). List every reader of NdGTab in the commit.

## Files and places
3ds/include/nd_gtab.h (v2 types next to v1 until S9DEL5), 3ds/tools/nd_gfx_conv.c, 3ds/tools/nd_gtab_test.c, new
3ds/src/nd_ntab.c (runner), nd_rec.c (nd_draw_list recorder only), the 6 pilot effect files. Blocker: NPSO's NdPsoDesc and
generator interface committed (CONV2 may branch after NPSO's first commit that adds them; it merges after NPSO).

## Gate (expected values)
- Build, memcheck OK. nd_host PASS with `ntab ... bad 0` and the pilot tables listed.
- LISTCEN2: nd_rec_dl count down by the pilot files' calls; the pilot effect files' gs data references to v1 tables gone (elfgate
  data: no `ndg_` symbol of the 6 pilot lists).
- `bench.py base fxpic_63` (snowfall), `bench.py base fxpic_105` (embers); one list `bench.py run fxpic_63,fxpic_105 --row CONV2
  --why "CONV2: native tables"` with psochk in a copy scenario of fxpic_63: byte-equal, psochk bad 0. Budget 8.

## Stop
A byte difference, `ntab bad` above 0, psochk bad above 0, or a generator error the design does not cover: stop, commit, hand back
(2 tries per named cause). Delete this row file in the completing commit.
