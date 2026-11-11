# Oracle handoff (2026-11-11 night; overwritten in place)

A replacement oracle starts here: read CLAUDE.md, orchestrator_rules.md, plan_hotcode.md (sections 2 and 5), then this file.

## Rows (state at writing)
- Running (6 coders): HC6B (Opus), WALLSHADE, UB1, HCCUT2 (step after the first try written in its row), HC9B, CRASHEXIT.
- Ready (refill 2026-11-11 night): M3CEN (census, no source edit), HCCUT3 (pm_gpu_gtrip edge copy, nd_set.c sync_render orbdbg).
- Merged tonight: WALLSHADE, CRASHEXIT, HC9B, SPARKLE, UB1. Running: HC6B, HUDGM, HUDRING, HCCUT2.
- Waiting: KEYMISS (after HC6B), BATUI (deleted when HUDGM and HUDRING merge and the user's wheel check passes), PAUSEDR and AUDDROP
  (3-line notes: console data), STEREOFX (crash.txt), COLDBOOT (the XL cold-boot log).

## Waiting on the user (park; ask in the morning)
1. Console checks after merges: hos_01 walls (WALLSHADE); battle wheel circles (HUDGM + HUDRING); sparkle lag in the hos_01 spirit
   pop-in (SPARKLE); crash exit to hbmenu with a `crashtest=game` push (CRASHEXIT); prof push mac_01 0 0 200 for HC9B (pmu_d), HCCUT2
   and HC6B (pmu_i, cycles, lgms).
2. PAUSEDR: is the pause menu still laggy, and does the top screen still not go grey? (a prof push in the pause gives the values).
3. AUDDROP: are sounds still missing? (a push with audiostat in normal play gives the counts).
4. BATUI wide layout: should the battle wheel touch the left screen edge in Wide mode (now 54 px from it, the 4:3 distance)? A look change.
6. KEYMISS: start the Opus audit (census of per-frame key values, then fixes M1/M2/M4) after HC6B merges? (Opus needs a user yes.)
5. STEREOFX: the ESCAPE crash.txt from the XL (3D slider hang). COLDBOOT: the XL cold-boot log.

## Morning list (rows to write next, by the oracle)
1. Class cuts after HC6B: read M1-hot per class (bench hotset_size on main after the HC6B merge) and the M3 census (hotset.py --fncov on
   fncov_mac01_det/kmr10_det logs of the HC6B lane); cut rare arms of the top functions per class as HCCUT2 did (byte-equal, cold
   attribute or a cold body behind the inline guard). Candidates read so far: tex_raw_piece (pm_gpu_rect.c:220, base form arm),
   sync_render orbdbg block (nd_set.c:233-243), gv_matrix_id first-use malloc (nd_mtx_set.c:55-60).
2. HC9 rest: NdNatC 64 B hot head, NdShadow compares by generation ids, mc_keyw/mc_tab probes in one line: all in HC6B files
   (nd_model.c, nd_emit.c): write after HC6B merges. NdWs tile tables native: needs a design read of nd_ws.tst1 users.
3. KEYMISS rewrite on the HC6 key code (key A on core 1, key B on core 0).
4. HC7b (NDC_TEXRECT resolved fields): after HC6B step 5 shows whether box_pat is known on core 1.
5. Rows from new console evidence (COLDBOOT, STEREOFX) when the user sends it.
6. UB1 leftovers (suspect 1 unit 1 refresh on a hit: pm_gpu_model_hit, HC6B's place; suspects 2, 3, 5, 6) after HC6B.

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
