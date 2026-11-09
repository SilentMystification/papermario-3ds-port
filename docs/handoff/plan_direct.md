# plan_direct: the game calls the native producers directly (no display lists)

Epic DIRECT (row_DIRECT.md). One Opus agent, worktree build3ds/wt/DIRECT, base main e703884fe. Rules: CLAUDE.md 5, 7, 8, 9, 11; hw_guidelines.md.
Tags: [C file:line] read in source for this plan; [M source] measured; [U] not checked.

## 0. Where the frame time goes now
- O3DS XL, mac_01 at 0 0 200, prof (hw_live_log_131.txt, f989): walk 20.57 ms per frame; split `cls`: rec 5.41 (model records), sub 3.14 (native submits), rect 2.29, comb 1.94, tex 1.80, ent 1.84, raw 1.53, list 1.04, mtx 0.68, spr 0.42, geo 0.25 [M]. Logic thread (core 1, main 72c60c53c): lgms 18.3 ms per step, of which bw = fw 11.8 ms is the wait for the core 0 walk; logic work about 6.5 ms. Core 0 5.29M cycles (about 19.7 ms) per frame, 54.2k I-cache misses, 30.0 fps: core 0 is the limit [M orchestrator, console run of 72c60c53c].
- The list decode and state part (tex + comb + geo + mtx + list) is about 5.7 ms of the 20.6 ms; the rest is the native draw work (records, entity lists, rects, submits) [M].
- Core 0 hot code about 23 KB against the 16 KB L1I (agent_s10.md, hot span 23,268 B); most I-cache misses are the walk evicting itself (row_CORE2C.md) [M].
- The seam (row_CORE2C.md): producers on one core, emit and submit on the other. Today core 1 builds N64 words and core 0 decodes them again and keeps the whole N64 state machine (nd_ws, td[], combine sync, g2_same, tex_op) in its hot set.

## 1. End state
- The logic thread (core 1) runs the game and records native commands into the frame stream (NdCmd, section 2). No dynamic display list is built: gMainGfxPos is not written by game code.
- The walker side (core 0) replays the stream: one loop over typed commands, each calling the native setter or the native draw (nd_model_draw, the sprite draw, ent_run, the rect draw, nd_emit_draw). No N64 word is decoded on core 0 per frame.
- Static asset lists (map shapes, entity and effect lists in ROM data, render mode lists) are compiled once into native objects at first use (nd_dlc_compile, ent_compile: these exist). A compile is a load-time decode, not a per-frame walk.
- Deleted: nd_walk_run (the pc loop), g1_skip, g2_same, the G_DL stack, the in-list records G_PM_REC, G_PM_SPR, G_PM_ENT, G_PM_ND, the per-frame word handlers (tex_op and the other op handlers become field setters), the DisplayContext mainGfx and backgroundGfx arrays (1 MB static on the 3DS build, include/common_structs.h:2261).

## 2. The frame stream (NdCmd)
Format (3ds/include/nd_cmd.h, new): a command is one 8-byte header plus 0 or more 8-byte payload units, 8-byte aligned, read in order.
- Header word 0: op << 24 | units << 16 | arg (16 bits). Header word 1: arg2 (a pointer or a value).
- Ops: NDC_END, NDC_JUMP (next chunk), NDC_LIST (bridge only: walk a span of the task list, word 1 = start, payload 1 unit = byte count), NDC_CALL (replay a stream range again: floor reflection), NDC_MODEL (payload: the model record and its R13 words, the old G_PM_REC layout, so rec_run reads it unchanged), NDC_SPR, NDC_ENT, and the typed state and draw ops of phase B (section 5).
- Storage: per display context (2), a chain of 32 KB chunks from the heap (malloc at first use, kept and reused; no per-frame malloc, hw rule 7). A chunk that has no room for the next command ends with NDC_JUMP to the next chunk of the chain (allocated once, at the high-water mark). The chain of a context is reset when that context's frame begins (the bg task build), so it lives exactly as long as the list memory of that context (the N64 two-context rule; depth-1 push in nd_wq.c keeps it safe).
- PmTask (pm_port.h) carries the first command of the task's stream instead of the list pointer. PmFrame stays 32 bytes (nd_wq.c static assert).
- Replay rule (plan_s10_path D8 kept): the replay reads the stream and writes nothing into it. The interp key frame replays the same stream. Stereo walks once: the right eye replays the GX queue entries of the left pass (command lists, fills, copies; pm_gpu.c sr_replay), the eye shift is a vertex shader uniform.

