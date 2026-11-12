# Endgame plan: remove the display lists completely (closed plan; ORACLE7, 2026-11-12, main 235bb8721)

The oracle writes only row_*.md files: this is plan_endgame.md under a row name. The orchestrator may `git mv` it to
docs/handoff/plan_endgame.md and add the INDEX.md line. Every row of the chain follows from the tables below; a row that moves no
counter of section 3 is not a row.

## 0. Rules of this plan
- Done = both proofs pass (section 4): the source counters are all 0 AND the ELF check of the final pm_3ds.elf finds no list
  symbol and no list data. Source-present code that the linker drops counts as gone only when the ELF check shows it gone.
  Nobody reports the epic done before both pass. Every stage report gives the remaining counts (LISTCEN2 output), not a verdict.
- Freeze (user, 2026-11-12): running agents finish; nothing new launches until this plan is accepted. Row files not named in
  section 5 are closed when the plan is accepted (the orchestrator deletes them in one commit): S7TEX2, S7MDL, S7ENT1, S7ENTX,
  S7ENT3, S7FLAME, S7EFX7/8 if not started, and the old S5*/S6M*/S1PREV/S2W/S5PQUAD2 rows. Their scope is inside the families below.
- User code rules: no stubs, no stand-ins, no toggles, no feature switches, no culling. Debug check switches (psochk, gtexchk)
  are debug tools, not feature switches: they compare, they never choose the draw path.
- Discovery rule: an item found later is appended to section 7 with the family row that covers it; the totals of section 6 are
  recomputed. The plan is not redesigned.

## 1. Census from the code (main 235bb8721; scratch census: TARGET_3DS-active lines vs N64-only `#else`/`#ifndef` lines)
listcen.py totals: build 4468, gfxpos 3967, data 22134, data_arrays 1873, gfxtype 3469, reader 714, model 15.
Group rows are sums of the per-file census (within a few counts; LISTCEN2 prints the exact values). Active = compiled for the 3DS. n64 = inside `#ifndef TARGET_3DS` or the `#else` of `#ifdef TARGET_3DS`.
| group | files | build active | build n64 | gfxpos active | gfxpos n64 | gs data | Gfx arrays |
|---|---|---|---|---|---|---|---|
| src/effects (127 effect files) | 127 | 1388 | 379 | 1365 | 438 | 2 | 7 |
| src/effects/gfx + assets/effects (static effect lists) | 738 | 0 | 0 | 0 | 0 | 6822 | 739 |
| src/world (45 files with calls) + assets/world | 116 | 383 | 42 | 379 | 47 | 3738 | 253 |
| src/entity (17 files with calls; model data 72 files) | 93 | 148 | 29 | 141 | 29 | 8112 | 751 |
| src/model.c, entity_model.c, animator.c | 3 | 155 | 49 | 100 | 35 | 541 | 83 |
| HUD and text: hud_element.c, msg_draw.c, msg.c, draw_box.c, windows.c | 5 | 402 | 15 | 388 | 19 | 60 | 11 |
| item_entity.c, imgfx.c, sprite.c, sprite_shading.c | 4 | 282 | 15 | 284 | 15 | 108 | 24 |
| overlays: screen_overlays.c, screen_render_util.c, curtains.c | 3 | 133 | 22 | 133 | 22 | 249 | 23 |
| backgrounds: background_gfx.c, background.c | 2 | 88 | 7 | 89 | 7 | 9 | 2 |
| title and logos: state_title_screen.c, state_logos.c, draw_img_util.c, titlemenu.c | 4 | 80 | 13 | 80 | 13 | 42 | 11 |
| src/battle (9 files with calls) + assets/battle, battle_ui_gfx.c | 27 | 44 | 36 | 39 | 41 | 577 | 44 |
| src/pause (7) | 8 | 72 | 0 | 72 | 0 | 293 | 15 |
| src/filemenu (4) | 6 | 65 | 0 | 69 | 0 | 99 | 5 |
| small root files: effect_utils, main, 43F0, cam_main, status_icons, 8a860_len_3f30, status_star_shimmer, src/evt, effects.c, entity.c, inspect_icon, i_spy, pulse_stone, speech_bubble, inventory, main_loop, src/os | 19 | 116 | 41 | 112 | 49 | 134 | 10 |
| star points and level-up letters (starpoint*.c, level_up*.c, assets) | 13 | 0 | 0 | 0 | 0 | 951 | 107 |
| include (gbi.h 382, gbi_custom.h 61, gs2dex.h 18, gt.h 3: the macro definitions) | 15 | 464 | 0 | 2 | 0 | 384 | 0 |
| TOTAL | | 3820 | 648 | 3254 | 715 | 22103 | 2107 |
Readers and the list state model in 3ds/src (reader-class hits / lines): nd_op.h 173/153, nd_rec.c 80/586, nd_walk.c 78/705,
nd_op.c 62/75, nd_scomp.c 49/354, nd_recs.c 48/333, nd_cen.c 42/601, nd_set.c 39/247, nd_model.c 33/678, nd_dlc.c 27/493,
nd_ent.c 21/447, nd_shape.c 12/953, nd_direct.c 11/370 (fallbacks), nd_gbi.h 9/231, nd_seam.c 9/428, nd_tex_set.c 7/725,
nd_gtab.c 6/87, nd_mtx_set.c 3/304, nd_cmd.h, nd.h, nd_api.h, pm_phase.c 1 each. Model-only files: nd_comb.c, nd_tex_set.c,
nd_texwin.c, nd_rect.c, nd_target.c (cimg roles), the gv walker part of nd_geom.c, pm_comb.c decide inputs. Generated:
nd_gfx_tab.c (612 Gfx uses: S7 v1 tables point into list units).

