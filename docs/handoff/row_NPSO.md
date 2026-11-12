# Row NPSO: pipeline states decided at build time (the pso table), complete descriptions, and the psochk proof

Model: Opus (design core of row_ENDGAME.md section 2; needs the user's yes).
- Why not Sonnet: it moves the N64 combine and render decision out of the device into a host generator and fixes the one state form
  every later family row uses; a wrong description form makes every later row redo its call sites.
- Why not smaller: the description, the generator, the host proof and the device lookup are one contract.

## Facts (ORACLE7 code read, main 235bb8721)
- Today: `NdPsoN64 { cw0, cw1, oml, omh, omh_m }` (3ds/include/nd_api.h), interned at run time by nd_pso_n64 (3ds/src/nd_api.c:18),
  decided on core 0 by nd_pso_decide / dec_make (nd_direct.c:42-81) through comb_in_walker (nd_comb.c:476) and nd_comb_decide
  (:508): the decision also reads walker fields the description does not hold (tex_on, use_t1, td[1].valid, dual_on, box state,
  comb_set, key_c, key_s, conv_k4, prim_lod, pm_comb_vprim; dec_key nd_direct.c:57-64) and the geometry mode (z, shade, cull, fog,
  lighting) through nd_render_state (nd_tmpl.c:292). So today's description is not complete.
- nd_host links only a test shim of nd_comb_decide (3ds/tools/nd_host_test.sh:57 defines an empty one for one test): the real
  decision is not host-linkable yet. pm_comb.c compiles on the host (:30).
- S7 v1 defs record `inherit` and `ctx` masks (plan_nativeapi.md section 6 item 6) for exactly these missing inputs.

## Design (fixed; the coder chooses the inner steps)
1. Complete description `NdPsoDesc` (new, nd_api.h): cw0, cw1, oml, omh (full words, no mask), geom (the geometry mode bits the
   decision and render state read), tex_on, tex1 (the tile 1 role: none, aux texture, shaded sprite), key (key_c, key_s), conv
   (conv_k4), vprim. Every input of nd_comb_decide, nd_render_state and nd_tev_build is a field or a constant; write the list of
   inputs with their source line in the commit. A description is a `static const NdPsoDesc` at the call site or a converter output.
2. Host decision: split the pure decision out of nd_comb.c and nd_tmpl.c into 3ds/src/nd_decide.c (host-linkable, as S7TEX1 split
   tex_win0 into nd_texwin.c): `nd_pso_build(const NdPsoDesc*, NdPsoOut*)` = the combine decision, render state, TEV and blend
   words. The device keeps calling the same code until S9DEL2 deletes the device copy.
3. Generator 3ds/tools/gen_pso_tab.sh (docker pm3ds-host:1 like gen_gfx_tab.sh, stamped in build.sh): collects every description
   (a scan of `ND_PSO_DESC(name, ...)` in src and the descriptions CONV2's converter writes), runs nd_pso_build on each, writes
   3ds/src/nd_pso_tab.c (git-ignored, generated): `const NdPsoOut nd_pso_tab[]` and one `const NdPso` id per name. Device API:
   `const NdPsoOut* nd_pso(NdPso id)`; no decision on the device for a table id.
4. Proof while the walker exists: debug switch `psochk` (pm_debug_has, read once): at each native draw that names a table id,
   core 0 also builds the walker's description of the moment (comb_in_walker plus the omode, geom and tile fields) and compares the
   two NdPsoOut; one entry per 100 frames `psochk f<n> draws <d> bad <b>` (pm_log_evt, counts since boot), the first 8 bad ones
   print the differing field. A family row's gate is `bad 0` in its scenes: proof that its descriptions are complete.
5. Pilot (this row moves counters): src/draw_img_util.c (26 gDP/gSP calls, 26 gMainGfxPos uses: image rectangles). Each draw
   becomes one native record with a table pso id, an NdBindDesc and the rectangle; no gDP/gSP call is left in the file.

## Readers and writers
Writers: call sites (ND_PSO_DESC), gen_pso_tab.sh, CONV2's converter. Readers: nd_pso (core 0 emit), psochk, nd_host (the host
compare: for every table entry, nd_pso_build on the host equals the device decision code compiled for the host from the same
source: same NdPsoOut bytes). grep `nd_pso_n64\|nd_pso_get\|nd_pso_decide` and list each caller in the commit: they stay until
the family rows move them (count them in the hand-back).

## Files and places
3ds/include/nd_api.h (NdPsoDesc, ND_PSO_DESC, nd_pso), new 3ds/src/nd_decide.c, nd_comb.c and nd_tmpl.c (the split only),
new 3ds/tools/gen_pso_tab.sh, 3ds/build.sh (one generator step next to gen_gfx_tab), 3ds/tools/nd_host_test.sh (the compare;
delete the empty nd_comb_decide test shim at :57 by linking nd_decide.c), 3ds/src/nd_direct.c (the psochk hook), src/draw_img_util.c.

## Gate (expected values; plan_endgame.md section 5)
- Build, memcheck OK. nd_host PASS with `psotab entries <n> bad 0`.
- LISTCEN2: src/draw_img_util.c build 0 and gfxpos 0 (from 26 and 26). The file includes nd_nolist.h (G8).
- Golden master: GOLD-TTL's file for draw_img_util.c is equal byte for byte after the conversion, gcov 100 % of its converted
  functions' lines (GMHARN, GMCOV). Blocker for this gate: GOLD-TTL merged.
- psochk: one frame-hash run of a scene that draws draw_img_util images (pick it with COVMAP or the callers of
  draw_image/draw_ci_image) with `psochk`: bad 0, frame hashes equal. Budget 8 Azahar starts.

## Stop
A golden or frame-hash difference, psochk bad above 0, or a decision the host and device give differently: stop, commit, hand back (2 tries per
named cause). Delete this row file in the completing commit.
