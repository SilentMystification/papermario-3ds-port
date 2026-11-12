# Oracle handoff (2026-11-11 night; overwritten in place)

A replacement oracle starts here: read CLAUDE.md, orchestrator_rules.md, plan_hotcode.md (sections 2 and 5), then this file.

## fps60 2x speed (oracle 2, 2026-11-12; user fault on XL build 4492; first in the queue)
- Cause and fix: row_FPS60SPD.md (Haiku). The fps60 idle wait (pm_main.c:657-668) waits 1 vblank; SYNC60 removed the second vblank
  of SYNCDRAW; INTERP1 interp_heavy (pm_main.c:708) makes no-key-frame steps (ISKIP) common, so steps run 1 LCD period apart.
- Open, not read (machine restart): retrace_call(0) of the LG_IDLE pass (pm_nusys.c:236) to confirm it never runs step_game_loop
  (the D_80073E0A flip); audio tick count per step (pm_audio_step at pm_main.c:493 and pm_nusys.c:168: two call sites, check that one
  step runs only one tick set). Neither is needed for row FPS60SPD.
- Open read (FPS60SPD try 3): with iskipall, 17 key frames in 130 steps (drawn 147 against 130 iterations). LGF_ISKIP is set on every
  step (pm_main.c:708) and nuGfxTaskStart clears nkey at the swap (pm_nusys.c:182). gfxRetrace_Callback runs the bg task before the swap
  (src/main.c:138-139), so no post-swap task is known. Find what queues these key frames; a key frame of only part of a step would show
  as a flash. Related: INTERPB3 try 3 kfb census.
- INTERPB3 (Sonnet coder): pm_key_show must call pm_lg_key_walked once per K frame and do one pm_gpu_begin (sync60_pace), as
  pm_key_walk does (pm_nusys.c:208-219). Give this line to the coder at its next hand-back.

## NATIVEAPI chain to the end state (oracle 3, 2026-11-12; replaces oracle 2's chain section; edit in place)
Definition of done (user, 2026-11-12): the complete native-only path, with ALL display-list code removed. While any of these stays,
the work is not done:
- a list builder (the gDP*/gSP*/gs* macros, Gfx arrays, gMainGfxPos);
- a list reader (anything that decodes N64 list words or typed ops);
- the walker state model (other mode, combine decide, TMEM/tile model, matrix stack of lists).
Core 0 then runs only native records (NDC_NAPI with pso, tex and mesh ids). The N64 decision code (nd_comb decide,
nd_render_state, tex_win) runs only in the host converter at build time.

