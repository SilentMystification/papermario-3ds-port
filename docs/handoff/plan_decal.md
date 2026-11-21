# plan_decal: outline around overlay textures (kmr_11 castle windows)

Oracle NATIVE25. User report (fact): overlay textures show an outline, for example the castle windows of kmr_11.
Pages: ws_DEC0 (gate tools), ws_DEC1 (fix rule). Other plans (plan_seams and the rest) stay as they are.

## 1. Producer (code read, FACT unless marked)
- Draws: kmr_11_shape.bin models 0x80219e4c, 0x80219ed4, 0x80219f5c carry kmr_kkg_madotif with render mode 0x0D
  (RENDER_MODE_ALPHATEST, include/enums.h:3313). The wall models carry kmr_kkg_wall_atif with mode 0x01 (SURFACE_OPA). Source: shape tree parse.
- Texture: madotif is CI4 16x32, hwrap Mirror, vwrap Clamp, filter true (assets/us/mapfs/tex/kmr_tex.json, ROM record 0x25A832E).
  Alpha-0 palette indices: 1, 7, 9-15. Index 1 fills row 0, row 31, column 0 and the arch corner. The frame ring uses indices 2-6.
- Key colour: TEXKEY2 gives index 1 the value 0x73A0 = (14,14,16) of 31 (3ds/include/texkey_patch.h:75, address 0x025A8460).
  The rule takes one mean for all texels of that index (3ds/tools/texkey_bake.py:25-56). The neighbours are (9,8,13), (20,20,20), (17,17,18).
  So the key is up to 6 steps of 31 off its local neighbour.
- Path: dma_copy (src/model.c:1573) -> nuPiReadRom (pm_os.c:548) -> tk_patch_apply (pm_os.c:506) -> tx1_build CI4 to RGBA5551 (pm_tex_tx1.h:62-68).
- Filter: G_TF_BILERP gives GPU_LINEAR (pm_tex.c:1884-1888). Mag and min bits 1 and 2 of the unit param (nd_tex.c:35-38).
- Cut: nd_edge_aa matches (nd_tmpl.c:286-288). nd_render_state sets blend 0 and atest 0x30 (nd_tmpl.c:302).
  R_ALPHA_TEST word 0x3061 (nd_tmpl.c:377), blend ONE/ZERO 0x01010000 (nd_tmpl.c:389).
- Result: a fragment with filtered alpha >= 0.19 draws at full weight. Its colour is up to 81 % key colour. So a light line shows on
  the dark ring and a dark line on the light ring. The N64 AA blender fades that pixel by its coverage (GUESS: RDP rule, no N64 run).
- History: KMR11W (1378f62617) blended these edges. EDGEAA (4a11a74e83) put the hard cut back (osr_04 depth fault) and kept TEXKEY2.
  No row since f0a9f642e4 touched nd_tmpl.c, nd_tex.c, pm_tex_tx1.h or texkey_bake.py (git log). The outline is old: since EDGEAA.

## 2. Ranked table
| rank | candidate | verdict | proof |
|---|---|---|---|
| 1 | (2)+(4) key colour under bilinear, hard cut | CAUSE (FACT) | section 1 |
| 2 | (8) merged rows | EDGEAA 4a11a74e83 is the regression point (FACT). NC0V, T1FIX, MCOPY, DO1, MSNAP2, DO3F touch no texel or cut word (git log) | git log f0a9f642e4..HEAD |
| 3 | (1) filter and half texel | pm_texf_centre is 0 by default (pm_options.c:463, nd_model.c:41-46): a half-texel image shift, no colour line (GUESS) | code read |
| 4 | (3) wrap and clamp | ruled out here (FACT): t clamp gives CLAMP_TO_EDGE (nd_tex.c:23-27), rows 0 and 31 are key texels. CLAMP_TO_BORDER only through the wrapov debug switch (pm_tex.c:1912) | ROM dump |
| 5 | (5) decal depth | ruled out here (FACT): mode 0x0D is not a decal, the decal passes (nd_tmpl.c:347-356) do not run | shape parse |
| 6 | (6) fog and combiner | GUESS ruled out: wall and window both use combine Multiply (kmr_tex.json), same map fog | json |
| 7 | (7) uv rounding | GUESS ruled out: a sub-texel shift, no colour line. plan_seams owns vertex seams | - |

