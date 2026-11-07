# Plan S10 path: producers write native records, then pm_gbi.c is deleted (oracle, 2026-11-06)

User decision 2026-11-06: commit to S10 now. Runs in parallel with plan WEND (docs/handoff/plan_walker_end.md). Supersedes the stale S4 to S10 rows of plan_native_slices.md where they differ.

## Oracle review CORE record and CORE-3 (2026-11-06, Opus; replaces the CORE-3 G_PM_ND reply and the opt-in steps of the oracle plan CORE-2)
(1) Record word, 64d344659: confirmed. The N64 word is self-contained: opcode plus fields, and a pointer that is read when the op runs. Re-encoding it buys nothing, so the word is the record, and decoding its fields in the setter is the record reader. My G_PM_ND layout was the wrong tool for this.
What follows from it (decided):
- At 2da2a6947, nd_rec_leaf is a memcpy of the leaf words, and the bridge leaves (nd_gbi.h) no longer decode. Both force-includes are now layers that change nothing. Delete nd_rec.h, nd_rec_leaf.h, nd_gbi.h, nd_gbi_leaf.h, the gen_nd_gbi.py record and bridge output, and the CMake lines 338-339, in CORE-3 (rule 7: no dead layers). Keep nd_rec_test's "replay state equals word state" check, rewritten against the dispatcher.
- So "opt a file in" no longer means anything. Producer work in UI, FXW and WBO shrinks to:
  1. replace the custom word hacks (G_PM_TEXI, G_PM_RM, the gPmSplashMark NOOP) with handled ops or native-only records;
  2. the WEND draw paths;
  3. side-buffer lists compiled through nd_dlc.
  Per-file force-includes are dropped from plan_s10_path.md.
- S10 now means: the dispatcher (opcode to handler table) and every handler live in nd_state.c, nd_rect.c, nd_geom.c, nd_mtx.c, nd_dlc.c and nd_core.c; pm_gbi.c is gone. There is one decoder for both walked lists and nd_dlc compile, never a second copy.
- Rule for G_PM_ND: only for native-only payloads that have no N64 encoding (glyph by sheet and index, splash on/off, target role, the TEXI/RM replacements). It never wraps N64 words.
(2) CORE-3: replaced. Texrect and fillrect keep their words. G_TEXRECT is the fixed F3DEX2 three-slot form (TEXRECT, then RDPHALF_1, then RDPHALF_2), and gbi_texrect already reads the two halves. G_FILLRECT is one slot, so wrapping it in G_PM_ND would also break rule (1).
CORE-3 corrected:
- Move gbi_texrect and gbi_fillrect (plus FILL_SX10 and the HUDB1 inclusive-corner rule) to nd_rect.c as the handlers of G_TEXRECT and G_FILLRECT. They read nd_ws and call pm_gpu_tex_rect / pm_gpu_fill_rect.
- The pm_gbi.c pc loop and the g1/g2 tables call them through the dispatcher entry.
- No G_PM_ND, no opt-in. Delete the bridge and record layers as above.
- The census counts ops by handler module, not by "word vs record".
- Gate: for camera_render and the 2 background lists, ops served by pm_gbi.c handlers = 0 (all in module nd_rect); mac_00 sky and mac_01 byte-equal against the CORE-2c build; pause sky by eye.
S10 precondition (census over the full catalog plus a grep). All must hold:
1. Per-opcode census of every walked list (main, bg, key replay, ND_CALL side lists): every opcode seen has its handler outside pm_gbi.c, and the "legacy handler" counter is 0.
2. G_PM_TEXI, G_PM_RM and the splash NOOP mark: 0 hits, and grep `G_PM_TEXI|gSPPmRenderMode|SPR_RM_LIST|gsSPPmTexInline|gPmSplashMark` over src and 3ds is empty.
3. wd = 0 (WEND).
4. No pm_gbi.c function has a caller outside pm_gbi.c (nm or grep of every exported pm_gbi_* symbol). The structured records G_PM_REC, G_PM_SPR and G_PM_ENT are allowed; their handlers move to nd_model.c, nd_sprite.c and nd_dlc.c in CORE-8.
5. Opcodes with 0 hits and no macro caller are deleted, not moved (WEND END-1).

