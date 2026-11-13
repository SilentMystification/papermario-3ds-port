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
3. Colour form (correction 1, ORACLE8 decision by code read, main 344822275): prim and env do not join the description. The
   decision makes names, the draw makes colours:
   - PmPlan already holds colour NAMES: st[i].cname, bufname, vtxname. pm_comb_colours (pm_comb.c:496-500) turns them into
     values with name_colour (pm_comb.c:477-494) from prim, env, plod, key, keys. nd_comb.c:561 calls it inside the decision.
   - nd_tev_build writes a colour only into e[3] of a stage (`e[3] = nd_abgr(st->constant)`, nd_tmpl.c:206) and buf_color
     (:194). Sources, operands, functions and scales of the stages read no colour. nd_emit.c:53 already takes the stage colours
     from the draw (tev_col[6], buf_col).
   - nd_comb.c const_rgb_tex_alpha (:110-135) returns 1 or 0 by the combiner fields only (a negative input code), and its RGB
     out is a colour. cc_cycle1 (:340) computes colours. List every other read of nd_ws.prim and nd_ws.envc in the commit with
     the output it feeds.
   So: nd_pso_build fills NdPsoOut with the plan WITHOUT pm_comb_colours (names kept, constants 0) and the TEV words with e[3]
   and buf_color 0. New pure function `void nd_pso_colour(const NdPsoOut* o, uint32_t prim, uint32_t env, uint32_t plod,
   uint32_t key, uint32_t keys, uint32_t tev_col[6], uint32_t* buf_col, uint32_t* vtx)` = name_colour per stage name, buf and vtx,
   in nd_decide.c. If any non-colour field of NdCombOut, NdRender or the stage words reads prim or env: stop, hand back the line.
   nd_comb.c is not host-linkable (globals nd_ws, tst_f, keyc, keys, conv_k4, leg_mode, comb_set, box_*): nd_decide.c takes
   them only from NdPsoDesc. The empty nd_comb_decide shim at nd_host_test.sh:57 stays until NPSO3.
4. Host compare 3ds/tools/nd_decide_test.c (host.d/decide.sh): for every description of a fixed corpus (all NdCombIn values the
   ndref captures in 3ds/tests/ndref/ndref_*.bin hold, converted to NdPsoDesc), nd_pso_build plus nd_pso_colour (with the prim and env
   of the capture) equals the old path (NdCombIn with the same fields, nd_comb_decide, nd_render_state, nd_tev_build) byte for byte. Prints `decide entries <n> bad 0`.
   A test that links nd_tmpl.c defines f32tof24 itself (copy 3ds/tools/nd_cmd_test.c:16-26, as the other host tests do).

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