## 2. End-state architecture (the one replacement form per category)
| category | today | replacement (end form) | design row |
|---|---|---|---|
| pipeline state (combine, other mode, geometry, filter) | gDP/gSP state calls, typed units, run-time nd_comb decide | a static `NdPsoN64` description at the call site; gen_pso_tab (host) decides every description once at build time into `nd_pso_tab[]` (TEV and blend blocks); the device looks up by id, no decide code | NPSO |
| texture | TIMG/TILE/LOAD*/TILESIZE, TMEM model | `NdBindDesc` built from native fields at the call site (raster, palette, format, size, window) or by the converter; nd_tex_bind_desc | NPSO (desc form), CONV2 (static) |
| transform | gSPMatrix/PopMatrix list stack | native transform calls nd_xf_push(mtx, mul or load) / nd_xf_pop recorded in the native stream | NXF |
| geometry, built at run time | VTX/TRI into the stream | nd_mesh(pso, tex, vtx, n, idx, ni) and the existing nd_rect, nd_quad, nd_glyphs, nd_parts | NXF |
| static lists (src and assets Gfx arrays) | S7 v1 tables pointing into list units, walker for units | CONV2 native tables: per (list, entry pso) pair a sequence of native draws (pso id, bind desc, transform op, vertex range, index range), no pointer into a Gfx array; the Gfx arrays become host-only inputs and are unreferenced on the device | CONV2 |
| map shapes | .dlc v2 + nd_shape list parse | .dlc v3 from CONV2's draw form; nd_shape reads no list word | CONV2 (+ GSET-MAP) |
| model records | R0 render-mode lists, R13 aux ops, rec_apply list runs | records carry pso ids and bind descs from CONV2 (render modes are static lists) | NMDL |
| the stream | NDC_STATE typed units, NDC_NAPI | NDC_NAPI native records only (it stays the core 1 to core 0 pipe and the draw order) | S9DEL3 |
Proof while the old model still exists: debug switch `psochk` (NPSO) compares, at every native draw, the stated pso and bind with
what the walker state would give; `psochk bad 0` in the gate scenes is the per-row proof that no inherited state is missing.

## 3. Counters (LISTCEN2 prints each; the gate of every row names which it moves and to what)
C1 build_active, C2 gfxpos_active, C3 build_n64, C4 gfxpos_n64, C5 data (gs macros), C6 data_arrays, C7 gfxtype, C8 reader (per
3ds/src module), C9 model, C10 nd_rec_dl + nd_scomp_rec uses (S6/S7 forms: they are list forms too), C11 elf_func (list symbols in
pm_3ds.elf), C12 elf_data (Gfx array symbols and v1 table symbols in pm_3ds.elf). Final: every counter 0.

