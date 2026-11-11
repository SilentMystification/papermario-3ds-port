# NATIVEAPI: the game's draw code calls native draw functions (plan; the orchestrator moves it to plan_nativeapi.md + INDEX.md)

Oracle plan, code read on main 2026-11-11. Data: HCM3 (M1-hot 71.0 KB: A 9.5, B 5.6, C 9.7, D 11.4, E 23.1, F 11.1, G 14.1, H 11.8, I 10.6;
M3 mac_01 f200 96,044 B run per frame, 409 functions, 45,528 B outside the classes), XL PMU (draw phase 83.2 % of core 0 I-misses).

## 1. Architecture
- Today: game draw code (core 1, logic thread) calls the gDP*/gSP* macros; nd_gbi.h turns each into a typed op (nd_op.h) recorded in the
  frame stream (nd_rec.c); core 0 walks the stream (nd_walk.c nd_ops_run / nd_walk_state) through an RDP/RSP state model: other-mode
  bits, N64 combine words and the combine decision (nd_comb.c, pm_comb.c), tile descriptors and the TMEM load model (nd_tex_set.c
  set_tilesize, tex_win0/1, ensure_tile), vertex slots and the matrix stack (nd_geom.c, nd_mtx_set.c), rectangles (nd_rect.c), then the
  native emit (nd_emit.c, pm_gpu.c nd_native_submit). Models (G_PM_REC), sprites (NDC_SPR) and entities (NDC_ENT) already reach core 0
  as records with a native draw path; effects, HUD, text, menus and overlays reach it as typed state ops.
- Target: the call site states its draw natively; the stream stays (it is the core 1 -> core 0 pipeline and keeps the draw order) but
  carries native commands only. API (3ds/include/nd_api.h, recorded on core 1, executed on core 0):
  - `NdPso nd_pso(const NdPsoDesc*)`: a pipeline state id (TEV template, blend, depth test and write, alpha test, cull, texture unit
    setup) made once per distinct description and cached; compatibility form `nd_pso_n64(cw0, cw1, omode_h, omode_l, geom)` runs today's
    decision (nd_comb decide, nd_render_state, nd_tev_build) once, cold, so a moved call site draws the same picture.
  - `NdTex nd_tex(const NdTexDesc*)`: a texture from raster, palette, format, size and wrap (no TMEM model); windows and scroll as a
    uv transform of the draw (TILEWIN2 semantics: one wrap period bound, the origin as an offset).
  - Draws: `nd_rect(pso, tex, x0, y0, x1, y1, s, t, dsdx, dtdy, colour)` (texture and fill rectangles), `nd_quad(pso, tex, const float m[16],
    const NdVtx v[4])`, `nd_glyphs(pso, tex, const NdGlyph* g, n)` (text), `nd_mesh(pso, tex, const float m[16], const Vtx* v, n, const u16* idx,
    ni)`, `nd_particles(pso, tex, const Gfx* list, const NdInst* inst, n)` (one static list, per-instance float matrix, colour and uv
    offset, drawn through the ring with the matrix palette, 8 per batch). Colours and matrices are floats or packed RGBA; no other-mode
    bits and no combine words cross at run time.
  - Command form: NDC_NAPI records {kind, pso, tex, uniforms, vertex range}; core 0 runs one switch and the emit (prebuilt register
    blocks per pso, as the HC6B model command block).
- Leaves core 0 when the last state-op producer moves: class A (stream and op loop 9.5 KB), B (combine sync 5.6), most of E (batch
  close, nd_tev_build, nd_tmpl_build at run time: 23.1), the walker part of F (gv_tri state checks, nd_gv_params per range: about 7 of
  11.1), G (texrect and fillrect 14.1): about 55 KB of the 71 KB M1-hot; remaining: D submit/emit, C model hit, H sprite, I entity, the
  ring copy. Expected per-frame hot set about 20-25 KB (from M3 96 KB with half outside the classes: the outside part needs its own read).