## 3. Scope (prototype scan of 1498 main images, GUESS until DEC0 records FACT)
Bad texels (alpha 0, opaque 8-neighbour, a channel more than 1 step off the neighbour mean): CI4 35581 in 343 textures, CI8 1324/16,
RGBA16 2730/24, IA4 7836/37, IA8 1966/11, IA16 368/2, RGBA32 512/8. madotif 46. RGBA5551 formats (CI, RGBA16): 39635 of 50317 = 79 %.
A per-texel bake at build time needs about 69K u16 patches, more than 400 KB static (GUESS). The memcheck margin is 390 KB (FACT:
build3ds/out memcheck line). So the build-time bake is ruled out.

## 4. Rule (fix, DEC1)
After the tiled RGBA5551 image is built (pm_tex.c load_impl, one site before :1634, for TX1 and for the tile copy), each alpha-0 texel
with an opaque 8-neighbour takes the floor mean RGB of those neighbours. Alpha stays 0. Opaque texels never change.
Every bilinear footprint that reaches alpha > 0 holds an opaque texel, so all its alpha-0 texels are 8-neighbours of it: no key colour
reaches a drawn fragment (FACT by construction, except across a Repeat wrap seam: tex_fringe counts it).
The rule holds for every map, sprite and HUD texture in RGBA5551. The cell atlas (pm_tex.c:276-310) uses GPU_NEAREST and needs no fill.
Cost: load time only (a texture miss). One alpha test per texel, 9 reads per alpha-0 texel, a reciprocal table (no division).
GUESS 0.1 to 0.5 ms per 64x64 cutout texture on the O3DS. The tx1 line (pm_tex.c:1129) times the fill (timer :1470 to :1652).

## 5. Rows
| row | model | content | gate | fallback |
|---|---|---|---|---|
| DEC0 | Haiku | tex_fringe.py, drec_state.py, kmr11_drec scenario and bench | selftests, base 2 equal runs, cmp byte-equal, census lists the window draws | - |
| DEC1 | Haiku | pm_tex_key.h, 1 call site, host test | host PASS and seeded FAIL, --fill 5551 = 0, cmp --expect tkd PASS, tdiff other 0 tex_opaque 0 | F1, F2 |
| DEC2 | Haiku | same rule for LA4, LA8, RGBA8 (IA4, IA8, IA16, RGBA32: 10682 bad, GUESS) | as DEC1 | page after DEC1 data |
| DEC3 | Haiku | delete TEXKEY2 (texkey_bake.py, build.sh:97-102, pm_os.c:472-489 and :506, build_key.py:364-369) | tdiff other 0 | page after DEC1 merge |
- F1 (tdiff tex_opaque > 0: an opaque draw shows alpha-0 texels): fill only for a texture whose first draw has an alpha test or a blend.
  nd_tex_get passes that bit to pm_tex_load (descriptor field). The oracle writes the page from the tdiff list.
- F2 (console cost too high): tex_fringe writes the source hashes of the textures with bad > 0. The runtime fills only those.
- F3 (soft edge wanted, user yes, Sonnet): two passes per edge-AA draw. Pass 1: alpha test 0x80, depth write. Pass 2: alpha test 0x08,
  blend SRC_ALPHA, depth GREATER, no depth write. The pass field of nd_tmpl.c:347-365 is the pattern.

## 6. Gates (exact)
- (a) tex_fringe.py: static scan of the ROM and the TEXKEY2 patches. --fill simulates DEC1. Numbers go into the commit message.
- (b) drec_state.py census: per draw the alpha test, blend, depth, depth offset, unit 0 filter and wrap, dim and texture hash.
  Wall and window state show as numbers. tdiff: a DEC1 diff may touch only t lines of alpha-test or blend draws.
- (c) kmr11_drec: warp=kmr_11:0 story=-120 det drec=221:3:20 (the KMR11W window shots are in frames 221 to 261). Marker `tkd f<m>`.
- Counters C1..C12: none (OPEN: this oracle did not read the list. A texture colour rule touches no list counter).