## 4. End proof (both parts; script rows LISTCEN2 and ELFGATE)
- Source: LISTCEN2 all classes 0, and `grep -rnE "\bg[DS]P[A-Za-z]|\bgs[DS]P[A-Za-z]|\bGfx\b|gMainGfxPos|NDO_|nd_ops_run|nd_walk_state|
  NDC_STATE|nd_rec_state|ENT_GBI" src include 3ds/src 3ds/include` returns nothing (3ds/tools keeps the host converter inputs).
- ELF (3ds/tools/elfgate.py, python ELF .symtab parse, no dependency): function symbols matching
  `^(nd_walk_state|nd_ops_run|op_typed|op_geo|walk_call|nd_op_apply|nd_op_same|nd_op_xlate|nd_rec_state|sync_combine|nd_comb_decide|
  decide|decide_miss|sync_render|set_tilesize|tex_win[01]|ensure_tile|ensure_tile_slow|nd_texrect|nd_fillrect|gv_tri|op_setcombine|
  load_mtx|ent_flat|ent_run|ent_compile|dlc_try|dlc_build|nd_gtab_run|gt_ops|nd_scomp_exec.*|nd_rec_dl|nd_set_(timg|tile|tilesize|
  loadblock|loadtile|loadtlut|texture)|gbi_.*|nd_lstcen|img_role|off_screen_check)$` (and GCC clones `.constprop`, `.isra`,
  `.part`): 0; data symbols `^ndg[odx]?_` (S7 v1 tables), `^gMainGfxPos$`, and every name of the Gfx-array list that
  elfgate.py builds from the source and asset scan (the gen_gfx_syms.py scan over all of src and assets/us): 0; and no
  DisplayContext member of type Gfx. Prints `elfgate func <n> data <m> gfxnames <k>`.

