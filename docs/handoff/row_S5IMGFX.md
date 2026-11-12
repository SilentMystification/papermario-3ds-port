# Row S5IMGFX: imgfx meshes drawn through native pso and mesh records (NATIVEAPI E7, design and first modes)

Model: Opus (user decides).
- Why not Sonnet:
  - imgfx is the widest producer: 15 render types (imgfx.c:905-1091) times 4 mesh types (:1100-1115);
  - the vertex meshes are rebuilt per frame in the heap (imgfx_mesh_make_strip, _grid, _wavy, imgfx_mesh_anim_update), and
    entity draws hash them per frame (ent_heap_list, nd_ent.c:188-194);
  - complex HUD mode 1 (S5CHUD) and every sprite with an effect draw through it.
  - The design must choose how a per-frame mesh reaches core 0 without the vertex hash and the op replay. That choice also binds
    S5CHUD mode 1 and the sprite effect path.
- Opus may choose its steps inside the gate. Write the design note into this row before code.

## Blocker and base
NAPI2B merged (direct helpers) and S6T's design note written (the state-range answer: DefaultImgFXSetupGfx is a static range).
`sh 3ds/tools/wt_new.sh S5IMGFX`; PM_ROW=S5IMGFX PM_LANE=S5IMGFX.

## Code read (src/imgfx.c)
- imgfx_appendGfx_component (:772) leads to imgfx_appendGfx_mesh (:864). That function does:
  - a PipeSync, and the setup list DefaultImgFXSetupGfx unless SKIP_GFX_SETUP;
  - the filter and cull geometry flags;
  - the render mode pair from ImgFXRenderModes[renderType], with the alpha < 255 replacement switch (:900-920);
  - per render type, the combine and prim or env colours (:946-1091);
  - the mesh draw by meshType (:1100-1115). mesh_basic loads a CI4 or other texture with a palette (TLUT RGBA16, LoadTLUT_pal16,
    ScrollMultiTile2_4b, :1565-1590) and draws vertex strips from the per-frame vertex buffer with the given matrix;
  - for HOLOGRAM, extra trailing state.
- Readers of the per-frame meshes: ent_heap_list (nd_ent.c:188-194) and the ent hash; the walker G_VTX path (nd_geom.c) for the
  inline gSPVertex.
- Callers: sprite.c (spr_appendGfx_component, the imgfx arm), hud_element.c render_complex_hud_element mode 1 (:1655-1688), and
  entities and NPCs through imgfx_appendGfx_component.

## Design questions (the design note)
1. Record form: NDC_NAPI {pso id, NdBindDesc (raster, palette, CI4 window), float matrix, vertex range in the ring or in a
   per-frame copy, index pattern of the mesh type}.
   - Can core 1 copy the vertices into the record pool (the ring copy happens today on core 0 in nd_geom_vtx)?
   - Size per mesh: grid meshes reach a few hundred vertices.
2. The pso per render type: the 15 types map to (combine, render mode, prim and env use).
   - Use nd_pso_n64 per type: a static table built from ImgFXRenderModes and the combine words of each case.
   - Keep the alpha < 255 replacement on core 1.
3. Texture: the CI4 load with palette and scroll as an NdBindDesc plus a uv offset (TILEWIN2 rule).
4. Draw: the ring path with a matrix palette entry (gv_tri with a matrix id), or a native mesh draw (nd_part_draw form)? Both
   must keep the CPU clip and cull decisions of today.
5. Fallback: a type or mesh the record cannot state runs the old ops. Count it per type, as a census line.

## First modes in this row
IMGFX_RENDER_DEFAULT and MULTIPLY_RGB / MODULATE_PRIM_RGB with IMGFX_MESH_DEFAULT: the sprite and HUD common case. The other
types fall back, counted.

## Files and places
src/imgfx.c (imgfx_appendGfx_mesh and mesh_basic under TARGET_3DS), 3ds/src/nd_imgfx.c (new runner), 3ds/include/nd_api.h
(declarations at the end), azt_scenarios.json and bench_defs.json (one census scene, after `mdl5_iwa10_det`; its expected_events
hold only lines both builds print, never the new imgfxcen line: a base would fail; grep the census in the head log). S5CHUD waits on
this row for its mode 1.

## Gate (expected values)
- Speed rule (oracle 5): a bench list runs at one speed; spot_mac00_det is a turbo bench (bench_defs speed turbo), every
  other bench here is normal. Run it as its own list: `bench.py run spot_mac00_det --row S5IMGFX --why "..."` (1 start).
- nd_host PASS. DIRECT byte set (spot_mac01_det, mdl5_iwa10_det: sprites with imgfx; spot_mac00_det), one list, 3 starts: every
  shot byte-equal.
- The census line `imgfxcen f<n> native <a> fb <b> by type ...` at f300 of spot_mac01_det: native > 0, and the fallback counted
  per type.
- napicen_mac01: sprite.o ops drop by the imgfx share (hand back the per-function numbers).
- Budget 8 Azahar starts.

## Stop
A byte difference: stop, commit, hand back (2 tries per named cause). Delete this row file in the completing commit, and list the
follow-up rows (other render types, mesh types) in the hand-back.
