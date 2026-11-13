# Row NPSO2: nd_pso_build, the pure pipeline decision from an NdPsoDesc (host-linkable), device behaviour unchanged (Sonnet)

Model: Sonnet (plan v3, accepted). Reason: a code move across nd_comb.c, nd_tmpl.c and nd_direct.c that cuts the walker reads
out of nd_comb_decide, nd_render_state and nd_tev_build into NdPsoDesc fields while the device output stays byte-equal. The order of
the cut is found while moving. Why not Opus: NPSO1 fixes the field list.

## Facts (ORACLE8, main b8b314758, NPSO1 row)
- NdPsoDesc (NPSO1, nd_api.h): cw0, cw1, omh, oml, geom, key_c, key_s, conv_k4, tex_on, use_t1, t1_valid, dual_on, box_prod,
  box_folded, comb_set, comb_alpha_shade, leg_prev, vprim, zimg, plod. NdPsoOut is declared there, not defined.
- The decision today: comb_in_walker (nd_comb.c:476-482) fills NdCombIn from walker globals, nd_comb_decide (nd_comb.c, after
  :482) works on it (it sets and restores globals around sync_combine_work). nd_render_state (nd.h:344, nd_tmpl.c), nd_tev_build
  (nd.h:360) take explicit inputs. tst[16] comes from the combine words (G_SETCOMBINE decode). nd_host links nd_tmpl.c and pm_comb.c
  already (nd_host_test.sh:30-31). nd_comb.c is not host-linked (the empty shim at nd_host_test.sh:57).
- prim and env are in NdCombIn but not in NdPsoDesc (NPSO1: colours are draw values).

## Design (fixed. The coder chooses the inner steps)
1. nd.h: `struct NdPsoOut { NdCombOut comb; NdRender rnd; NdTevOut tev; };` (the typedef is in nd_api.h).
2. New 3ds/src/nd_decide.c: `void nd_pso_build(const NdPsoDesc* d, NdPsoOut* o)`: the tst decode of cw0/cw1, then the combine
   decision, nd_render_state(omh, oml, zimg, &o->rnd), the TEV build. No walker global is read (nd_ws, tst_f, keyc ... are
   inputs through d only). nd_comb_decide keeps its signature and calls the same moved code, so the device does not change.
3. prim and env: prove by code that the decision does not read them for any field of NdCombOut except the constant colour cc
   (cc_on, cc). If any other output reads them, stop and hand back the line (the description form is then not complete).
4. Host compare 3ds/tools/nd_decide_test.c (host.d/decide.sh): for every description of a fixed corpus (all NdCombIn values the
   ndref captures in 3ds/tests/ndref/ndref_*.bin hold, converted to NdPsoDesc), nd_pso_build equals the old path (NdCombIn with
   the same fields, nd_comb_decide, nd_render_state, nd_tev_build) byte for byte. Prints `decide entries <n> bad 0`.

## Readers and writers
Writers: nd_decide.c. Readers: nd_comb_decide (device, unchanged behaviour), NPSO3's generator, NPSO4's psochk. List every global
the moved code read before and its NdPsoDesc field in the commit.

## Places
New 3ds/src/nd_decide.c, 3ds/tools/nd_decide_test.c, 3ds/tests/host.d/decide.sh. nd.h: one struct after NdTevOut (nd.h:360).
nd_comb.c: nd_comb_decide and the code it moves. nd_tmpl.c: only if nd_render_state or nd_tev_build must move. Not nd_api.h.

## Gate (expected values)
- Build, memcheck OK. nd_host PASS with `decide entries <n> bad 0` (n above 0) and a seeded fault shown first (one field of the
  desc not copied: bad above 0).
- Runtime byte-equal (the device code path is the same): `bench.py base spot_mac01_det` at the merge-base, run, `bench.py cmp
  spot_mac01_det --row NPSO2`: 6/6 byte-equal. Budget 4 Azahar starts.

## Stop
A decision output that reads prim or env outside cc, a desc field that is not enough to rebuild an NdCombIn, or a byte difference:
stop, commit, hand back (2 tries per named cause). Delete this row file in the completing commit.