## 5. Row families, order and blockers
Phase A (start now): LISTCEN2 (Haiku: census v2, C1-C10 and the per-group view; row_LISTCEN2.md), ELFGATE (Haiku: the script
3ds/tools/elfgate.py for C11-C12 now; its final run is S9GATE; row_ELFGATE.md),
N64DEL1 (after LISTCEN2), N64DEL2 and N64DEL3 (after N64DEL1's tool) (Haiku: delete every N64-only branch; decision 2026-11-12, the port stops building for N64; they move C3
and C4 to 0: 648 + 715): N64DEL1 src/effects (106 files, 373 blocks), N64DEL2 src/world, src/entity, src/battle, src/pause,
src/filemenu, include (42 files, 112 blocks), N64DEL3 root src files (27 files, 245 blocks).
Phase B (design cores): NPSO (Opus, start with A), CONV2 (Opus, branches after NPSO's NdPsoDesc commit, merges after NPSO),
NXF (Sonnet, after NPSO: nd_xf_* and nd_mesh records). Row files: row_NPSO.md, row_CONV2.md, row_NXF.md.
Phase C (pilots, after NPSO and NXF; CONV2 for the ones that call static lists), Sonnet, each writes the pattern its Haiku rows copy:
NFX0 (effects, 8 files), NENT0 (src/entity setupGfx, 17 files, 148 calls), NWLD0 (src/world, 6 files), NMDL1-2 (model.c,
entity_model.c, animator.c: 155), NHUD (hud_element 145), NMSG1-2 (msg_draw 134, msg 31), NBOX (draw_box 83 with the ROTSCALE
off-screen pass, windows 9), NIMG1-2 (imgfx 122), NSPR (sprite 34, sprite_shading 23), NITEM (item_entity 103), NOVL
(screen_overlays 50, screen_render_util 70, curtains 13), NBG (background_gfx 65, background 23), NMENU1-2 (pause 72, filemenu 65),
NBTL (src/battle 44). 18 Sonnet rows.
Phase D (Haiku waves, after their pilot): NFX1..NFX15 (the other 119 effect files, 8 per row, about 87 calls each), NWLD1..NWLD6
(39 files), NTITLE (state_title_screen, state_logos, draw_img_util, titlemenu: 80), NMISC1-2 (the small root files: 116),
GSET1..GSET6 (CONV2 sets: effects, entity, world, model render modes, star points and level-up, battle and UI arrays: they move C5,
C6 for their sets and C12 once the device stops referencing the arrays). 30 Haiku rows.
Phase E (deletes, in order, Sonnet; each needs C1, C2, C10 at 0 for the code it deletes): S9DEL1 (list runners, ENT_GBI, nd_dlc run
compile), S9DEL2 (decide, sync_combine, tile and TMEM model, nd_rect, gv walker; decision only in gen_pso_tab), S9DEL5 (new: S7 v1
tables, nd_gtab unit form, nd_scomp run-time exec, nd_shape list parse, nd_seam list reads, nd_direct fallbacks), S9DEL3 (NDC_STATE,
nd_rec_state, nd_op.h, nd_gbi.h, nd_set op table), S9DEL4 (Gfx type, gMainGfxPos, gbi.h includes, include/PR/gbi.h macros, the
DisplayContext Gfx buffers). Then S9GATE (Haiku, the final run of LISTCEN2 and elfgate.py): section 4 both parts, the DIRECT byte set, battle, pause, talk and file
select scenes, and one console push (C5 batch).
Blockers: NPSO and CONV2 need the user's yes for Opus. Console: one push after S9GATE. No other user decision is open.

## 6. Totals and wall time
Rows left: Haiku 5 (A) + 30 (D) + 1 (S9GATE) = 36; Sonnet 1 (NXF) + 18 (C) + 5 (E) = 24; Opus 2. Total 62, plus about 20 %
retries (12) = about 73 row runs. Rate this session: 79 merges since 02:00 (14 h, 5.6 per hour, mostly small rows). Critical path:
B (about 5 h) -> C pilots (about 4 h, parallel) -> D waves (30 rows at about 4 per hour = 8 h, overlapped with C) -> E (5 rows in
sequence, about 2.5 h each = 12.5 h) -> S9GATE and the console run (2 h). Ideal about 30 h of continuous running; honest estimate
with retries and tool faults 40 to 50 h, that is 2 to 3 days.

## 7. What the census cannot see, and how it is covered (discoveries are appended here)
| item | covered by |
|---|---|
| lists built at run time into heap or local buffers (effects writing strips per frame, imgfx meshes, sprite quadBuffers, model vertex copies) | C1 counts every gDP/gSP call whatever the packet pointer; the family row of the file converts it to nd_mesh |
| map shape lists inside the ROM image (resident blob, not the ELF) | CONV2 + GSET-MAP make every map's .dlc v3; S9DEL5 deletes nd_shape's list parse, so no list word of the ROM is read; the blob itself leaves with ASSETFREE (parked) |
| battle stage shapes and other ROM-loaded shapes | as map shapes (GSET-MAP takes every *_shape the game loads) |
| segment-addressed lists (0x09 effect segment, entity vertex segments) | CONV2 maps by symbol; a run-time segment base becomes a native vertex base in the draw |
| lists named through data tables (preset->dlist, model texture lists) | CONV2 tables keyed by list address (nd_gtab_find v2); the call site keeps the pointer |
| version-conditioned data (#if VERSION_PAL) | the GSET scan reads only the active branch (S7ENTX form, moved into GSET2) |
| S6 compiled defs in game code (static const Gfx X_u[] for nd_scomp_rec) | counted in C5, C6, C10: their call sites move to NPSO descs in the family rows |
Discoveries: (none yet)

## 8. Earlier stages that were not tied to the end state (so they did not move these counters)
S6T/S6E/S6M (compiled ranges: still typed units and run-time decide), S7CONV/S7RUN/S7EFX1-6/S7MAP (tables that point into list
units; S7EFX swapped only gSPDisplayList: C1 in src/effects is still 1388), NAPI2B/S1/S5 direct executors (each kept the old ops
as a fallback), S7TEX2 and S7MDL as written (walker state kept). From now on a row's gate is its counters going to 0 for its
files plus the byte set; a file-local byte pass alone is not a row result.
