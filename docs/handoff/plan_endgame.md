# Endgame plan: remove the display lists completely (closed plan, version 2; ORACLE7, 2026-11-12)

The oracle writes only row_*.md files: this is the full new text of plan_endgame.md. The orchestrator replaces plan_endgame.md with
it (`git mv -f docs/handoff/row_ENDGAME.md docs/handoff/plan_endgame.md`). Every row follows from the tables below; a row that
moves no counter of section 3 and builds no part of the test harness of section 5 is not a row.

## 0. Rules of this plan
- Done = both proofs pass (section 4): the source counters are all 0 AND the ELF check of the final pm_3ds.elf finds no list
  symbol and no list data. Source-present code that the linker drops counts as gone only when the ELF check shows it gone.
  Nobody reports the epic done before both pass. Every stage report gives the remaining counts (LISTCEN2 output), not a verdict.
- Every compare is exact: one differing bit fails (user, 2026-11-12). No screenshot is stored or compared (user, 2026-11-12).
- Test harness first (user, 2026-11-12): the host harness and the golden captures of the current code come before the conversion
  waves; the captured golden files are never edited after their capture row merges.
- Freeze: nothing new launches until the user accepts this plan. Row files not named in section 6 are closed on acceptance (the
  orchestrator deletes them in one commit): S7TEX2, S7MDL, S7ENT1, S7ENTX, S7ENT3, S7FLAME, S7EFX7/8 if not started, and the old
  S5*, S6M*, S1PREV, S2W, S5PQUAD2 rows. Their scope is inside the families below.
- User code rules: no stubs, no stand-ins, no toggles, no feature switches, no culling. Test inputs (fixture files of fixed values
  under 3ds/tests) and debug check switches (psochk, fhash) are tests, not product code: they read, they never choose a draw path.
- Discovery rule: an item found later is appended to section 7 with the row that covers it; section 8's totals are recomputed.
  The plan is not redesigned.

Model rule (user, 2026-11-12, after v2 was written): most rows, if not all, are Haiku-sized. A row that Haiku cannot do gets a spec for the model that can. No hard rule picks the model. No stronger model without value. Before the plan is shown again the oracle re-cuts the 27 Sonnet rows and the 2 Opus rows below into Haiku-sized pieces (one file, one function family or one call-site group, exact edit, one gate) and keeps a Sonnet or Opus row only for a design core with a stated reason. The row counts and the wall time in section 8 change with the re-cut.

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
what the walker state would give; `psochk bad 0` in the gate scenes is the large-group proof that no inherited state is missing.

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

## 5. Gates (row_TESTIDEAS.md G1-G12; adopted by the user 2026-11-12)
| gate | where | exact rule |
|---|---|---|
| G8 compile ban | every converted file includes 3ds/include/nd_nolist.h (poisons gDP*/gSP*/gs*/Gfx/gMainGfxPos); native draws take a static ND_PSO_DESC | the build fails on one leftover |
| Golden master + gcov | small and medium groups (about 2460 of 3356 call sites, 73 %): the converted draw functions run on the host with the fixed test inputs of their GOLD row and print the same draw records as the golden file captured from the old code; gcov shows 100 % of the converted functions' lines run | text equal byte for byte; coverage 100 % |
| G1 host differential | every static list (CONV2 and GSET rows): old model vs native table, per draw | field equal |
| psochk + frame hash | large groups (about 900 sites, 27 %: model, HUD, messages, imgfx, sprites, item entities) in scenes the coverage map (G7) shows run at least 90 % of the row's sites; phase ends | psochk bad 0; every frame hash equal |
| G6 PICA validator | on the frame-hash capture of each phase end and each large-group row | 0 findings |
| G11 console soak, G10 dump pull | each phase end (one push, user action) | crash.txt empty, no hang dump |
| G9 end proof | final | LISTCEN2 all 0, elfgate func 0 data 0 gfxnames 0 |
| N64DEL gate | N64DEL1-3 | `gcc -E -P` with the device defines gives identical output before and after, per file |
Today's `bench.py base` + `bench.py cmp` on ppm shots is no longer a gate anywhere.
Rule text for CLAUDE.md rule 2 (the user confirms): "2. NO VISUAL CHECKS AND NO SHOTS. No agent looks at a shot, describes a
picture, counts pixels or compares images. Proof is the code read and exact compares: nd_host, the golden draw records of the
converted functions (3ds/tests/golden, gcov 100 % of their lines), and for large inputs and phase ends the frame hash (`bench.py
base <scene>_hash`, then `bench.py cmp <scene>_hash --row <row>`) with psochk. One differing bit fails. The user checks the look
on the console." (Rule 4's exact-compare text is already in, b9aff62a6.)

