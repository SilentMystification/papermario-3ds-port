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
- PBATCH2: closed, not merged (two failed tries; branch PBATCH2 kept). The earlier claim that its 44 draws are cheap came from an outlier XL run
  (build 4324, effects probably not drawn). XL runs xl2 and xl3 give pink_sparkles 6.0 ms. The effect cost is GPU and per-part work, not
  draw count: see the effect cost model in row_ORACLE_NEXT.md. fxflush_pink: no lasting bench (phase=1 gives PG0 ad hoc).

## 6. S7 table form (row S7CONV design note; tool 3ds/tools/nd_gfx_conv.c, API 3ds/include/nd_gtab.h, host compare nd_gtab_test.c)
Code read of main e3ada758a.

1. Input build. One shell step, 3ds/tools/gen_gfx_tab.sh <out.c>, run inside docker pm3ds-host:1 by build.sh (gen_gfx_tab, after
   gen_shapes, skipped by a stamp of its inputs) and by nd_host_test.sh. It reads the gfx sources of the converted set (this row:
   src/effects/gfx/<g>.c for effect_gfx_<g> of the 7 effects: sparkles, effect_3D, snowfall, snaking_static, water_block, embers;
   the set is the list GCONV_SRC at the top of the script, which each S7 row extends), compiles each with gcc -m32 and the device
   defines (TARGET_3DS VERSION_US F3DEX_GBI_2 _LANGUAGE_C _MIPS_SZLONG=32, include paths of 3ds/CMakeLists.txt), so every gs macro
   gives the same typed unit as on the device (nd_gbi.h ND_GS). Host-only override: 3ds/tools/gconv_inc/include_asset.h (first on
   the include path) makes INCLUDE_IMG/PAL/RAW with `.balign 8` (x86 gas reads `.align 3` as bytes) and a `<NAME>_gend` label after
   the incbin (the image size). Output: 3ds/src/nd_gfx_tab.c (git-ignored, never committed, as map_restore_table.c: CMake reads only
   the synced code dirs, so the file sits in 3ds/src, not build3ds/gen), in the build key.
2. Symbol map. 3ds/tools/gen_gfx_syms.py (python3, in the same docker step) reads each input .c and its `#include "*.inc.c"` files
   (under assets/us) and lists `Gfx NAME[] =`, `Vtx NAME[..] =` and INCLUDE_IMG/PAL/RAW NAME. It writes a header for the tool: the
   extern declarations and `static const GSym gsym[] = { {"NAME", kind, addr, size}, ... }` (size = sizeof, or NAME_gend - NAME for
   an image). The tool (3ds/tools/nd_gfx_conv.c) is linked with the host objects of the inputs, so a pointer word p of a list is
   reverse-mapped to the symbol s with s.addr <= p < s.addr + s.size and written as symbol + offset: the same link address on the
   device. A pointer in no symbol (a segment address, data outside the set) is not mapped: its unit stays a run-time unit (the
   device reads the word, ptr_of resolves it).
3. Table form (3ds/include/nd_gtab.h). One NdGTab per Gfx symbol of the set, the list in unit order as 8-byte ops:
   `typedef struct NdGOp { uint8_t k, a, b, c; const void* p; } NdGOp;`
   | k | a, b, c | p | from |
   | NDG_VTX | a = n (1 to 64), b = v0 (first slot) | const Vtx* (symbol + offset) | NDO_VTX with a mapped source |
   | NDG_TRI | the three slots (index / 2) | NULL | NDO_TRI1; NDO_TRI2 gives two ops |
   | NDG_DEF | a = units | const NdGDef* | a state run accepted by nd_scomp_analyse (below) |
   | NDG_UNIT | a = 1 | const uint32_t* (the unit in the list) | every other unit: run-time values (MTX, POPMTX, MOVEMEM, MW SEG,
     SCISSOR, FILL, FOG, BLEND, PRIMZ, CIMG), RARE, RECT and its halves, MODVTX, a VTX or CALL with an unmapped pointer |
   | NDG_CALL | a = 1 for the branch form | const NdGTab* (the callee) | NDO_CALL to a list of the set |
   | NDG_END | - | NULL | NDO_END (the last op; units after it are not converted) |
   Sync and no-op units (ND_NOP_A) give no op (every reader skips them).
   `typedef struct NdGDef { NdSDef d; uint32_t inherit; uint16_t ctx; uint16_t k0; } NdGDef;` d is the S6T def made on the host by
   nd_scomp_analyse itself (d.u = the list + 2 * k0 words, d.n, st = 1, id = the def index & 0xff, nv, pre, vmask, the folded
   writes), so core 1 never analyses a converted def; nd_scomp_exec_own(&g->d) runs it (st 1: no write to the table). Def split: the
   units between two draw or control units form a state run; the tool takes from its start the longest prefix (at most 32 units)
   that nd_scomp_analyse accepts as one NDG_DEF, else the first unit as one NDG_UNIT, and repeats. Texture units (TIMG, TILE,
   TILESIZE, LOAD*) are ordered value units of the def (SCOMP0 class 4), run by the def from the list words. The NdBindDesc form of
   a load is not in this row's tables: tex_win0 lives in nd_tex_set.c next to nd_ws, and a host build needs that pure part split out
   first; the S7EFX row that moves the first setup list does it, with nd_tex_win_desc as the function (a new op NDG_TEX, p = const
   NdBindDesc*, made by the tool for a load whose inputs are all mapped).
   `typedef struct NdGTab { const uint32_t* list; const NdGOp* op; uint16_t nop, flags, nvtx, ntri; } NdGTab;` nvtx = the vertices
   of all loads, ntri = triangles. flags NDG_F_PARTS: a flat particle list (the parts_ok rule: op 0 a VTX at slot 0, then only VTX
   and TRI to END, each later load starts at a slot <= the slots loaded so far, every triangle index below them).
   Names: `ndg_<SYM>` (the table), `ndgo_<SYM>` (ops), `ndgd_<SYM>_<k0>` (defs), all const. Declare with NDG_DECL(SYM), take with
   NDG(SYM) (macros of nd_gtab.h).