Thread rule:
- Recorder (logic thread, or the game thread in serial mode): nd_rec_* functions write only the stream of the context being built. They read game memory only.
- Replay (walker side, core 0, inside pm_frame_walk): reads the stream and the memory its pointers name, owns nd_ws, pm_gpu, pm_tex and the native caches (plan_core2.md Ownership).
- A pointer in a payload names memory under the same rule as a list pointer today: the display context (matrices, vertex copies, viewport copies) or static data. The c2haz census checks payload pointers as it checks record words (nd_c2_rec).

## 3. Bridge (LIST spans) and its deletion
- Until every producer of a task records natively, the task stream interleaves NDC_LIST spans of the words that unconverted code wrote. Each nd_rec_* call first cuts: if gMainGfxPos moved since the last cut, it appends NDC_LIST {cut, gMainGfxPos - cut} and sets cut = gMainGfxPos. nuGfxTaskStart closes the stream with the last span and NDC_END.
- The first span of a task starts at the task's list base (patched at close; the recorder does not know the base before the task starts).
- A span walk continues the walker's sync state (nd_walk_ops, walk_keep: no DIP2 full sync at a span start), so cutting a list into spans changes no draw and no GPU word.
- Rule: a cut never falls inside a region that a branch skips (effects that reserve inline vertex data, build_custom_gfx). Producers that run inside such a region keep their in-list form until their builder is converted (nd_rec_sub > 0; section 7 D7).
- Deleted in D9 (f05be775d): NDC_LIST, the span cutting and the task list (mainGfx, backgroundGfx). gMainGfxPos at the top level names nd_rec_top; a word that no redirect records is logged (`ndc: list words outside the stream`) and dropped. D10a: the custom gfx lists are stream ranges (build_custom_gfx records them between nd_rec_mark and NDC_END; R11 names them; nd_range_run). D10c: nd_walk_run is deleted; the lists that remain (render mode lists, refused G_DL targets, the fallback words) run on the op executor nd_walk_state (agent_direct.md STATE).

