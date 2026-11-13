# Row CONV2D: nd_ntab_run, the core 0 runner of the v2 tables (NdNTab) into the native emit, used by nd_gtab_run

Missing fact: there is no device consumer of NdPsoOut yet. The grep of 3ds/src for nd_pso( and nd_pso_colour finds only nd_decide.c and the generated table. So this row writes the pso apply (design part B).
(Model: Sonnet. The user decides. Reason: the row adds a new emit path in shared draw code. It has four design choices that Haiku cannot make: the pso apply from NdPsoOut, the draw of absolute indices on the ring, the walker-state handover, and the id map. The gate is a byte-exact drec. It cannot be cut smaller: a runner without its emit and its caller is untested code (CLAUDE.md 7). The mechanical call-site moves that follow are CONV2F and the ENTRY rows (Haiku).)
## Facts (ORACLE8, main 182ac5091)
- The form is at 3ds/include/nd_gtab.h:73-96: NdNOp {op, a, b, v, p, idx_pos} and NdNTab {list, entry, nops, ops, idx}. The
  converter is 3ds/tools/nd_gfx_conv.c (v2_walk :380-470, output :528). The tables are in the device nd_gfx_tab.c. The index ndn_all
  is only in the host file nd_ntab_idx.c (gen_gfx_tab.sh:18). The host compare 3ds/tools/nd_gtab_test.c v2_compare gives
  `ntab tables 923 ... bad 0` and `unsup 23 descs 105` (CONV2MTX).
- NDN_PSO b is the converter-local description index (nd_gfx_conv.c:461, :563 "b = the line number below"). It is not an nd_pso
  id. gen_pso_tab.py (:105, sorted file order) gives the ids. nd_ntab_descs.c (in 3ds/src, written by gen_gfx_tab.sh:22) names
  them ndn_pso_<i>. gen_pso_main.c (:42-58) writes nd_pso_raw and nd_pso(id).
- NDN_XF: a = the low byte of the G_MTX w0 (the converter keeps `a & 0xff`, :409). The unit is
  `NDO_W0(NDO_MTX, 0x380000u | a)` with w1 = p or v (nd_xf.c:14 makes the same unit). a 0xFF = pop: `NDO_W0(NDO_POPMTX, 0x380002u)`
  with w1 = v (64). NDN_COL: a 0..3 = NDO_PRIM (unit a = b), NDO_ENV, NDO_FOG, NDO_BLEND (unit a = 0) with w1 = v (nd_op.c:22-25).
- NDN_TEX: p = NdBindDesc, a = tile. nd_tex_bind_desc(d, n) is at nd_tex_set.c:390. nd_direct_tex (nd_direct.c:142) shows the walker
  bookkeeping for tile 0 (tile_x0, tile_y0, tile_fx, tile_fy, tsz_last). The walker draw reads nd_ws.td[0..1] (nd_comb.c:73, :201,
  nd_geom.c:222).
- NDN_DRAW: p = Vtx base, b = index count, idx from idx_pos (absolute indices). The ring draw of a flat list is nd_part_draw
  (nd_geom.c:354-404): room check, refusals (lighting, gv_matrix_id) before the first draw, the gv_tri epoch path per triangle.
- The caller is nd_gtab_run (nd_walk.c:568). It takes the state-only arm, then ent_gdl, dlc_try, and else the ops. nd_rec_dl records
  it (nd_gtab.c:85). nd_rec_dl is used in 119 src files.
- The gates are HOSTCORE2 drec (pm_gpu.c:574, `drec=<first>:<count>`, one record per draw at nd_native_submit :1310) and the bench
  spot_mac01_drec (bench.d/HOSTCORE2.json). fxpic_89 (water_splash, 2 nd_rec_dl sites) has a stable base (BASEEVID, BOOTSTATE).
## Design (you choose the steps inside it, not the gate)
A. Id map: gen_pso_main.c also writes `const NdPso nd_ntab_pso[]` (the nd_pso id of ndn_pso_0 .. ndn_pso_<n-1>, in local
   order) and `const unsigned nd_ntab_npso`. A missing name is a generator error.
B. Pso apply: nd_pso_apply(id, prim, env, plod) in nd_ntab.c. It writes NdPsoOut (comb, rnd, tev) through the same pm_gpu setters
   that gv_epoch_enter (nd_geom.c:203) and its callees use for the walker decision, and nd_pso_colour for the colours. Reason: the
   drec must be byte-equal, so the setters and their order must be the walker's.
C. Walker handover: before the first op, the runner sets the nd_ws fields that the draw reads (comb words, omode, geom, tex_on,
   td[0..1], tile bookkeeping) from the desc and the bind, or it bumps gv_epoch so that the walker resyncs. After the table, the
   walker state must equal what the list's own ops leave (the state ops of the list still run through nd_walk_state). Reason: the
   next walker draw reads nd_ws.
D. Draw: nd_ntab_draw(base, idx, n) in nd_geom.c, next to nd_part_draw. It does one ring load of the index range of the table
   (min..max of its idx, checked before any op, as nd_part_draw) and the same triangle body (factor it out of nd_part_draw into a
   static function and call it from both; nd_part_draw output must not change).
E. Runner: `int nd_ntab_run(const NdNTab* t)`, core 0, one switch over NdNOp (the mapping of the Facts, NDN_CALL recursive).
   It returns 0 before any emit when a precondition fails, and then the caller runs the old path.
F. Hookup: nd_gfx_conv.c also writes a sorted index (list -> NdNTab) into nd_gfx_tab.c, read by
   `const NdNTab* nd_ntab_find(const void* list)` (nd_gtab.h; NULL = no v2 table; ENTRYCAP calls it). nd_gtab_run tries nd_ntab_run first in
   the non-state-only arm, before ent_gdl. It counts the runs in `nd_gtr_ntab` and writes `ntab runs %u` to the log next to the
   drec entry.
## Readers and writers (edit each one)
nd_gtab.h (prototypes), new 3ds/src/nd_ntab.c, nd_geom.c (nd_part_draw split, nd_ntab_draw), nd_walk.c nd_gtab_run, gen_pso_main.c
(the map), nd_gfx_conv.c (the device index output only, not v2_walk), nd_gtab_test.c (link the runner with stubs only if the
host test needs it), new 3ds/tests/host.d/ntabrun.sh, new azt.d/CONV2D.json and bench.d/CONV2D.json.
## Places
The files above: nd_walk.c nd_gtab_run only, nd_geom.c nd_part_draw and the new function after it, nd_gfx_conv.c the main output
block after :594, gen_pso_main.c the writer :42-58. CONV2MIX (nd_gfx_conv.c resolve block :270-295) can run at the same time.
## Gates (exact, in this order). Start: `sh 3ds/tools/wt_new.sh CONV2D`. PM_ROW=CONV2D PM_LANE=CONV2D.
1. Commit 1 holds only the scene: azt.d/CONV2D.json fxpic_89_drec (the `debug` field of fxpic_89 with `drec=412:2` in place of
   `shots shotwin=412:4`, until `drec f413 draws [1-9]\d* bytes`, cap 40, deadline 35, needs_picture false) and
   bench.d/CONV2D.json (kind azahar_pic, as spot_mac01_drec). Run `bench.py base fxpic_89_drec` and `base spot_mac01_drec` at
   commit 1 (before any code).
2. Host: ntabrun.sh runs the id map and the op-to-unit mapping of every table (XF, COL) against the v1 units of nd_gtab_test's
   model: `ntabrun ok <n>` with 0 bad. Seed first (a value change): pop unit a 0x380002 -> 0x380003: bad above 0.
   nd_host PASS. Build, memcheck OK.
3. Device: `bench.py run fxpic_89_drec,spot_mac01_drec --row CONV2D --why "CONV2D drec"`, then `bench.py cmp` of each:
   drec.txt byte-equal. The log of fxpic_89_drec shows `ntab runs` above 0. Seed first (a value change): in nd_ntab_run, NDN_COL
   prim v + 1: cmp fxpic_89_drec must differ. Budget 8 Azahar starts.
Counter: none of C1-C12 moves. This is the plan v3 runner row (blocker of CONV2F, ENTRYCAP).
## Stop and end
`ntab runs 0` in fxpic_89_drec (no v2 table is on that path: hand back the list names of its nd_rec_dl sites), a drec difference
with the real code, a seed with no difference, a base pair that differs, or a tool fault: stop, commit, hand back the exact output
(CLAUDE.md 4-5). 2 tries per named cause. Delete this row file in the completing commit.