4. Lookup. `const NdGTab* nd_gtab_find(const void* list)` (3ds/src/nd_gtab.c): binary search of nd_gtab_idx[] ({list, tab},
   generated, nd_gtab_n entries), sorted by address once at the first call (link addresses are unknown when the file is made).
   Caller: core 1 only (the record carries the table pointer to core 0; core 0 never looks up). nd_parts_add keeps a memo of the
   last 8 {list, tab} (the per-call-site memo). A list with no table, or a table without NDG_F_PARTS, takes the stream form (as a
   parts_ok failure did).
5. First use. nd_parts_add puts the table pointer in the instance's call unit (w1); nd_parts_run and nd_parts_run_v (nd_walk.c,
   one line each) pass it to `int nd_part_draw(const NdGTab* t)` and, when it returns 0 (lit, no ring room), run t->list as before.
   nd_part_draw reads only ops: the room check is t->nvtx, the loads and triangles come from the op array (sequential 8-byte reads,
   no list word read). parts_ok is deleted.
6. Inherited mask (the S9DEL2 condition). Per def the tool prints `gconv def <SYM>+<k0> n <units> inherit 0x<mask> ctx 0x<ctx>`.
   mask = the NdCombIn input groups of nd_scomp_key that a static list can set and this def does not set before its decision (the
   static block plus the pre value units; units after the block run on the op path and decide again there):
   NDG_IN_COMB 0x01 (w0, w1, tst[16], use_t1, box_prod, box_folded, comb_set, comb_alpha_shade: set by a COMB unit), NDG_IN_CYC
   0x02 (the two omode_h cycle bits: set when omh_m holds both), NDG_IN_TEXON 0x04 (G_TEXTURE), NDG_IN_PRIM 0x08 (prim and prim_lod
   low 8: a pre PRIM unit), NDG_IN_ENV 0x10 (a pre ENV unit), NDG_IN_KEY 0x20 (key_c, key_s, conv_k4: RARE units are refused by
   nd_scomp_analyse, so a def always inherits them; a call site closes them by the static values of its record). ctx = inputs no
   list sets, keyed by the record kind, never closed by list state: NDG_CTX_T1V 0x1 (td[1].valid, the texture model), NDG_CTX_DUAL
   0x2 (dual_on), NDG_CTX_VPRIM 0x4 (pm_comb_vprim, 1 only in particle records); ctx is printed as 0x7 for every def.
   Rule: a def with mask 0 may take its decision at build time keyed only by ctx (one decision per ctx value its call sites can
   have); mask != 0 keeps the S6T run-time key. S9DEL2 gate: every def mask 0 (KEY closed by its call site), ctx keyed per runner.
   The table stores inherit and ctx in NdGDef, so a row can test them at run time (debug) without the tool output.
7. Dynamic producers (no table; data written at run time): imgfx.c (meshes, vertex colours, palettes), hud_element.c (per-element
   quads), sprite.c quadBuffers and palettes, model.c vertex copies of animated models (its Gfx_RM* render-mode lists are static:
   S7MDL), actor palette swaps and tattle lerps (palettes edited in place), effects that write vertex or list data per frame into
   gMainGfxPos or effect heap: chomp_drop, confetti, effect_65, firework, green_impact, hieroglyphs, lightning_bolt, red_impact,
   shape_spell, shimmer_burst and shimmer_wave (their built strips, not the particle lists), shockwave, squirt, stop_watch,
   thunderbolt_ring, water_block (its built mesh), waterfall, whirlwind. Map and entity data loaded at run time (pointers by segment
   address) is outside the set until S7MAP/S7ENT add a host load of it (an unmapped pointer stays a run-time unit).
8. Host compare (3ds/tools/nd_gtab_test.c, in nd_host). Linked with the host objects of the set, the generated table and
   nd_scomp.c (ND_SCOMP_HOST). For every table: (a) the old reader, the parts_ok rule and the nd_part_draw list walk (copied from the
   code this row deletes), on the list words gives the parts verdict, the loads {src, n, v0} and the triangle triples; the table
   gives the same from its ops; they must be equal (verdict, every load, every triple, nvtx, ntri). (b) Round trip: the ops cover
   every unit of the list once in order (UNIT p is the unit address, DEF k0 and n the unit range, VTX/TRI/CALL match their word,
   CALL p->list is the call word). (c) Defs: a copy of the def with st 0 run through nd_scomp_analyse gives the table's fields (st,
   nv, pre, vmask, has_*, last_op, masks, words), and the mask rule of 6 on it gives the stored inherit. Prints `gtab lists <n>
   parts <p> defs <d> ops <o> bad <b>`; any difference prints the list and op and fails nd_host.