## 6. Row families, order and blockers
Phase H (harness and scripts; start now unless a blocker is named):
| row | model | blocker | builds |
|---|---|---|---|
| LISTCEN2 | Haiku | none | census v2, counters C1-C10 (row_LISTCEN2.md) |
| ELFGATE | Haiku | none | elfgate.py, counters C11-C12 (row_ELFGATE.md) |
| NOLIST | Haiku | none | G8: 3ds/include/nd_nolist.h and a build check that a poisoned name fails |
| HOSTCORE | Sonnet | none | the draw core on the host down to the emit boundary, writing one draw record per draw (pso output, bind content, transform rows, vertex and index bytes) |
| GMHARN | Haiku | HOSTCORE | one host test program per game file (the file, the real recorder, gu math, HOSTCORE, its test-input file), text output, a runner in nd_host |
| GMCOV | Haiku | GMHARN | gcov of the host runs, per function line coverage |
| FRAMEHASH | Sonnet | none | the frame hash (fhash) and the `<scene>_hash` benches, for large groups and phase ends (row_FRAMEHASH.md, reduced role) |
| PICAVAL | Sonnet | FRAMEHASH | G6: host validator over captured command words |
| COVMAP | Haiku | FRAMEHASH | G7 for the large-group files only: per-site counters in the scenes |
| N64DEL1, N64DEL2, N64DEL3 | Haiku | LISTCEN2 (N64DEL2/3 also N64DEL1's tool) | C3, C4 to 0 |
Phase G (golden capture from the current code; Haiku; blocker GMCOV; each row writes test inputs until gcov shows 100 % of its
draw functions' lines, captures the text twice, commits it under 3ds/tests/golden/<group>/):
GOLD-FX1..GOLD-FX16 (src/effects, 8 files each, about 87 call sites), GOLD-ENT1..2 (src/entity setupGfx, 17 files), GOLD-WLD1..5
(src/world, 45 files), GOLD-OVL (screen_overlays, screen_render_util, curtains), GOLD-BG (background_gfx, background), GOLD-TTL
(state_title_screen, state_logos, draw_img_util, titlemenu), GOLD-BOX (draw_box, windows), GOLD-MISC1..2 (the small root files),
GOLD-PAUSE, GOLD-FMENU, GOLD-BTL. 32 rows. They run in parallel with phase B and use idle slots.
Phase B (design cores): NPSO (Opus; blocker GOLD-TTL for its pilot draw_img_util), CONV2 (Opus; branches after NPSO's NdPsoDesc
commit, merges after NPSO; G1 on its 6 pilot lists plus GOLD-FX of their files), NXF (Sonnet; after NPSO; pilot files' GOLD rows).
Phase C (pilots, Sonnet; each needs NPSO and NXF, CONV2 where it calls static lists, and its GOLD rows): NFX0, NENT0, NWLD0, NOVL,
NBG, NBOX, NMENU1 (pause), NMENU2 (filemenu), NBTL: 9 rows, golden + gcov gates. Large groups (Sonnet; need NPSO, NXF, FRAMEHASH,
COVMAP, PICAVAL): NMDL1, NMDL2, NHUD, NMSG1, NMSG2, NIMG1, NIMG2, NSPR, NITEM: 9 rows, psochk + frame hash + G6 gates.
Phase D (Haiku waves after their pilot; golden + gcov + G8): NFX1..NFX15, NWLD1..NWLD5, NTITLE, NMISC1..2 (23 rows); GSET1..GSET6
(CONV2 sets: effects, entity, world, model render modes, star points and level-up, battle and UI arrays; G1 gate; they move C5,
C6 and C12). 29 rows.
Phase E (deletes, Sonnet, in order; each needs C1, C2, C10 at 0 for the code it deletes; gate: frame-hash set + G6 + nd_host):
S9DEL1, S9DEL2, S9DEL5, S9DEL3, S9DEL4. Then S9GATE (Haiku): G9, the frame-hash set, the console batch (G10, G11).
Phase ends (after B, C, D, E): one frame-hash set (spot_mac01_hash, mdl5_iwa10_hash, spot_mac00_hash plus the group scenes), G6
on its capture, one console soak push (user).
First rows to start now (disjoint files): LISTCEN2, ELFGATE, NOLIST, HOSTCORE, FRAMEHASH, NPSO (needs the user's yes for Opus;
it may start its design part at once, its pilot gate waits for GOLD-TTL). Next when they merge: N64DEL1 (after LISTCEN2), GMHARN
(after HOSTCORE), PICAVAL and COVMAP (after FRAMEHASH), CONV2 (after NPSO's first commit).

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

## 8. Totals and wall time (recomputed with the harness and the golden rows)
| model | phase H | G | B | C | D | E | total |
|---|---|---|---|---|---|---|---|
| Haiku | 9 (LISTCEN2, ELFGATE, NOLIST, GMHARN, GMCOV, COVMAP, N64DEL1-3) | 32 | 0 | 0 | 29 | 1 | 71 |
| Sonnet | 3 (HOSTCORE, FRAMEHASH, PICAVAL) | 0 | 1 (NXF) | 18 | 0 | 5 | 27 |
| Opus | 0 | 0 | 2 (NPSO, CONV2) | 0 | 0 | 0 | 2 |
Total 100 rows (Haiku 71, Sonnet 27, Opus 2), plus about 20 % retries: about 120 row runs. Rate this session: 79 merges in 14 h
(5.6 per hour, mostly small rows); with 6 coders about 4 rows per hour for Haiku-sized rows.
Critical path: HOSTCORE (about 4 h) -> GMHARN and GMCOV (about 2 h) -> the GOLD rows of the pilot groups (about 2 h; all 32 GOLD
rows take about 8 h of slots, in parallel with NPSO and CONV2, about 6 h) -> pilots (about 4 h) -> D waves (29 rows, about 7 h)
-> E (5 rows in sequence, about 12 h) -> S9GATE and the console batch (about 2 h). Ideal about 33 h of continuous running;
honest estimate with retries and tool faults 45 to 60 h, that is 2.5 to 3 days. The golden rows add about 32 rows but remove most
Azahar runs from the waves (seconds per rerun instead of 20-40 minutes per try).

## 9. Earlier stages that were not tied to the end state (so they did not move these counters)
S6T/S6E/S6M (compiled ranges: still typed units and run-time decide), S7CONV/S7RUN/S7EFX1-6/S7MAP (tables that point into list
units; S7EFX swapped only gSPDisplayList: C1 in src/effects is still 1388), NAPI2B/S1/S5 direct executors (each kept the old ops
as a fallback), S7TEX2 and S7MDL as written (walker state kept). From now on a row's gate is its counters going to 0 for its
files plus the gates of section 5.
