# Row NXF: native transform and mesh records (nd_xf_push / nd_xf_pop, nd_mesh)

Model: Sonnet (row_ENDGAME.md phase B).
- Why not Haiku: a new record pair on the core 0 emit, and the list matrix stack (load_mtx, nd_mtx_set.c) must give the same matrix
  palette rows for the same calls; that is a choice in shared emit code.
- Why not Opus: the emit, the matrix conversion (load_mtx fixed to float, mtx_cache) and the native draw records exist; this row
  gives them a call-site API.

## Facts (ORACLE7 code read, main 235bb8721)
- gSPMatrix / gSPPopMatrix become NDO_MTX / NDO_POPMTX units; core 0 runs load_mtx (nd_mtx_set.c) and gbi_popmtx through op_geo
  (nd_walk.c:267-297). Built geometry goes VTX/TRI through nd_geom_vtx and gv_tri (the ring).
- Native draw records exist for rectangles, quads, glyphs and particles (nd_api.h); there is no transform or mesh record.

## Design (fixed; the coder chooses the inner steps)
1. `void nd_xf_push(const Mtx* m, unsigned flags)` (G_MTX_LOAD or MUL, PROJECTION or MODELVIEW, PUSH or not, the same meaning as
   gSPMatrix) and `void nd_xf_pop(unsigned which)`: NDC_NAPI records; their core 0 runner does what load_mtx and gbi_popmtx do
   (same float conversion, same palette row reuse), with no typed unit.
2. `void nd_mesh(NdPso pso, const NdBindDesc* tex, const Vtx* v, unsigned nv, const uint16_t* idx, unsigned ni, uint32_t prim,
   uint32_t env)`: one indexed draw under the current transform, through the native emit used by the model and particle paths. The
   vertex data stays the N64 Vtx layout (vertex data, not list words).
3. Proof: psochk (NPSO) for the pso; the transform rows equal by the byte gate.
4. Pilot (moves counters): src/effects/ring_blast.c and src/effects/stat_change.c (stat_change 22 calls): every gDP/gSP call of
   the two files becomes NPSO descriptions plus nd_xf_* and nd_mesh or nd_draw_list (CONV2 if merged; else stop at nd_mesh for
   built geometry and keep their static-list calls for NFX0).

## Files and places
3ds/include/nd_api.h (the three calls), 3ds/src/nd_direct.c or a new 3ds/src/nd_xf.c (runners), the two pilot effect files.
Blocker: NPSO merged (table pso ids).

## Gate (expected values; plan_endgame.md section 5)
- Build, memcheck OK; nd_host PASS. LISTCEN2: the two pilot files build 0 and gfxpos 0; both include nd_nolist.h.
- Golden masters of the two files (their GOLD-FX row) equal byte for byte, gcov 100 % of the converted lines. No Azahar start.

## Stop
A golden or frame-hash difference or psochk bad above 0: stop, commit, hand back (2 tries per named cause). Delete this row file in the completing
commit.