## 4. Producer API per content class
| Class | Producer today (file: function) | Native call (record side) | Replay (walker side) |
|---|---|---|---|
| Models | src/model.c appendGfx_model (G_PM_REC at :1537, R13 words at :1642, values at :2195) | nd_rec_model(rec, aux) [D1]; the record layout stays (D1 of plan_s10_path, the word is the record) | rec_run (nd_recs.c) on the payload |
| Floor reflection | src/model.c execute_render_tasks, nd_range_begin/end/repeat (:5079) | nd_rec_range_begin/end, nd_rec_call(range, flip, proj) [D1] | NDC_CALL replays the range |
| Sprites, NPCs, player | src/sprite.c spr_appendGfx_component (G_PM_SPR at :410), spr_appendGfx_component_flat | nd_rec_spr(rec) at the final vertex point [D2] | pm_gpu_spr_native; battle context and imgfx fall back to the payload words (named fallback, D2) |
| Entities, items, animators | src/entity_model.c appendGfx_entity_model (:353, :503), src/item_entity.c (:1014, :1052, :1230, :1262), src/animator.c (:896) | nd_rec_ent(rec) [D3] | nd_ent_case, ent_draw |
| HUD, rects, boxes, windows | src/hud_element.c, src/draw_box.c, src/windows.c, src/draw_img_util.c, src/status_icons.c | nd_fill_rect, nd_tex_rect plus the state calls of section 5 [D4] | nd_fillrect, nd_texrect setters (nd_rect.c) |
| Messages | src/msg_draw.c (msg_draw_char, prim rects, frames), src/msg.c | as HUD, plus nd_glyph(sheet, pal, index) [D4] | the glyph atlas draw |
| Background | src/background_gfx.c gfx_init_state, gfx_draw_background; src/background.c | nd_bg_strip and the state calls [D5] | the strip rects |
| Fog, shroud | model records (R3 flags 2, 32, 64; model.c), gfx_init_state, the battle shroud (background tint, BTLDIM) | fields of nd_rec_model and nd_fog(min, max, colour) [D1, D5] | rec_fog, set_fogcolor setters |
| Effects | src/effects/*.c (127 files), src/effects.c render_effects_scene/UI, src/effect_utils.c | nd_gbi.h redirect (section 5) per file group; static effect lists stay compiled (ent_gdl) [D6] | typed state and draw ops |
| World, battle UI, custom gfx | src/world/**, src/battle/** (btl menus, action command HUD), build_custom_gfx (model.c :4275) | redirect; custom gfx lists become stream ranges (R11 pre and post) [D7] | NDC_CALL of a range |
| Frame state, overlays, pause, file menu | src/main_loop.c, cam_main.c, screen_overlays.c, curtains.c, pause/*.c, filemenu/*.c | redirect; nd_target(role) for colour and depth image ops [D8] | the off-screen and capture setters (nd_target.c) |

## 5. Phase B: the record-side state (nd_gbi.h)
- Done (D4b-3, D5): 3ds/include/nd_gbi.h, included by include/common.h, redefines the base gbi macros (gSPSetOtherMode, gDPSetCombineLERP, gDPSetColor, gDPSetPrimColor, gDPNoParam, gImmp1, gSPGeometryMode, gSPTexture, gSetImage, the scissors, the fill rectangles, gDPLoadTileGeneric, gDPSetTile, gDPLoadBlock, gDPLoadTLUTCmd, gDma0p/1p/2p, gSPVertex, gSP1Triangle, gSP2Triangles, the texture rectangles). Each evaluates its packet as gbi.h does (once per word), so compound macros and local lists keep their behaviour; a compound macro redirect that evaluates the packet once is wrong for a local list (it was the D4b-1 NDC_TEX4 form, deleted in D5).
- nd_gx_w (nd_rec.c): a packet equal to gMainGfxPos - 1 with nd_rec_sub == 0 is taken back and the op appended to the open NDC_STATE run (one 8-byte unit per op; any other record, a span or a range mark closes the run); a texture rectangle (its op and two RDPHALF words) becomes NDC_TEXRECT; the RDP syncs are dropped (g1_skip). G_ENDDL, the branch form of G_DL, local lists and nd_rec_sub > 0 keep the word. A file that defines ND_GBI_WORDS before its includes keeps words (named fallback).
- Replay nd_walk_state (nd_walk.c): per op the walker rules: g2_same/g2_note, pm_gbi_cur_op, gv_epoch (not for vertex, triangle, matrix and list ops), the debug prologue, the same handler as the walker arm; a push G_DL goes to ent_gdl, dlc_try, else nd_walk_ops to its G_ENDDL (walk_call). The ops stay N64 words in the stream: the finding after D3 holds (the texture ops are a stateful model with GPU calls inside the op; a decode on core 1 saves only bit shifts). Typed fields and dedupe on core 1 come after D9, when the walker no longer needs the words.
- List position tests: msg_draw.c:348 and filemenu_msg.c (char state reset) compare gMainGfxPos and the stream position (nd_rec_pos): a producer that records only commands does not move gMainGfxPos.
- Audit (done for D5): pointer use of gMainGfxPos other than `gMainGfxPos++` (`gMainGfxPos[`, `gMainGfxPos -`, `= gMainGfxPos`, `savedGfxPos`, `gSPBranchList`). Inline vertex data is stream data (nd_rec_data) and an effect list for later is a stream range (NdSub, D6c); the entity setupGfx functions write through gMainGfxPos++ (D7); Block.c-style local lists keep words by the packet test (walked through their G_DL). ND_GBI_WORDS is used by no file.

## 6. Interp, det, census, stereo
- Interp (60 fps, CORE2I): key_list holds PmTask values; a task is a stream head, so the key replay walks the stream. Matrix replacement (pm_interp_find on the matrix pointer, nd_mtx.c:42) is unchanged: the payload keeps the same Mtx pointers. FI_OPEN marks are game side.
- Det: the serial order is kept (record, push, replay at once). Byte gates compare shots.
- Census (pm_ph_cen, WCEN, MDL census): the replay calls the cold walk_dbg_rec hook for NDC_MODEL, NDC_SPR, NDC_ENT as the walker does for the in-list records; a LIST span of the main task is a main-list walk for pmx_main.
- c2haz (CORE2-2): nd_c2_rec scans payload pointer words.
- Stereo: one walk; eye 2 replays the GPU entries (pm_gpu.c sr_replay), not the stream. CPU reads of the target (pm_gpu_read_frame, pm_gpu_depth_at) see the left eye.

## 7. Slices (one class per slice; each: record, check byte-equal against the old path, switch, delete the old path; one commit or more per slice)
| Slice | Content | Deletes | Premise check (before code) | Scenes for the byte gate |
|---|---|---|---|---|
| D0 STREAM | nd_cmd.h, nd_rec.c (recorder), the replay loop (nd_stream_run), PmTask carries the stream, nuGfxTaskStart closes it, key and eye replay | the list pointer in PmTask and key_list (the walker is reached only through NDC_LIST) | a span walk with walk_keep gives the same words as one walk: read nd_walk_run (sc_ok/sr_ok, gv_streak_flush, tex_streak_flush at a span end) | spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015 |
| D1 MODEL | appendGfx_model records NDC_MODEL; floor reflection as NDC_CALL ranges; models inside build_custom_gfx keep G_PM_REC (nd_rec_sub, named fallback until D7) | G_PM_REC in the main list, the model.c branch-slot range trick | records are only written by appendGfx_model (grep G_PM_REC: model.c only) [C]; mdl_draw_hidden_panel_surface (model.c:4914) runs inside build_custom_gfx [C] | spot_mac01_det, mdl5_iwa10_det, tour10_det, a pra_01 reflection det shot (new scenario) |
| D2 SPRITE | spr_appendGfx_component records NDC_SPR at the final vertex point with the vertex and triangle words in the payload (fallback when the native sprite draw refuses: battle context, imgfx) | G_PM_SPR, spr_at/spr_rec in nd_walk.c | count the refusals per scene (spr census) | spot_mac01_det, battle_start_f182_cold, nok11_det |
| D3 ENTITY | entity_model.c, item_entity.c, animator.c record NDC_ENT; the fallback words go into the payload span | G_PM_ENT in the list, the nd_walk.c G_PM_ENT arm | nd_ent_case reads only the record and the words after it [C nd_ent.c] | spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold |
| D4a TEXSTATE | the texture tile, TMEM and TLUT model of nd_tex_set.c (tex_op, NdTst, ldrec, blk, tlut_pal, ld_valid, m1) into one state struct passed by pointer; the walker passes nd_ws (no logic change) | the file-static texture state | the walker reads the state only through the struct (grep of the statics) | spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det |
| D4b RECT | nd_gbi.h with the state ops plus the rect ops; file group messages (msg_draw.c glyphs), HUD, boxes, windows; the record side runs the D4a model and records resolved descriptors | the rect and state word path for those files | grep audit (section 5) of the group | spot_mac01_det, a text box det scene, pause_spin_grab_det_w300 (layout-sensitive, row_UB1.md) |
| D5 BG | background_gfx.c, background.c, gfx_init_state, cam_main.c render_frame | as D4 | BTLDIM merged (background.c owner) | spot_mac01_det, battle_start_f182_cold |
| D6 FX | src/effects in 5 groups by size, effects.c, effect_utils.c; G_PM_ND splash as a typed op | G_PM_ND | the inline-data effects (squirt, stop_watch, waterfall, water_block, whirlwind, chomp_drop) keep words or get a bracket | nok11_det (splash), battle_start_f182_cold |
| D7 WORLD | src/world, src/battle; build_custom_gfx lists as stream ranges (R11 pre and post become range ids) | the D1 fallback, nd_r11_walk list parse | list the custom gfx builders (agent_S7.md, 57 sites) | tour10_det, battle_start_f182_cold |
| D8 FRAME | main_loop.c, main.c, nugfxinit.c, screen overlays, curtains, pause, file menu, the colour and depth image roles (nd_target) | the last NDC_LIST producers | ndc list count 0 in the catalog scenes | menu_open_det, pause_spin_grab_det_w300, gameover_fps60 |
| D9 DELETE | nd_walk_run, g1_skip, g2_same, the walker G_DL stack, NDC_LIST, mainGfx and backgroundGfx (static size drops) | the walker | the full catalog once, with the user's approval (rule 5) | full catalog, console prof list |
| D10 WALKER | custom gfx ranges (D10a), flat op ranges on nd_ops_run (D10b), nd_walk_run deleted (D10c); open: load-time compile of static lists, the fallback words, g2_same | nd_rec_sub, nd_walk_run, g1_skip, the G_DL stack | the list kinds that reach the executor (agent_direct.md STATE) | the gate set of agent_direct.md |
| D11 LISTS | D11a: static state lists recorded in line (nd_rec.c rec_inline, G_PM_LST); open: the model record render mode list (rec_dl), the NDC_ENT and NDC_SPR fallback words, typed ops | the G_DL to a static state list at replay | lstcen census (lstcen_mac00_det, lstcen_btl_det) | the gate set of agent_direct.md |

Order rationale: D0 to D3 move the classes that already have producer records (the largest core 0 class, rec 5.4 ms, and ent, spr); they change structure, not cost. D4 is the first slice that moves decode to core 1 (rect 2.3 ms plus the state ops of the HUD and text files). D5 to D8 widen the redirect to the rest. D9 removes the walker and its hot code.

Gates per slice (row_DIRECT.md): nd_host PASS; byte-equal shots against build-at base main on the scenes above; one picture of the subject; memcheck OK. After each group (D0-D1, D2-D3, D4, D5-D6, D7-D8): the console prof bench list for the user (spot_mac01_prof, spot_mac00_prof, spot_sbk56_prof, starfx_hos01_prof). Speed is decided only by those console runs (Azahar: direction only). Prof line field per slice: `ndc` counts (commands per type and LIST spans per frame) so the console run shows the move.

## 8. Memory (O3DS XL margin 330 KB, hw_guidelines memory section)
- No new static buffer above 1 KB. The stream chunks come from the heap (malloc, 32 KB each, reused). Expected use per context: D1 about 140 records x 128 B = 18 KB (mac_01 [U]); the full stream at D8 about the size of today's used list (prof `st`/`vb` lines) [U]. The high-water mark prints in the ndc line.
- The heap reserve (3458 KB leave) covers two contexts of 4 chunks (256 KB) [U; checked with the boot `mem boot` line and the ndc high-water mark].
- D9 removes mainGfx and backgroundGfx from DisplayContext (2 x 0x10630 bytes): the static size drops by about 1 MB and the margin grows by the same.

## 9. Hardware rules (hw_guidelines.md)
- The replay loop is one switch by op over a sequential stream: PLD ahead every 32 bytes (rule 3), no division (rule 4), table dispatch (rule 5). Cold paths (census, c2haz, overflow) are cold, noinline (rule 1).
- Records are copied into the stream in order, whole words (rule 3); one cache line per header plus payload.
- Phase B moves the decoders off core 0: the core 0 hot set loses nd_walk_run, g2_same, tex_op decode and the othermode decode (rule 1, 10). Each phase B slice reports the hot span size from the map.
- Core 1 budget: logic work about 6.5 ms per step at mac_01 (O3DS; the rest of lgms 18.3 is the wait for core 0), so about 26 ms of a 33.3 ms step is free for the recorder; each phase B slice reports lgms and the core2 wait line on the console list.

## 10. Risks
- A cut inside a branched-over region walks data as words: the nd_rec_sub bracket and the section 5 audit prevent it; the close check logs a span outside the task list (cold, once).
- Record address memo (nd_mc_find_rec, keyed by the record address): stream addresses move as list addresses did; the memo verifies the key words, so a move costs a probe, not a wrong draw [C nd_model.c, checked in D1].
- Parallel rows: BTLDIM (background.c, pm_tex.c), CORE2I (pm_nusys.c key list, nd_wq.c, pm_logic.c), KEYMISS (model keys). D0 touches pm_nusys.c and pm_port.h PmTask; merge main before D0 and after each slice.
- The epic is larger than one session: the report names the last finished slice and the next one; no slice is left half done on the branch (rule 7: untested code stays on the branch, each slice is its own commit).