### Census at writing (main 529924217; LISTCEN makes it a bench)
- Builders in src/*.c: 4014 gDP/gSP calls. src/effects 1769, src/world 416, src/entity 169, src/battle 75, src/pause 72,
  src/filemenu 65, root files 1436. The largest root files: msg_draw 142, hud_element 131, model 125, imgfx 122, item_entity 114,
  entity_model 96, draw_box 83, screen_overlays 71, screen_render_util 70, background_gfx 70, state_title_screen 48, sprite 38,
  animator 36, msg 35, draw_img_util 26, cam_main 26, background 25, sprite_shading 23.
- gMainGfxPos: 4030 uses.
- Static list data: 853 asset files and 100 src files with `Gfx X[]`, about 8053 gs macros in assets (effects gfx, entity, map
  data, ui).
- Gfx in include/: 14 headers.
- Readers in 3ds/src:
  - the walker and typed-op runners: nd_walk.c, nd_set.c, nd_rec.c (NDC_STATE, nd_rec_state), nd_recs.c, nd_ent.c (ent_run,
    ENT_GBI), nd_dlc.c (static list compile), nd_shape.c (map shape lists at load), nd_model.c;
  - the state model: nd_comb.c, nd_tex_set.c, nd_rect.c, nd_geom.c (gv walker part), nd_mtx_set.c;
  - the recording side: 3ds/include/nd_gbi.h and nd_op.h (the macro-to-typed-op mapping).
- Direct executors (NAPI2B and on) still keep the old ops as a fallback (hud_rect_emit plus nd_walk_state). Those fallbacks are list
  code too.

### Stages and rows (id: scope | blocker | model)
Every row is gated by nd_host, the DIRECT byte set (spot_mac01_det, mdl5_iwa10_det, spot_mac00_det) or its named scenes, and the
LISTCEN deltas (the counts it removes). CONSOLE = one batch push, compared with a stored run.
- Running: GPUHANG1, S1FILL, NAPI3B, S4PDIR, S5SPR, S6T.
- Written and waiting:
  - S5PQUAD and S1TSTEN after S1FILL;
  - S5CHUD after NAPI3B (nd_api.h end), then S1PREV, then S2W (S2W also after FSELFIX);
  - S4SPLASH after S1PREV;
  - S4INST after S4PDIR;
  - S5EMDL and S5IMGFX after S6T.
- L0 LISTCEN (row written, Haiku, ready now): 3ds/tools/listcen.py and bench `listcen` (host_test). Builder, data and reader counts
  per file and per class go into the bench database. This is the measure of every later row and the final gate.
- S5 rest (one Sonnet row each, by LISTCEN order, after S5CHUD sets the quad helper). Each moves every gDP/gSP call of its files
  to native records and deletes that file's fallback ops:
  - S3MSG: msg.c (window, arrows, choice pointer, draw_digit and draw_number);
  - S5ITEM: item_entity.c;
  - S5BG: background_gfx.c and background.c;
  - S5TITLE: state_title_screen.c, draw_img_util.c and the logos;
  - S5SHADE: sprite_shading.c;
  - S5BTL: src/battle (75 calls);
  - S5MENU: src/pause (72) and src/filemenu (65); the off-screen passes wait for S6T design question 3.
- S6 (written: S6T). Then:
  - S6S: pure state ranges (cam_main, status_icons, gfx_init_state, setup lists) | S6T | Sonnet.
  - S6E: entities through compiled ranges (ent_run without ENT_GBI; SaveBlock) | S6T | Sonnet.
  - S6M: map custom gfx (src/world 416, crystal_ball_gfx) | S6T | Sonnet.
- S7: build-time conversion of static list data. This stage is new: without it, effects, entities and maps still run N64 list
  words at load or at draw.
  - S7CONV (Opus): a host converter for every static Gfx array of src and assets (as nd_shape_conv is for shapes).
    - It reads the gs macros and runs the N64 decision once on the host (nd_comb decide, nd_render_state and tex_win compiled
      for the host; nd_host already builds parts of them).
    - It emits C tables of native records: pso descriptors, NdBindDesc, vertex and index buffers, matrix and segment slots, and
      the draw order.
    - Design questions: segment-relative addresses (the 0x09 effect segments, the entity vertex segments); lists the game
      patches at run time (palette or vertex edits: they stay dynamic); lists called under run-time state (inherited other
      mode).
    - Gate: a host test that runs each converted list on the old walker and on the native tables and compares the draw words.
  - S7EFX: the 127 effect files. Their D_* gfx data goes through S7CONV; the appendGfx sites call nd_particles, nd_mesh or nd_quad
    with table ids | S7CONV, S4INST | Sonnet, about 4 rows by effect group.
  - S7ENT: entity data (src/entity 169, assets entity) | S7CONV, S6E | Sonnet.
  - S7MAP: map shapes. nd_shape.c converts the N64 shape lists at map load; that conversion moves to the host, and pm_map.c loads
    native meshes. Map custom gfx data too | S7CONV, S6M | Sonnet.
  - S7MDL: model.c (125 calls: the render-mode lists Gfx_RM*, custom gfx builders, the fog and tint paths). Models are native
    records already, but their setup still records lists | S7CONV | Opus (render-mode lists times the env tint paths; the
    largest single producer of state).
- S8: remove every fallback (Sonnet, one row per helper group). Delete the old-op fallback of each direct executor: hud_rect_emit,
  the msg fallback, the fill and quad fallbacks, nd_part_draw's nd_ops_run fallback, ent_run_seg_interp. Gate: first show the
  fallback census counters are 0 in every bench scene (LISTCEN plus one run of each scene), then delete the code.
- S9: deletion (Sonnet unless noted). Each row's gate is a grep proof (its LISTCEN class count 0), a full build, nd_host, the
  DIRECT byte set, battle_wheel_nowide_det, pause_tab_det_pic and the msgbox talk scene.
  - S9DEL1: per-frame nd_walk_state, nd_ops_run, ENT_GBI and the nd_dlc static list compile | S6E, S6M, S8.
  - S9DEL2: the run-time nd_comb decide, sync_combine, scm memo, sync_render, set_tilesize, tex_win and ensure_tile model,
    nd_rect and the gv walker state. The N64 decision code stays only in the host converter.
  - S9DEL3: NDC_STATE and nd_rec_state, the nd_op.h typed ops, the nd_gbi.h macro mapping, the nd_walk.c G_* arms and the nd_set.c
    op table.
  - S9DEL4: the Gfx type and gMainGfxPos in game code, the gbi.h includes in src and include, and the decomp gDP/gSP call sites
    left as `#else` (N64) branches: DELETED (decision 2026-11-12, best practice chosen for the user: the port stops building for N64;
    the tag pre-list-removal keeps the old tree).
  - S9GATE (Haiku): LISTCEN with every class at 0 (builders, data, readers), plus this grep returning nothing:
    `grep -rnE "\bg[DS]P[A-Za-z]|\bgs[DS]P[A-Za-z]|\bGfx\b|gMainGfxPos|NDO_|nd_ops_run|nd_walk_state" src include 3ds/src 3ds/include`.
    The full DIRECT byte set and the battle, pause, talk and fileselect scenes are byte-equal to the last S9 base. CONSOLE: prof push
    mac_01 0 0 200, plus pause, battle and file select, compared with the stored prof run, and PMU (hotset M1).
- CONSOLE batches: C1 after NAPI3B; C2 after S4 batch A plus S4INST (XL fxtest=all); C3 after S5 and S6; C4 after S7; C5 after
  S9GATE.
- Opus rows: S6T, S5IMGFX, S7CONV, S7MDL. All others are Sonnet or Haiku, chosen per row by oracle.md step 4.
- Order:
  1. S5 and S6 first (they need no data conversion);
  2. S7CONV;
  3. S7EFX, S7ENT, S7MAP and S7MDL in parallel (different file sets);
  4. S8;
  5. S9DEL1 to S9DEL4 in sequence;
  6. S9GATE.
- Recovery: the orchestrator tags main `pre-list-removal` before S9DEL1 starts (row_S9DEL1.md step 0): `git checkout pre-list-removal` restores the last tree with list code.
- No user decision is open in the chain. ASSETFREE stays parked: S7 converts list data into native tables
  inside the ELF, and ASSETFREE (assets out of the ELF) is separate.

## File select and pause flip (oracle 2, 2026-11-12; user fault: entry slow, flip on a file pick very laggy)
- Path (code read): ROTSCALE windows (filemenu_common.c:648, :669; 11 windows: filemenu_main.c:1108-1118, filemenu_createfile.c:589-599;
  entry appear: windows.c:130-134; pause flip: pause_main.c:59-62; message box msg_draw.c:200) take draw_box.c:432-830: per window per
  frame an off-screen contents pass (draw_box.c:770-786; pm_gpu_off_begin pm_gpu.c:2022: flush, a 512 x 256 slot clear :2089 though
  the contents begin with a fill of the window area draw_box.c:777, C3D_FrameDrawOn, rebuild_3d; off_end :2383) and height/6 strips
  (draw_box.c:801-820: per strip a load of the slot, a tile size, a matrix multiply, one draw). 3 full slots (pm_gpu.c:2059-2068).
  Loads of the slot bind the render target texture (pm_tex.c offt), no conversion and no CPU read. Save reads are RAM copies
  (pm_os.c:656), not the entry cost; the entry runs the same appear flip.
- Shared with the pause menu: yes (same draw_box ROTSCALE path for the pause window flip and every appear/disappear window).
- Fix candidates after FSELCEN counts: (a) one quad per window instead of height/6 strips (the strips are an N64 TMEM limit; one
  plane, so the same picture up to rounding: rounding rule); (b) clear only the window area, or no clear when the fill covers it;
  (c) fewer target switches. The GPU part (clears, fills) needs the console: one push with the file select after the fix.

## NATIVEAPI (oracle 2, 2026-11-11; plan_nativeapi.md has no section 6: the S2 executor design is here)
- Rows: NAPI1 (skeleton: NDC_NAPI with the core 0 runner as word 1, nd_rec_napi, pso table + host test), then NAPI2A (HUD rects) and
  NAPI3A (glyphs) in parallel. Form "replay record": core 1 keeps the game arithmetic and records the emit inputs; core 0 runs the same
  gbi macros into a local list (nd_gx_w writes the typed unit for a packet that is not gMainGfxPos) and runs nd_walk_state. Byte-equal
  by construction, no GPU state change; the walker state model still runs on core 0 (adds the emit code to core 0: NAPI2A reports sizes).
- S2 read (main 6e171b39a), for the direct executor (S2b/S3b): nd_texrect (nd_rect.c:219) for a HUD rect: rect_to_image returns 0
  (tile tmem 0), off_screen_check, box_fold_sync no-op (box_prod 0), sync_render, rtile 0 (no TEXEL1, w1 tile 0), gp3_t0_surv,
  ensure_tile(0), dual_bind no-op (plan has no tex1), s/t from tile_x0/tile_fx/tile_shift[0] (HUD: shift 0, fx 0), copy span only in
  COPY cycle, t1_fold returns when the aux unit is not off-screen, zprim and fold arms not taken. The window comes from tex_win0
  (nd_tex_set.c:137, pure on NdTst/NdTexM: the HUD masks 3/4 can cut h or w to the mask period), the bind from set_tilesize
  (:184: glyph cell path for CI4, tsz_last skip) and ensure_tile_slow (:374: tile_bound, glyph_atlas_bound, flush + pm_tex_set_wrap +
  pm_tex_load, tile_x0 = x0 - ox - gox). pm_gpu_tex_rect (pm_gpu_rect.c:267) reads gs.combine (class 4-8 kept, else 2), gs.env_col
  (class 5, 7), gs.plan.vtx (class 8), pm_tex_cur_idx, pm_tex_cell_bound, nb.rect_z/fill_z, gs.vp_*, gs.off_active, pm_wide.
- Direct executor rule: a direct draw does not update the walker tile and combine fields; it must call ens_clear(), set cur_tile -1,
  sc_ok 0 and nd_sr_forget() so the next walker draw rebinds and resyncs. Byte-equal then holds only if no walker producer reads state a
  direct producer left (verified by the byte gate per stage, not by construction). Its pso part: nd_comb_decide (nd_comb.c:507, dry, on
  an NdCombIn) and nd_render_state (nd_tmpl.c:292, pure) give the decision without the walker. A direct executor changes flushes and
  binds: console prof gate (lesson 1). Written after NAPI2A and NAPI3A merge and one console prof push (mac_01 0 0 200 and a talk spot).
- I-cache note: until S6 deletes the walker model, each direct executor adds code to core 0; the gain arrives at S6.
- Interpolation: one walk (design, oracle 2). Today an fps60 step walks the stream twice (pm_nusys.c:164-166 the t = 0.5 pass with
  pm_interp_find per matrix load, nd_mtx_set.c:92; pm_key_walk :191-200 the key frame): the whole state model, the batch closes and the
  emits run twice; only the matrices differ by intent. Options: (A) GPU-side: upload two matrices per palette row and lerp in the
  shader with a t uniform, replay the recorded GX entries twice as the stereo right eye does (sr_replay, pm_gpu.c:2111): uniform space
  doubles (pm5 is near the 96-register limit) and a lerp of MVP rows is not the product of lerped matrices: rejected. (B) one walk,
  two command streams: the matrix stack keeps a twin (cur and lerp: load_mtx loads both, pm_interp_find for the twin), each gm_tab
  entry gets its twin row, and each emit (nd_raw_native, nd_native_submit, blk_put) writes its words to K and a copy to L with the twin
  palette rows and L's ring and target addresses; L is presented first, then K at the next 59.83 Hz slot. Cost: the walk once plus a
  word copy and the twin products; CPU clip, cull and refusal decisions use K's matrices (a triangle off-screen in K is missing in L
  at the screen edge for one frame). Off-screen passes and grabs follow the stereo address map (sr_map). Premise row: INTERP2M (L vs K
  command words by class: uniform, address, target, other); option B rows (twin stack; twin emit; present order) after it, Sonnet
  (user approved).
- Stereo with design B: the right eye is already no second walk: the walk records the GX queue entries of the left pass (sr_on,
  sr_frame_begin pm_gpu.c:2049) and sr_replay (:2111) queues them again before C3D_FrameEnd with the right target bind list (bind_main
  :2066-2089: targetR, eye uniform -eye_e) and fill and copy addresses mapped to targetR (sr_map :2091). Per eye only the eye uniform
  (u_eye, one register in the bind list) and the render target differ; the vertex ring, textures and command lists are shared. So one
  walk can give all four pictures: B writes the K and L streams, and each stream's frame end runs the stereo replay for its right eye.
  Cost per step: 1 walk + 1 command copy (L) + 2 replays (as today per frame). Adds to INTERP2M: nothing new to measure for the eyes
  (the replay already shows eye independence); the L stream's GX entries must be recorded for its own replay (the B present-order row
  keeps sr_on per stream). Rows: the B rows carry the stereo case in their gates (spot_mac01_fps60_det with stereo forced, both eyes).

## Rows (state at writing)
- Running (6 coders): HC6B (Opus), WALLSHADE, UB1, HCCUT2 (step after the first try written in its row), HC9B, CRASHEXIT.
- Ready (refill 2): HCCUT4 (nd_comb.c decide/dual_bind/sync_combine_work, pm_comb.c, nd_model.c mdl0 notes). Waits: HCCUT5 (nd_tev_build; after HCCUT2), HC10 (hot order regenerate; after HCCUT2-5).
- Merged tonight: WALLSHADE, CRASHEXIT, HC9B, SPARKLE, UB1. Running: HC6B, HUDGM, HUDRING, HCCUT2.
- Waiting: KEYMISS (after HC6B), BATUI (deleted when HUDGM and HUDRING merge and the user's wheel check passes), PAUSEDR and AUDDROP
  (3-line notes: console data), STEREOFX (crash.txt), COLDBOOT (the XL cold-boot log).

## Waiting on the user (park; ask in the morning)
1. Console checks after merges: hos_01 walls (WALLSHADE); battle wheel circles (HUDGM + HUDRING); sparkle lag in the hos_01 spirit
   pop-in (SPARKLE); crash exit to hbmenu with a `crashtest=game` push (CRASHEXIT); prof push mac_01 0 0 200 for HC9B (pmu_d), HCCUT2
   and HC6B (pmu_i, cycles, lgms).
2. PAUSEDR: is the pause menu still laggy, and does the top screen still not go grey? (a prof push in the pause gives the values).
3. AUDDROP: are sounds still missing? (a push with audiostat in normal play gives the counts).
4. BATUI: decided 2026-11-11 (wide layout): row BATUI widens the two battle description boxes; the wheel keeps WIDE3/WIDE4.
6. KEYMISS: decided 2026-11-11 (yes): rows KEYMISS, KEYFIX1, KEYFIX2 written.
5. STEREOFX: the ESCAPE crash.txt from the XL (3D slider hang). COLDBOOT: the XL cold-boot log.

## Open designs for the next oracle (user decision: Haiku rows with exact edits; console fxtest costs in build3ds/wt/FXCEN3/build3ds/out/fxcen_o3ds.txt)
- Per-particle draws (snowfall +14.1 ms for 120 draws, embers +10.5, energy_shockwave +7.2, effect_3D, pink_sparkles, shimmer_burst):
  each particle is gSPMatrix + gSPDisplayList of a static effect list (snowfall.c:207-218). walk_call (nd_walk.c:221-243) sends a
  non-map list to ent_gdl (nd_ent.c:332) -> ent_run (:389): per call ENT_GBI replays the list's state words and ent_seg_native makes one
  native draw per segment (about 115 us per draw on the O3DS by the table). The ring path (nd_ops_run -> nd_geom_vtx -> gv_tri) keeps
  a matrix id per vertex and batches up to 8 matrices per draw. Read ent_run's per-call cost first (state replay, flush, palette).
- Water refraction (water_splash +12.3 ms, water_block +11.3): SPLASHPREV reads the previous frame (water_splash.c:210); a SETTIMG of a
  non-current frame buffer calls pm_cfb_capture_prev (nd_tex_set.c:47, pm_main.c:347): a CPU copy of the PICA colour buffer to RGBA5551,
  then one LoadMultiTile window per drop. Design a GPU-side copy (render-to-texture of the last frame) and bind it as unit 0.
- Not read: fire_breath beyond TILEWIN, spiky_white_aura (+3.9 ms, 8 draws), bulb_glow (45 tex calls), firework_rocket (2 texmiss).
- Reading notes (old oracle, stopped near 600K): ent_run (nd_ent.c:389-412) per call: ENT_GBI of the list's leading state words (:403),
  then per segment ent_seg_native (one native draw) or ent_run_seg_interp; then the trailing state words; dlc_dirty = 1. The static
  vertex buffer of a compiled list has no per-instance matrix index, so N calls cannot merge into one native draw without a vertex copy;
  the ring path (gv) can (matrix id per vertex, up to 8 per batch). Which part of the ~115 us per call costs (state replay, texture
  bind, palette product, submit, flush) is not known by code read. The prof line splits it per class (pf_report cls line: ent, sub,
  kick, tex, comb, geo, mtx, ms per sampled frame): one console push per effect gives it; fxtest takes a repeated index, so 8 slots
  of one effect fill one 300-frame window. Pushes (O3DS XL, prof profile): `warp=mac_01:0 pos=0:0:200 profdisk=1 fxtest=63,63,63,63,63,63,63,63`
  (snowfall), the same with 105 (embers), 89 (water_splash), 84 (water_block). Design the batching and the refraction rows from those
  class splits: ent/sub/kick large -> route small effect lists to the ring path (walk_call :231 condition); tex/fbread large for
  water -> GPU copy of the previous frame instead of pm_cfb_capture_prev.
- Group 2/3 result (old oracle): rows PBATCH1 (particle lists 4+ calls per frame -> ring path) and SPLASHPREV2 (water_splash reads
  the buffer the GPU copy is made for). The GPU previous-frame copy already exists (PT12 pm_gpu_grab_prev, grab_impl prev_frame,
  queued at pm_gpu_begin pm_gpu.c:2526); the CPU read (pm_gpu_read_frame pm_main.c:380) is only its fallback: first frame of a request
  run, a T2 grab earlier in the frame (grab_n > 0 overwrites grab_tex), no VRAM slot (off_fail), BTRANS skip. Open: a separate texture
  for the previous-frame copy so a T2 grab in the same frame does not force the CPU read (callers: every set_timg of a non-current cfb:
  pause picture pause_cfb_src background_gfx.c:263, game over blur, screen_render_util.c:44, state_pause.c:110, file select :189).
  water_block reads no frame buffer: its +11.3 ms (39 draws, 59 texture calls) needs the console class split.

## Data path audit SD -> RAM -> CPU/GPU (old oracle, code read; ASSETFREE stays parked: it moves assets out of the ELF, not these hops)
| hop | where | per frame? | status |
| shape .nds read on the game thread + wasted Yay0 decode of the shape | pm_map.c:58-94, :177-190; pm_prefetch.c:38-47 | map change | row MAPROMFS |
| hit file Yay0 + byte swap | pm_map.c:194 swap_hit_file | map change, once | host pre-convert possible (like shapes): later, low gain |
| R13 sync units in the record payload | nd_rec.c:214 | every record | row R13NOP (also a correctness fault) |
| texture window reload on a scroll (tile origin or width moves the key) | nd_tex_set.c tex_win0/1 | every frame | rows TILEWIN (merge), TILEWIN2 |
| per-particle static list = one native draw + state resync | nd_ent.c:389-412, pm_gpu.c:3033 | every frame | row PBATCH1 |
| refraction source buffer without a GPU copy (CPU-side texture from RAM) | water_splash.c:210 | every frame | row SPLASHPREV2 |
| model cache key misses: prim colour w12 (kzn_08), panner tile sizes w18-21 (kzn_08, hos_01) | pm_gpu.c nd_mkey_a; nd_model.c:42-44 uvso | every frame on those maps | design read open (patch uvso and TEV colours on hit) |
| static model matrices fixed -> float | nd_mtx_set.c:106-125 | cached by slot and version (mtx_cache) | fine |
| moving model matrices fixed -> float | load_mtx | every frame, needed (16 conversions per matrix) | fine |
| static vertices | dlc/ent compile at first use into chunk 0 (linear) | once | fine (16-byte N64 Vtx layout read by the GPU as is) |
| dynamic vertices (effects, sprites, HUD) copied to the ring | nd_geom_vtx -> gv_tri | every frame, needed | fine |
| cached model draws: command block copy | pm_gpu.c:1102 blk_put | every hit | fine (memcpy of the block, patches) |
| previous-frame copy | pm_gpu_grab_prev (GPU, PT12) | when asked | CPU fallback pm_main.c:380 only on the first frame or after a T2 grab: separate texture open |
Open reads for this audit: sprite raster decode per animation frame (spr_load_sprite Yay0 path, pm_tex sprite memo spc_miss), the
data-cache flush ranges (pm_gpu.c:2943 svcFlushProcessDataCache per range, pm_tex.c:554 per span), resident bytes per class (no census
yet: a memcheck-like class split of heap use would give it).

- PBATCH2 (design step, old oracle): pink_sparkles calls 8 lists (D_E01248A0[unk_28 & 7], pink_sparkles.c:283-286) with a
  gDPSetPrimColor per part: on the ring path a prim change is a state change, so every part still flushes a batch (44 draws). A batch
  needs the prim colour per vertex (the shade x prim product folded into the copied vertex colour when the combine reads PRIMITIVE only
  as a multiplier); needs a read of sync_combine's plan for this effect's combine before a row.
- Still to read for the data-path rows: HITHOST (hit file format and its readers: swap_hit_file pm_map.c, collision code), PALGEN
  (every writer of a palette in RAM: tattle lerp, set_palette, sprite palettes, imgfx), reloc pre-apply (waits on MAPROMFS: pm_map.c),
  DPCEN2 (resident bytes per pool), texture pre-conversion study (sizes per area texture archive: assets/us/mapfs/tex/*.json).

- Console PMU (fxcen_pmu_o3ds.txt, 2026-11-11, before PMUPHFIX): core 0 CPI 4.70, I-miss 46 / 1000 inst, D-read-miss 19, bmiss 9.6,
  istall 41.8 % and dstall 24.7 % of about 5.3M cycles per clean frame (20 ms at 268 MHz); core 1 similar (3.1M per step). For
  plan_hotcode: instruction fetch is the largest single stall (M1-hot 70.1 KB against a 16 KB I-cache); the next HC cuts need the
  per-phase split (PMUPHFIX, then the same console push) to pick the class with the most I-miss; until then no new HC row by code read.
- fire_breath (+17.4 ms, 7 draws, 204 tris, texmiss 0.05 after TILEWIN): fire_breath.c:191-223 draws a few large scaled puffs, each a
  2-cycle 2-texture translucent mesh (tile 0 and 1 scroll by imgFrame, :210-211): the cost is GPU fill of big alpha quads, not CPU. The
  ph line's gpu phase on the console confirms it; a fix changes the look (smaller puffs or one texture): a user decision, no row.

- DPCEN1 numbers (for plan_datapath.md section 1/3, per 300 frames): steady mac_01 upkb 6 hashkb 1 flkb 1069 texmiss 13 ndraw 33300
  nidx 1240200 gvtri 34800; kzn_08 upkb 7 hashkb 1 flkb 865 texmiss 15; map load window (f300) mac_01 upkb 634 hashkb 147 flkb 3058
  texmiss 109, kzn_08 upkb 176 hashkb 50 flkb 1326 texmiss 57. So steady state moves few texture bytes; the map load carries uploads and
  hashes: the map-load loop cases (seam table, texture decode, list compile) rank first for the one-time generation. The cache flush is
  the largest steady byte count (about 3.5 KB per frame). cmd is 0: C_CMD is counted only by the nd_cen census (nd_cen.c:491), not a fault
  of DPCEN1; a GPUCMD words counter (the command buffer offset at the frame end) is a small follow-up row if wanted.

## Effects cost model (oracle 2, 2026-11-11; XL run xl3 = main 6e171b39a with FILLA0R: build3ds/out/fxcen_xl3.txt; xl2 agrees)
Values: ms per frame above base (the "us" column: frame period of the logic step, pm_fxtest.c:200; every spawned effect shows a 0.55-0.6
ms floor). Counts per frame from fxcenb. Bound = oracle code read only; FXSPLIT (gpu, walk us per effect, two console runs) decides it.
| effect | ms | per-frame work (fxcenb) | structure (code read) | bound by read | cut without a look change |
| fire_breath | 17.1 | 204 tri, 7 draw | ~51 puffs: a 200x200-unit 4-triangle fan scaled by data->scale (fire_breath.c:214-223), 2-cycle TEXEL1/TEXEL0 lerp (I4 128x32, tiles 0 and 1 scroll), ZB_CLD_SURF2 blend | GPU fill | none: the N64 draws the same quads (inherent) |
| snowfall | 12.8 | 940 tri, 2728 vtx, 9.7 draw, gvrej 536 | 117 parts x (gSPMatrix load, gSPMatrix mul, list D_09000D50: 24 vtx, 8 tiny tris) (snowfall.c:207-218); 57 % of tris trivially rejected after the per-vertex clip work | core 0 per part (2 matrix conversions, 24-vertex load with clip bits, 8 gv_tri); GPU work is small by read (8 tiny tris per part, about 0.2 ms of vertex work for 2728 vertices) | instanced particle path (plan S4 nd_particles: list vertices once, per-instance float matrix) |
| water_splash | 11.3 | 50 draw, 50 tex, 66 tri | one previous-frame window bind per drop (LoadMultiTile) | core 0 bind per drop + GPU | one bind of the previous-frame texture, window as a uv offset (TILEWIN2 rule) |
| water_block | 9.4 | 38 draw, 36 tex, 66 tri | per component list + matrix + texture (water_block.c:280-285) | core 0 per draw | as water_splash |
| energy_shockwave | 6.0 | 200 tri, 336 vtx, 16 draw | one scaled ring mesh, 2 scrolling tiles (SetTileSize per frame), translucent | mixed | after FXSPLIT |
| pink_sparkles | 6.0 | 47 draw, 100 tri | prim colour per part breaks the ring batch | core 0 per part (draw count alone is cheap: PBATCH1 cut snowfall draws 117 -> 9.7 for 14.1 -> 12.8 ms) | after FXSPLIT; PBATCH2 branch kept, not merged |
| embers | 4.5 | 21 draw, 16 tex, 127 tri | SetTileSize per particle (atlas cell, embers.c:217-218) | core 0 per particle | PBATCH3 (cell offset in the copied vertex st) |
| spiky_white_aura | 3.6 | 4 draw, 15 tri | large translucent quads scaled per part | GPU fill | none (inherent) |
| underwater | 3.2 | 423 tri, 678 vtx, 2 draw | full-screen distortion mesh rebuilt each frame (core 1) | mixed | after FXSPLIT |
| shimmer_burst | 2.9 | 16 draw, 154 tri | per part matrix + prim + list D_E0070CD0[i & 7] | core 0 per part | instanced path with per-instance colour |
- XL run xl4a with FXSPLIT (fxcen_xl4a.txt; run 2 pending): only fire_breath is GPU-bound (gpu 17.9 ms, walk 4.6). Core 0 walk above
  base: snowfall 18.1, water_splash 12.9, energy_shockwave 11.2, pink_sparkles 11.1, water_block 10.8, embers 9.2, shimmer_burst 8.7,
  effect_3D 8.4, explosion 6.8, snowman_doll 6.5, radiating_energy_orb 6.0, snaking_static 5.6. That is 150-700 us per part or
  draw, and about 2 ms for a 4-triangle effect (ring_blast): far above the per-op code estimate, so the per-part path is dominated by
  code fetched per op (I-miss 46 / 1000 inst, CPI 4.7), not by its arithmetic. PBFLUSH (run 504363): pink_sparkles flushes from
  pm_gpu_set_plan 53.8 per frame (base 11.4): a new plan per part (prim baked into the plan colours, pm_comb_colours).
- S4 order (native particle path: one pso and one tex per effect, static list vertices once, per-instance float matrix, colour and uv
  offset): 1 snowfall (117 parts: matrix load + mul, 24 vtx, 8 tri each), 2 pink_sparkles and 3 shimmer_burst (prim per part: plan per
  part), 4 embers (tile size per particle: uv offset per instance), 5 energy_shockwave (11 live instances, each its own setup list),
  6 effect_3D, 7 water_splash and water_block (one previous-frame bind with the window as a uv offset instead of a bind per drop; a
  nd_quad case, not particles), then explosion, snowman_doll, radiating_energy_orb, snaking_static. fire_breath is not S4 (GPU fill).
- S4 rows (user: run 1 is enough): S4P (nd_parts builder, nd_parts_run, nd_part_draw) + S4SNOW (caller, fxpic scenes) in one
  worktree; then S4PINK, S4SHIM, S4EMB in parallel. Byte gate fxpic_<idx>; console gate fxtest of the effect, base and row, two runs
  each. FXFNCOV (build3ds/out/fxfncov_classes.txt, calls per frame minus base): the per-part growth is class F (snowfall +7015,
  energy_shockwave +4617, shimmer +3596), with A, C, G, I growing with the part count: what S4P removes. pink_sparkles and water_splash
  grow class D (+5306, +3644) and E: a flush per part (pink: new plan per prim; splash: a bind per drop). Colour step: row S4COL (PRIM
  gets the plan's vertex slot when no SHADE: pm_comb.c src_of gave it a stage constant, so PBATCH2's routing never ran). Next: the
  water uv-offset bind (nd_quad), energy_shockwave (list with two vertex loads: nd_part_draw for multi-load lists), embers uv step.
- Culling of off-screen particles is not a cut: user code rule "no culling the game does not do" (feedback_code-rules.md).
- Cut rows are written only after FXSPLIT and its two XL runs agree (lesson 2 below); then one row per cut with the console gate
  (fxtest of that effect, its ms before and after, two runs each).

## N3DS QTM RAM (oracle 2, feasibility only, unverified; for plan_datapath.md CIA memory; do not start)
- libctru os.h: OS_QTMRAM_VADDR 0x1E800000, OS_QTMRAM_PADDR 0x1F000000, OS_QTMRAM_SIZE 4 MiB, the last 128 KiB kernel-reserved:
  about 3.9 MiB, New 3DS only. Claim (user's source, not checked: 3dbrew refused our fetch): a CIA maps it through a static memory
  mapping descriptor in its ExHeader ARM11 kernel capabilities; a .3dsx runs under the launcher's ExHeader and cannot get it.
- Could hold: data the GPU or CPU reads every frame with no allocator churn: hot native textures (a fixed-slot pool: map textures
  of the current area are 220-340 KB native each, so all of one area fits), the command and vertex rings (pm_gpu ring chunks), the
  frame matrices (gm_tab, PM_GM_N x 64 B). Whether the GPU can read QTM RAM (it is FCRAM-like memory outside the linear heap: the
  GPU reads by physical address, so likely yes; must be probed) decides textures and rings versus CPU-only data.
- Conflicts: the QTM module (3D camera head tracking) owns the region while it runs: the slider 3D with head tracking (super-stable
  3D) needs QTM stopped or not started (a probe must check the service state first); no libctru allocator (a fixed carve-up of our
  own); the ExHeader descriptor needs our own RSF/makerom CIA build, which the repo does not have yet (no RSF, no makerom step); the
  applet .3dsx (the user's current push path) cannot use it; O3DS has no such region, so every use needs an O3DS path anyway.
- Worth: low next to ASSETFREE (the 25.7 MB resident ROM image leaves the heap) and the N3DS CIA SystemModeExt 124 MB (about +60 MB
  on N3DS with no new code path). 3.9 MiB is one extra pool on one model behind a new build type. Do after a CIA build exists and
  only if a measured N3DS memory limit remains. Probe row: row_QTMPROBE.md (user 2026-11-11: planned work, parked with the
  ASSETFREE epic and the CIA build row; the user's probe plan: standalone probe.c, probe.rsf, qtmprobe.cia).

## Texture pre-conversion study, CIA memory (old oracle, for plan_datapath.md section 2 and 5)
- Memory today (pm_mem.c:72-110 __system_allocateHeaps): the budget is already read at run time: commit = svcGetResourceLimit
  (RESLIMIT_COMMIT) of the process, capped by PM_FCRAM (64 MB); the whole commit is one MEMOP_ALLOC_LINEAR block split into the 14 MB
  linear pool (PM_OURS) and the heap (the rest); the ROM image (25.7 MB, rom_remap.h PM_ROM_PACKED) must fit the heap (pm_os.c
  pm_rom_init). O3DS XL boot line: region 65536 KB commit 65536 KB (Homebrew Launcher in applet or title mode: the APPLICATION region of
  the host title). VRAM is 6 MB on every model (texture slab, pm_tex.c :578 vramAlloc).
- A CIA title gets the APPLICATION region of its system mode (exheader ARM11 system capabilities; makerom RSF AccessControlInfo
  `SystemMode` and `SystemModeExt`): O3DS 64 MB (Prod, the retail default; 72/80/96 MB dev modes exist, the system reboots into the mode
  the title asks for: not a retail-safe choice), N3DS `SystemModeExt: 124MB` (Prod extended) or 178 MB (Dev1), plus N3DS `CpuSpeed: 804MHz`
  and `EnableL2Cache: true`. So on an O3DS a CIA gains no memory over HBL title mode; on an N3DS a CIA with 124 MB about doubles the commit.
- Run-time choice, no build flag: keep pm_mem.c reading the commit; replace the fixed PM_FCRAM cap by the commit itself and size the
  linear pool and the ROM keep-set from it (a larger commit keeps more ROM ranges resident or caches more native assets); memcheck.py
  keeps one device entry per target (o3ds_xl applet, n3ds_cia) with its measured commit, as today.
- Map texture sizes (assets/us/mapfs/tex/*_tex.json, main images only): source 1.7 MB for all areas, native (palette applied, 16-bit or
  8-bit PICA forms) 4.5 MB, plus power-of-two padding (about +30 %): about 6 MB for every area; per area 220-340 KB (kmr 340, flo 278,
  kpa 275, jan 243, kkj 231, mac 221). The census map load uploads 634 KB (mac_01, map textures and sprites in view). So native map
  textures are cheap to keep resident even in the applet budget: RGBA5551 with the palette applied for CI with 1-bit alpha cutouts,
  RGBA4 for CI with graded alpha, L8/LA4/LA8 for I and IA (native today), ETC1 only for opaque textures without edges (not needed for
  size). Blocker: palettes the game edits at run time (tattle lerp, actor palette swaps): those textures stay CI and convert at bind.
  Sprites (player, NPC, battle) are the larger texture set: their size is not counted yet (DPCEN2 or a sprite archive scan).
- ASSETFREE effect: with native files on SD (generated once from the user's ROM), the ROM image need not stay resident: the generator
  reads it once; the runtime reads native files. That frees most of the 25.7 MB on every model, more than a CIA on an O3DS gives. The
  pre-converted textures are loaded by one read and one upload (no decode, no Morton pass, no palette hash at bind).

- The first XL run (fxcen_xl.txt, build 4324, "worst case 2 ms") was an outlier: effects did not draw (negative draw counts). Void.
- Phase sum of I-misses (68.6K) above the clean-frame mean (47.3K): not a fault. pmxph counts sampled frames only (1 in 30), which run
  the phase timers and two PMU svc reads per push and pop: their own code adds I-misses (the prof line's oh field shows the sampled
  frame cost). The shares hold; the absolute numbers are of a sampled frame.
- HC next: row HCM3 (census only) gives M1-hot and M3 per class with call counts on main; the oracle then writes the cuts for the gbi
  phase (classes A, D, E, F, G first by size: plan_hotcode.md section 0 table) from those numbers.

## Data-path rows status (non-ASSETFREE queue, old oracle)
| item | row | status / reason |
| palette hash per CI bind | PALGEN | ready (memo per walked frame, byte-equal) |
| resident bytes per pool, sprite decode and list compile per load | DPCEN2 | ready (after DPCEN1 merge) |
| hit file swap | - | no row: a few KB swapped once per map load (pm_map.c:261-300, under 0.2 ms), its Yay0 decode is already on core 1 (prefetch slot 1); a second file format does not pay |
| pre-applied shape relocs | - | no row: the reloc pass runs on the prefetch thread (core 1) since MAPROMFS, a few thousand adds; pre-applying needs the link address in gen_shapes (it runs before the link) |
| build-time texture pre-conversion | design step | needs DPCEN2's map-load numbers and a host port of tx1_build (pm_tex.c :1540-1600) keyed by the source hash shash0 and phash; then rows |
| static list compile per map load | after DPCEN2 | DPCEN2 times it (compiles, compus); a row only if it is large |
| sprite archive decode per load, audio predecode | after DPCEN2 | DPCEN2 times sprite loads; audio needs its own census (pm_dspaud.c predecode) |
| phase names | PHNAME | ready (identifiers and texts only, log keys kept) |
Start order: PALGEN, PHNAME and DPCEN2 (after the DPCEN1 merge) start together (pm_tex.c / pm_port.h+pm_phase.c+pm_nusys.c+pm_wdog.c /
pm_gpu.c dp_frame+sprite.c+nd_shape.c+nd_ent.c: disjoint). HC cut rows after HCM3.
- Native draw API note (for the user, from plan_hotcode 5.7): no N64 display list word is decoded at run time (HC4e4); static lists are
  typed at build time (nd_shape_conv, gs macros). What remains is the RDP/RSP state model on core 0: other-mode bits, N64 combine words
  and the combine decision (nd_comb, pm_comb), tile descriptors and the TMEM load model (nd_tex_set set_tilesize, tex_win0/1), the
  vertex slots and matrix stack, the render mode table: about classes A, B, E, F, G of plan_hotcode (70 KB hot). A native API (the game's
  draw code calls nd_draw_mesh / nd_draw_sprite / nd_draw_quad with a resolved texture, PICA template, float matrix and colours, no
  recorded stream) would remove that model from core 0 (the largest I-cache cut) and the stream walk. Cost: every draw call site changes
  (128 effects' appendGfx, hud_element, msg_draw, draw_box, sprite, model, entity, imgfx, background, battle UI: about 15-30k lines),
  each needs its state resolved (a build-time table per static list, run-time for dynamic lists), and each needs its own byte gate.
  Blocks: the size and the gates; models (records), sprites (NDC_SPR) and entities (NDC_ENT) already take native paths, so the gain is in
  effects, HUD and text. A staged path: one native API per high-traffic producer (HUD elements, message text, the top effects).

## Open items decision (old oracle, 2026-11-11)
| item | decision |
| PBATCH2 | step 2 written: pm_gpu_set_prim flushes on a prim change in combine 8 (pm_gpu.c:3820-3825); vertex reuse guard |
| PBATCH3 embers (SetTileSize per particle) | NO now: 1.2 ms in one effect after PBATCH1; the tile origin goes to uvso per particle (a gv param change), folding it into vertex st needs the bound texture to cover every cell (tex_win0 binds the window for clamp tiles): a tile-model change for under 1.2 ms |
| build-time texture pre-conversion | NO now: about 20 ms of decode per map load by estimate (mac_01 634 KB uploaded, 109 misses; tx1_build about 15 cycles per texel), under the map fade; needs a host port of tx1_build, a romfs key scheme and the CI-palette exceptions: with ASSETFREE |
| static list compile per map load | DO AFTER DPCEN2: its `compus` per map load decides (row if above 10 ms) |
| sprite raster decode per load | DO AFTER DPCEN2: `sprus` per map or battle load decides |
| audio predecode | NO now: no lag reported at voice starts; battle songs are predecoded on core 1 (BTLWIPE F4); AUDDROP (missing sounds) is the open audio fault |
| HC rows for gbi | DO AFTER HCM3 (M3 per class and calls) |
| effects above 1 ms | DO AFTER FXSPLIT and its two XL runs (Effects cost model section) |

- Lessons (corrected 2026-11-11, oracle 2): (1) an Azahar byte compare proves the picture only; a row that changes GPU state needs a
  console measure. (2) No GPU-cost claim from a single console run: confirm with a second run before acting. The FILLA0 claim (+3 to
  +17 ms per translucent effect) was wrong: it came from one outlier XL run (fxcen_xl.txt, build 4324: effects did not draw, negative
  draw counts); xl2 (with FILLA0) and xl3 (FILLA0R) give the same costs. (3) After two failed tries on one cause, a measurement row.

- DPCEN2 decisions (Azahar per map load: sprite loads 74-138 ms, list compiles 4.6-8.4 ms, texture upload 634 KB / hash 147 KB):
  sprite load -> YAY0FAST (faster Yay0 run copies, same output; host equality test over the ROM asset table); a decoded-sprite cache
  across map loads is the next step if YAY0FAST is not enough (heap free is about 3 MB: sizing needed); host pre-decode to romfs is
  worse (an SD read of the decoded size is slower than decoding from the resident ROM). List compile: NO (under 10 ms). Texture first
  bind: the source hash (pm_tex.c:1225 src_hash, bytewise FNV, 147 KB per load: about 3 ms) is a small word-wise step; the decode and
  tiling stay for ASSETFREE (user deferral).

## Morning list (rows to write next, by the oracle)
1. Class cuts after HC6B: read M1-hot per class (bench hotset_size on main after the HC6B merge) and the M3 census (hotset.py --fncov on
   fncov_mac01_det/kmr10_det logs of the HC6B lane); cut rare arms of the top functions per class as HCCUT2 did (byte-equal, cold
   attribute or a cold body behind the inline guard). Candidates read so far: tex_raw_piece (pm_gpu_rect.c:220, base form arm),
   sync_render orbdbg block (nd_set.c:233-243), gv_matrix_id first-use malloc (nd_mtx_set.c:55-60).
2. HC9 rest: NdNatC 64 B hot head, NdShadow compares by generation ids, mc_keyw/mc_tab probes in one line: all in HC6B files
   (nd_model.c, nd_emit.c): write after HC6B merges. NdWs tile tables native: needs a design read of nd_ws.tst1 users.
3. KEYMISS (user yes, 2026-11-11): KEYMISS and KEYFIX1 merged; KEYFIX2 deleted (no word 23 count). Census f600: mac_01 and sbk_56
   miss 0; kzn_08 6246 misses / 300 frames carried by w12 (prim) and w18-21 (main panner); hos_01 1439 by w20/21 (aux panner). The
   census counts every full-key word: words 33-91 except 53/54 (36 prim, 37, 49, 57+ gvkey, 68-75) are not compared by the cache
   (key B = 28-32, 53, 54) and do not cause misses. Next: design reads for w12 and w18-21 (not Haiku-ready yet). Members: w8/w11 fog colour and position (shroud lerp model.c:2071-2083, depth
   tint :1577: zmix and the F08 mix in the var uniform, not patched by blk_put pm_gpu.c:1102), w12/w13 prim/env (TEV stage constants
   and plan constants, nd_tmpl.c:184-276; blk_put patches col[] only from the stored draw), w18-w21 panner tile sizes (uvso in the var
   uniform, nd_model.c:42-44; the tile 0 window and WATER B1 invariant: tex_win0 nd_tex_set.c:202 reads uls/ult), w22 pre list (never
   looked up, nd_recs.c:315; M3 content key), key B words 28-32 (walker order). Census word index: 0-27 record (R0 w0 .. R13 w1),
   28+ pm_gpu_model_key s[] (pm_gpu.c:3187-3194).
4. HC7b: closed (row_HC7.md deleted): box_pat, td[] and the tile state exist only on core 0 (nd_comb.c:175-187, the walker texture model); HC5's census (T_SYNC 99.3 %) shows core 1 cannot know them.
5. Rows from new console evidence (COLDBOOT, STEREOFX) when the user sends it.
6. UB1 leftovers: suspect 1 is void (DOORHITCH: an entry with a unit 1 slot never hits, pm_gpu_model_hit e->h1s); suspects 2, 3, 5, 6 need an oracle read.
7. HC9 rest (NdNatC 64 B hot head, NdShadow generation ids, mc_keyw/mc_tab one-line probes): files free since HC6B merged; needs an oracle design read of nd_model.c (cache layout) and nd_emit.c (shadow) before rows.

## Facts learned this session
- Shooting Star Summit is area_hos (hos_00 Path, hos_01 Summit); area_sam is Shiver Mountain.
- Since MDL-5e (856d74716) a model the native path refuses is not drawn at all (nd_recs.c:191); since TRI-4 (c3e7afb19) a triangle the
  ring path refuses is dropped (nd_geom.c:291-293): every refusal arm is a "missing" fault.
- DEADCODE premise void: --gc-sections already drops unreferenced functions (a map + call-graph + data-word listing finds 2).
- hot_order.ld names must follow GCC clones: load_impl became load_impl.constprop.0 after KZN04 TEV (HCCUT2 step).

## Decisions made (user, through the orchestrator)
- HC5/HC6/HC6B: Opus. KZN04: Sonnet, non-byte-equal shading accepted. SPARKLE: Sonnet, non-byte-equal sparkles accepted.
- Battle circles: fix every path, widescreen or not (no narrowing questions). Night of 2026-11-11: ask the user nothing; park here.
- Byte gates: spot_mac01_det, mdl5_iwa10_det, spot_mac00_det; battle_start_f182_cold is not a byte gate (cold runs differ).
