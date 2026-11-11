# Oracle handoff (2026-11-11 night; overwritten in place)

A replacement oracle starts here: read CLAUDE.md, orchestrator_rules.md, plan_hotcode.md (sections 2 and 5), then this file.

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
