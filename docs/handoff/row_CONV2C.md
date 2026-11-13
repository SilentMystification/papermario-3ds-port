# Row CONV2C: the converter writes native tables v2 (one NdNTab per static list and entry state), host compare (Sonnet)

Model: Sonnet (plan v3, accepted by the user). Reason: one algorithm over every static list (the entry state, the call graph,
the vertex slots, the state units to a pso), and its error cases are not known per list before it runs. Why not Opus: the form
(CONV2A NdNOp/NdNTab), the slot resolver (CONV2B nd_vslot.c) and the decision (NPSO2 nd_pso_build) exist.

## Facts (ORACLE8, main ac0e68cd5)
- Today: 3ds/tools/gen_gfx_tab.sh (19 lines) runs 3ds/tools/nd_gfx_conv.c (177 lines) over the lists named by
  3ds/tools/gconv_src.d/*.txt (000_base.txt, S7EFX1..8, S7ENT ...) and writes the S7 v1 tables (NdGTab, ops pointing into the
  list units) into the generated 3ds/src/nd_gfx_tab.c. nd_host checks them with nd_gtab_test.c against the old list reader.
- v2 form (nd_gtab.h, CONV2A): NdNOp {op, a, b, v, p, idx_pos} with NDN_END, NDN_PSO (b = NdPso id), NDN_TEX (p = NdBindDesc*),
  NDN_XF (a = flags, 0xFF pop), NDN_COL (a = which, v = RGBA), NDN_DRAW (p = Vtx base, b = index count, idx_pos), NDN_CALL;
  NdNTab {list, entry, nops, ops, idx}. CONV2B: nd_vslot_resolve (VTX and TRI ops to absolute index triples, draws per array).
- NPSO2: nd_pso_build(const NdPsoDesc*, NdPsoOut*). NPSO3 (row written): build-time table nd_pso_tab.c and nd_pso(id); the ids
  come from ND_PSO_DESC in source. A converter-made pso needs the same table: CONV2C writes its descs into a file that
  gen_pso_tab.py scans (3ds/src/nd_ntab_descs.c, generated) before it numbers the ids.
- Not in main yet: nd_ntab_run (CONV2D, the runner), nd_draw_list (CONV2F, the call site API).

## Correction 1 (ORACLE8, after the coder's read): scope = the lists that state their own pipeline state
- No entry state exists for a v1 list: nd_gtab_test.c main (:189-206) only checks the tables, NdGDef.inherit and ctx (nd_gtab.h)
  only name the groups the live walker gives at run time. 544 of the 1372 lists draw before they set a combine (16 of 24 in
  000_base.txt), with state that callers and earlier lists built at run time (src/effects/snowfall.c:199-201 and
  src/effects/gfx/snowfall.c:13-17). No call site names an entry yet (CONV2F and the entry rows are later).
- Decision: this row converts only the SELF-CONTAINED lists: before every draw the list itself has set every group that
  NdPsoDesc holds (combine, other mode H and L, geometry mode, texture on, the tile 1 role): the NdGDef inherit mask of those groups
  is 0 at each draw. Their table needs no entry. An inheriting list gets no v2 table: the
  generator lists it in build3ds/out/ntab_inherit.txt (list name, the inherited groups, the first draw op). Never a v1 fallback
  inside a v2 table.
## Design (fixed. The coder chooses the inner steps)
1. Per self-contained list one NdNTab with entry 0 (the table states every group: the entry is not read; nd_ntab_run, CONV2D,
   ignores it for these tables). Name it `ndn_<list>`.
2. Ops: every state run up to a draw becomes one NdPsoDesc decided by nd_pso_build (written to 3ds/src/nd_ntab_descs.c, which
   gen_pso_tab.py of NPSO3 scans for the ids), NDN_PSO; texture loads (TIMG, TILE, TILESIZE, LOADTILE, LOADBLOCK, LOADTLUT) one
   NdBindDesc through a tile tracker, NDN_TEX; matrices NDN_XF; colours NDN_COL; VTX/TRI through nd_vslot_resolve into NDN_DRAW
   with a generated u16 index array. A run-time value (segment base) is an NDN_DRAW input (p NULL, v the segmented address).
3. Output: v2 tables next to v1 in nd_gfx_tab.c, v1 unchanged.
4. Host compare (nd_gtab_test.c v2): per table and draw, v2 against the v1 path on the host model started from the reset walker
   state (a self-contained list needs no other): pso output bytes, bind desc, transform ops, absolute triangles.
   `ntab tables <n> draws <d> bad <b>`; and `ntab inherit <k>` (expected 544 by the coder's count).
## Places
3ds/tools/nd_gfx_conv.c, 3ds/tools/gen_gfx_tab.sh, 3ds/tools/nd_gtab_test.c (the v2 compare), new generated
3ds/src/nd_ntab_descs.c (.gitignore line). Not nd_gtab.h (CONV2A form is fixed), not 3ds/src runtime files.

## Gate (expected values)
- Build, memcheck OK. nd_host PASS with `ntab tables <n> draws <d> bad 0` (n = 1372 - inherit count) and `ntab inherit 544`, and
  a seeded fault (a value change: one NDN_COL value + 1 in the compare's copy) shown first with bad above 0.
- v1 output unchanged: the v1 tables of nd_gfx_tab.c are byte-equal to the merge-base's (diff of the generated file parts).
- No Azahar (no runtime file reads v2 yet). Budget 0.

## Stop
A list whose state the scan cannot name, a draw that differs (bad above 0), or a v1 table that changes: stop, commit, hand back
(2 tries per named cause). Delete this row file in the completing commit.