## Producer inventory
How the frame runs today:
- pm_nusys.c gbi_task calls nd_bridge_cut on the whole main list, so the list is one LEGACY span, and nd_emit then hands it to pm_gbi_run.
- nd_gbi.h (the bridge form of the gSP/gDP leaves) is force-included only in main_loop.c, cam_main.c, background.c and background_gfx.c (CMakeLists.txt:338). It writes the words and also decodes them into nd_st (shadow state only).
- Word records already in use (include/gbi_custom.h): G_PM_REC (model.c), G_PM_SPR (sprite.c), G_PM_ENT (entity_model.c, item_entity.c gPmEntDrawRecord, animator.c), G_PM_TEXI (hud_element.c, msg_draw.c, Shadow.c, sprite.c), G_PM_RM (sprite.c, entity_model.c), and the gPmSplashMark NOOP (water_splash.c). Grep `G_PM_TEXI|gSPPmRenderMode|SPR_RM_LIST|gsSPPmTexInline|gPmSplashMark` over src: 28 hits in 7 files.
- Count of GBI macro sites: `grep -rhoE 'g[DS]P[A-Za-z0-9]+\(' <path> | wc -l`.
| slice | producers (sites) | record form needed |
|---|---|---|
| S4 left over | sprite.c 36 (TEXI, SPR_RM_LIST), imgfx.c 111, sprite_shading.c | ND_SPRITE (G_PM_SPR as a struct); imgfx goes through ND_RANGE |
| S5 | hud_element.c 125 (shared with session HUD1), msg_draw.c 116, msg.c 31, draw_box.c 77, windows.c 9, draw_img_util.c 21, status_icons.c 13, status_star_shimmer.c 11, inventory.c 1 | ND_RECT, ND_FILL, ND_GLYPH (sheet, palette, index, x, y), state setters |
| S6 left over | entity_model.c 97 (G_PM_RM), Shadow.c (TEXI), item_entity.c 110, animator.c 36 | ND_ENT (G_PM_ENT as a struct), ND_CALL for model lists |
| S7 | src/world: 41 files, 412 sites (custom gfx, e.g. mac_01 crystal_ball_gfx, flo_10 waterfx, hos_05 intro); src/battle: 9 files, 73 sites | force-include (record mode); side-buffer lists through ND_CALL |
| S8 | src/effects: 127 files, 1723 sites; effects.c 8 | ND_RANGE (VTX+TRI), ND_CALL (static effect gfx data D_09xxxxxx), ND_SPLASH(on/off) |
| S9 | screen_overlays.c 71, screen_render_util.c 70, curtains.c 13, pause: 7 files 70, filemenu: 4 files 60, state_title_screen.c 46, worker.c (calls producers), main_loop.c 5 | ND_FILL, ND_RECT, ND_TARGET(role) for gDPSetColorImage (grep `gDPSetColorImage` in src: 17 files) |
| S2/S3 | cam_main.c 26, background*.c 87, model.c 120 | ND_MODEL (G_PM_REC as a struct), ND_RECT; already bridged |
Data that is not a producer: the N64 words in asset lists (map shapes, entity and effect gfx D_09xxxxxx, animator heap lists). These are compiled, not walked: nd_dlc.c compile plus nd_state.c nd_words (D3).