## 2. Inventory (gDP/gSP macro uses and files, grep of src on main; frame cost from fxcen, PMU and M3 class G/E)
| group | macro uses / files | per-frame weight | risk |
| HUD elements src/hud_element.c | 141 / 1 | every frame (class G 10.2 KB run per frame in M3) | medium (transforms, scissor) |
| message text src/msg_draw.c, msg.c | 154 + 59 / 2 | every dialogue frame (glyph rects) | medium (glyph cells, MSGBOX rules) |
| windows src/draw_box.c | 82 / 1 | menus, dialogue | medium (rotated windows, off-screen pass) |
| screen overlays and fades src/screen_overlays.c, screen_render_util.c | 194 + 75 / 2 | transitions, fades | low (full-screen quads) |
| effects src/effects | 1809 / 127 | per effect (fxcen table) | high count, each simple; particles via nd_particles |
| battle UI src/battle (btl_states_menus etc.) | 406 / 9 | battles | medium |
| pause and file menus src/pause, src/filemenu | 365 + 164 / 11 | menus | medium (off-screen grabs) |
| backgrounds src/background_gfx.c | 79 / 1 | every map with a background | low |
| imgfx src/imgfx.c | 128 / 1 | sprites with effects | high (dynamic meshes) |
| curtains, title, logos | 134 + 66 + 35 / 3 | rare | low |
| item entities, entity_model, animator | 135 + 96 + 36 / 3 | per entity | medium (static lists) |
| models src/model.c | 642 / 1 | records already native | stays a record (no change) |
| sprites src/sprite.c | 78 / 1 | NDC_SPR already native | stays |
| map scripts src/world (custom gfx) | 3414 / 45 | per map | high (static gfx data, custom builders): last |
| entity gfx data src/entity | 8279 / 17 | static lists typed at build | stays static data (nd_shape_conv-style) until the end |

## 3. Staged order (each stage merges before the next; both paths work during the migration; no toggle, each call site moves whole)
- S0 census and skeleton: NAPI0 (per producer file: typed ops per frame, so the order follows measured traffic), NAPI1 (nd_api.h/.c,
  NDC_NAPI record, nd_pso_n64 and nd_rect on the existing emit; no caller yet; nd_host test of the pso cache).
- S1 overlays and fades (screen_overlays.c, screen_render_util.c): nd_rect/nd_quad. Gate: byte-equal of a fade scene (xfer_map_shots,
  battle wipe) and spot_mac01_det.
- S2 HUD elements and windows (hud_element.c, draw_box.c): nd_rect, nd_quad. Gate: byte-equal msgbox_talk_nowide, pause_tab_det_pic,
  spot_mac01_det; hotset class G down.
- S3 text (msg_draw.c, msg.c): nd_glyphs. Gate: byte-equal msgbox_talk_nowide and a menu scene.
- S4 effects, by fxcen cost, one effect group per row: nd_particles / nd_quad. Gate: fxcen counts (draws down), byte compare where a
  det scene exists, the console fxtest run.
- S5 battle UI, pause, file menus, backgrounds, curtains, title, logos, item entities, imgfx.
- S6 the last state-op producers (map custom gfx, entity static lists through a host conversion of their lists to NDC_NAPI form), then
  delete the walker state model (nd_walk_state op table, nd_comb decide at run time, set_tilesize/tex_win TMEM model, nd_rect, the gv
  walker state): classes A, B, E, G leave core 0. Gate: nd_host, the DIRECT byte set, hotset M1 per class, console PMU.

## 4. First rows: NAPI0 (census, ready), NAPI1 (skeleton: written after the oracle reads the NDC_SPR record and pm_gpu_spr_build path,
which is today's closest native quad: pm_gpu.c:3950, nd_rec.c:220 nd_rec_spr), then S1 rows. Later stages: S1 2 rows, S2 4, S3 3, S4 about
15 (one per effect group), S5 about 10, S6 about 8.

## 5. Risks and blocks
- Combine and blend coverage: nd_pso_n64 reuses today's decision, so a moved site keeps its picture; the native description form comes
  later per site. TMEM load model: windows, scroll, palettes per tile (TILEWIN/TILEWIN2 rules) must map to NdTex + uv transforms.
- Decomp code that builds gfx in data (custom gfx builders, imgfx meshes, entity lists, map scripts): last, via host conversion.
- The matrix stack: call sites keep the game's float matrix (before guMtxF2L) for the API; sites that only have an Mtx convert once.
- Draw order and frame pipelining: the stream stays, so order and core 1 / core 0 overlap stay.
- Obsolete HC work: further cuts in classes A, B, E, G for the typed-op path (they leave core 0 at S6); HC7 rect path (replaced by S2-S3);
  HC8 sprite/entity steps 2-4 stay (H and I remain). Keep HCM3 as the per-stage measure; keep PMU runs per stage.
- PBATCH2: not needed: pink_sparkles costs 0.08 ms on the XL without FILLA0 (build 4324), so its 44 draws are cheap; do not merge
  4ab13ab4e (the fill cost came from FILLA0). fxflush_pink: no lasting bench (phase=1 gives PG0 ad hoc).