## Streams
| id | slices | owns (files) | must land first |
|---|---|---|---|
| CORE | the recorder (S1/S2 finish) and the moves out of pm_gbi.c | nd_core.c, nd_state.c, nd.h (NdRec), 3ds/tools/gen_nd_gbi.py, nd_gbi.h, nd_gbi_leaf.h, new nd_imm.c; each pm_gbi.c function only after the WEND row that owns it has landed | CEN-1b |
| UI | S5 and S9 | the S5 files, screen_overlays.c, screen_render_util.c, curtains.c, src/pause, src/filemenu, state_title_screen.c, main_loop.c, worker.c; hud_element.c and status_* last (D7) | CORE-1 to CORE-3, RECT-1 |
| FXW | S8 | src/effects/*.c (energy_orb_wave.c and radial_shimmer.c only after FX closes), effects.c, effect_utils.c | CORE-1 to CORE-4, RECT-2 |
| WBO | S7, plus the S4/S6 left-overs | src/world/**, src/battle/**, sprite.c, imgfx.c, sprite_shading.c, entity_model.c, entity/*.c, item_entity.c, animator.c | CORE-1 to CORE-5; sprite.c after STALE2 and MDL-3 |
Merge order against WEND:
- Effect triangles. RECT-2 owns the draw side (pm_gpu_gtrip/gappend, ring, merge key) and makes it native under the walker now. FXW owns only the producer side and calls the same pm_gpu_gtrip from ND_RANGE records. FXW rows that touch triangle effects start after RECT-2b.
- Sprites. MDL-3 (shaded sprite two-unit, imgfx through the ring) lands before WBO-4. WBO-4 only removes the word headers.
- Rects and fills. RECT-1 lands before CORE-3. CORE-3 calls pm_gpu_fill_rect and pm_gpu_tex_rect as they are.
- Off-screen. OFF-1 lands before CORE-7 (ND_TARGET).
- Lit/fog models. PM5-2 lands before WBO converts lit world files.
- Moves out of pm_gbi.c. TRI-4 before CORE-6 (vtx/tri move); MDL-4 before CORE-8 (rec_run move).
- Deletion rows come last in each stream. S10 starts after all four streams and WEND END-1.

## Stream CORE
CORE-1 RECORD MODE. gen_nd_gbi.py emits a second leaf form. When pkt is the main list position (`(Gfx*)(pkt) == gMainGfxPos` at expansion), the leaf calls a typed setter (nd_set_om, nd_set_comb, nd_set_prim, ..., the same fields as nd_words) and writes no words. Any other pkt writes words as today. A file opts in with `-include nd_rec.h`.
- Host test: setter state equals nd_words state for every leaf.
- 120 lines. Gate: host test passes; mac_01 byte-equal with no file opted in.
CORE-2 NdRec KINDS. nd.h gets the kinds ND_STATE (index into a per-frame state pool, deduplicated by hash), ND_RANGE, ND_RECT, ND_FILL, ND_CALL, ND_MTX, ND_SEG, ND_TARGET, ND_GLYPH, ND_SPLASH, ND_MODEL, ND_SPRITE, ND_ENT. nd_emit runs them in order and LEGACY spans in between. The main list is cut at each record (nd_bridge_cut per span, in order).
- 100 lines. Gate: main_loop.c and cam_main.c opted in; mac_01 byte-equal; nd_emit order log equal to the walker op order (ndref).
CORE-3 RECT/FILL. ND_RECT and ND_FILL call pm_gpu_tex_rect and pm_gpu_fill_rect at emit, with the state from the pool.
- Gate: background.c and background_gfx.c opted in; walker ops of camera_render and the 2 background lists are 0; mac_00 sky right by eye.
CORE-4 RANGE. ND_RANGE: gSPVertex keeps the pointer (read at emit, like the walker does), triangles append indices; emit goes through pm_gpu_gtrip. Segments are resolved through ND_SEG at emit.
- Gate: walking_dust.c opted in; its ops 0; dust right by eye at mac_01.
CORE-5 TEX. Move the tile/tmem/load rules (tex_op, tb_inline, glyph_atlas_bound, load_shaded_sprite after MDL-3) from pm_gbi.c into nd_tex.c, after STALE2. pm_gbi.c calls the moved code. One group per row, about 80 lines.
- Gate: mac_01 byte-equal per row.
CORE-6 MTX and GEOMETRY. Move gbi_popmtx, mtx_cache, the G_MTX handling and gv_tri/vertex slots into nd_mtx.c and new nd_geom.c, after TRI-4.
- Gate: mac_01 byte-equal; the stereo pair (yard_stereo_l) right by eye.
CORE-7 CALL and TARGET.
- ND_CALL(ptr, segment snapshot) compiles through nd_dlc (ent_gdl and dlc_try moved into nd_dlc.c). A side-buffer or heap list not seen before compiles per frame (dynamic key).
- ND_TARGET(role): img_role moves to nd_target in nd_core.c, after OFF-1.
- Gate: entcheck_mac01 eq with diff 0; pause_spin_grab right by eye.
CORE-8 MODEL and ENT. rec_run, ent_draw and pm_gpu_spr_native take ND_MODEL, ND_ENT and ND_SPRITE structs. The G_PM_* word headers become struct records. Runs after MDL-4.
- Gate: mac_01 byte-equal; iwa_10 by eye.

## Stream UI
For each row: opt the files in to record mode (CMake), fix leaves that have no native form, and delete their G_PM_TEXI blocks.
- Gate per row: walker ops of the producer 0 in WCEN (CEN-1a line), picture right by eye at the named scenario.
UI-1: draw_box.c, windows.c, draw_img_util.c (fill rule: FILL_SX10 and the inclusive corner move into the ND_FILL decoder). Scenario: toad_house_menu.
UI-2: msg_draw.c and msg.c. The GLYPH G_PM_TEXI block becomes ND_GLYPH; the atlas is in nd_tex.c (from CORE-5). Scenarios: text_menu, super_text.
UI-3: screen_overlays.c, screen_render_util.c, curtains.c (ND_TARGET for the colour-image sites). Scenarios: intro_native, merlon_door.
UI-4: src/pause/*.c, src/filemenu/*.c. Scenarios: pause_spin, file_select, pause_spin_wide.
UI-5: state_title_screen.c, main_loop.c front UI, worker.c. Scenario: golden_multi (title).
UI-6: hud_element.c, status_icons.c, status_star_shimmer.c, inventory.c. Only after HUD1 has committed (check `git log -1 -- src/hud_element.c` on nds1); rebase on its scale-1.25 code. The two HT1 G_PM_TEXI blocks become ND_RECT with a texture state. Scenarios: ui1_choice, mapchange_hud, battle_wide.

## Stream FXW
Blocks are about 32 files each, in name order, A-Z.
- Each row: opt the block in; a file that fails gets a fix item (one file per item).
- Gate: the walker ops of the block's effects are 0 in nok11_splash, wcen_sbk56 and wcen_iwa10, plus one ndref_fx scene per block; pictures right by eye.
FXW-1: effects A-D. FXW-2: E-L (energy_orb_wave.c only after FX closes). FXW-3: M-R (radial_shimmer.c after FX). FXW-4: S-Z.
FXW-5: water_splash.c gPmSplashMark becomes ND_SPLASH; flame.c and underwater.c colour images become ND_TARGET; effects.c. Scenarios: nok11_splash, chapter_wide, bowser_intro_fire.

## Stream WBO
WBO-1 LIST. No code. Grep src/world and src/battle for custom gfx builders: `gMainGfxPos`, local `Gfx*` side buffers, gSPEndDisplayList, gDPSetColorImage. Write the site list to docs/handoff/agent_S7.md, one anchor (function plus grep string) per site.
WBO-2: src/world areas opted in by area (about 10 files per row). Side-buffer lists stay as words and run through ND_CALL. Scenarios: house_sign, toad_fountain, mac_01 crystal ball, flo_10, hos_05 intro.
WBO-3: src/battle. Scenarios: battle_start, battle_menu_bar, battle_danger.
WBO-4: sprite.c (SPR_RM_LIST and the NP1 TEXI to ND_SPRITE state), imgfx.c, sprite_shading.c. After STALE2 and MDL-3. Scenarios: battle_start, talk_off.
WBO-5: entity_model.c (delete the rm_first G_PM_RM rewrite; the render-mode list becomes ND_STATE), entity/*.c, Shadow.c TEXI, item_entity.c, animator.c. Scenarios: toad_gate, coin_kmr03, sam_11.
Gate for every row: walker ops of the producer 0; picture right by eye; entcheck diff 0 where entity code changes.

## S10 update
Precondition, all three must hold:
1. Census: walker ops = 0 for every producer in the 8 CEN scenes, plus the full catalog once.
2. Counter: nd_emit LEGACY spans = 0 in those runs, i.e. the main list holds no words except G_ENDDL.
3. Grep: every src file that uses a GBI macro is in the CMake record-mode list (`grep -rlE 'g[sS][DS]P' src` minus the CMake list = empty, apart from pure data files that only ND_CALL reaches).
S10-1 BRIDGE: delete the word-writing half of the record-mode leaves for the main-list pkt, nd_bridge_cut and the LEGACY kind. gbi_task calls nd_emit only (one call per eye).
- Gate: host build has no pm_gbi_run symbol (nm); mac_01 byte-equal.
S10-2 WALKER:
- Delete pm_gbi.c and its CMake line.
- Before that, check that every function it still has is moved (CORE-5 to CORE-8) or has no caller (WEND END-1 opcode census).
- In gbi_custom.h delete G_PM_RM, G_PM_TEXI, G_PM_REC/SPR/ENT word forms, gsSPPmTexInline, gSPPmRenderMode, gPmSplashMark, and the dead `#else` forms of SPR_RM_LIST and MODEL_RM_OP.
- Keep model.h pmRecVer unless the Model layout stays at 0xB0.
- In pm_gpu.c delete what lost its callers: the census hooks cen_add stay (D8), the ndref hook stays.
- Gate: build links; full catalog G2.
S10-3 HOT SPAN: 3dsx.ld.in CF4 span lists nd_emit, nd_native_submit and their callees in call order (the walker names are gone). nd_dlc.c and the cold nd files build at -Os.
- Gate: nd_size_test.sh (hot code 8 KB or less); map file check.
S10-4 FULL: full catalog once (epic merge), console prof push (prof, profstall). Record the numbers in docs/handoff/agent_S10.md.

## Decisions
- D1 Record format: typed NdRec structs (nd.h kinds above), filled at macro time by the record-mode leaves. Never N64 words in the main list. Pointers (vertices, matrices, lists) are kept and read at emit, as the walker reads them, so data written after the macro still draws right. State goes in a per-frame deduplicated pool, and records point to it.
- D2 Record mode applies only when pkt is the main-list position. Leaves that write into other buffers keep writing words. Those lists are reached by gSPDisplayList and compiled by nd_dlc (ND_CALL).
- D3 The decoder that stays: nd_dlc.c (compile static asset lists and per-frame side-buffer lists into native ranges) plus nd_state.c nd_words (state ops inside those lists). It is a compiler with a cache, not a per-frame walk of the main list. Rare world/battle custom lists are opted in mechanically; no hand rewrite. Their side buffers use this decoder.
- D4 pm_gbi.c is moved function by function into nd_tex.c, nd_mtx.c, nd_geom.c, nd_dlc.c, nd_core.c (target), with mac_01 byte-equal per move. Its CPU paths are deleted by WEND. Nothing in it is kept as a debug tool. ndref capture/compare and the census stay.
- D5 File ownership is the stream table; one owner per file at a time. pm_gbi.c functions are owned by WEND until that WEND row lands, then by CORE. pm_gpu.c stays with WEND (producers call its API, never edit it). nd_tmpl.c tev_op_* stays with FX; pm_tex.c and nd_tex.c stay with STALE2 until it closes, then CORE-5.
- D6 Effect triangles: RECT-2 owns the draw path, FXW only the producer records. Sprites: MDL-3 owns the draw, WBO-4 the producer.
- D7 hud_element.c and the status bar files are shared with the HUD1 worktree. UI-6 runs last, after HUD1 has committed, and only after reading HUD1's diff. Never edit these files in parallel.
- D8 No toggles. Record mode is per file, by CMake membership. Debug switches only in diagnosis rows. A file that cannot convert is a named exception in agent_S7.md with its words compiled through ND_CALL, never a runtime switch.
- D9 Byte-equal (mac_01) only for CORE rows and shared draw code. Producer rows are right by eye at the named scenario plus the walker-ops-0 gate.
- D10 Two tries per named cause, then hand back with the measurement.
