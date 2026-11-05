# agent_nds_s1e: S1 closed (S1-3 to S1-6 done), S2 not started (2026-11-04)

Branch nds1. Commits: c0875db9c (S1-3), 8bc56f1cc (S1-4 and S1-5), 9ee4ad45b (S1-4 lights), a857ddeb2 (S1-6). Host tests: `sh 3ds/tools/nd_host_test.sh` passes (nd_mtx, nd_core, nd_tmpl, nd_tex, seam step 2). ARM build passes. Pictures viewed and correct: battle_start, text_menu, yard_stereo_l, merlon_ball60, yard_shot.

## What changed
- ndref is format v3 (3ds/tests/ndref/FORMAT.md). Kind 4 (one per GPU draw call) holds the citro3d context words C3Di_EffectBind and C3Di_TexEnvBind turn into register writes, the other mode words and the plan key. Kind 5 holds post and game matrices of each palette product. All 8 files are re-captured. They are the oracle of nd_tmpl_test, nd_seam_test, nd_tex_test.
- nd_tmpl.c: key (44 bytes, no padding) holds the TevKey fields, flags, pass (0 main, 1 and 2 decal passes, 3 zmark pass 2), decal bias, w0, w1, two, t1only. Template = 64 (reg, value) words: effect block, stencil, early depth, color operation, blend, 6 TEV stages, buffer words. nd_tev_build is the TEV code moved from flush_tev_rebuild and pm_gpu.c calls it (no second copy). nd_render_state is sync_render's othermode rules moved, sync_render calls it.
- nd_mtx.c: nd_mtx_unpack, nd_mtx_load, nd_mtx_pop, nd_mtx_combine (eye shift), nd_mtx_cmul (libcitro3d Mtx_Multiply order, verified by disassembly), nd_mtx_pcomp (pal_mul and rebuild_3d call it), nd_mtx_replace (fps60 lookup), nd_lights. The walker's mv stack, projection, unpack and upload_mvp math are gone from pm_gbi.c.
- nd_core.c: NdRec has the full 32-byte layout of plan 2.2 (LEGACY: aux = list, aux2 = bytes). nd_order, nd_eye_copy added. pm_native.c, pm_native.h, nat_host_test.*, nat_layout_check.py, the fixture are deleted.
- nd_tex.c: descriptor table (128 blocks), wrap rules and param word math. pm_tex.c wrap_of and the final wrap use them. nd_tex_get loads through pm_tex_load on a miss.

## Deviations from the plan text (decided in the code)
- Matrix stack depth is 10 (RSP and walker value), not 16. A 16 deep stack would change the PT70 cap behaviour.
- The TEV template key holds w0, w1, two, t1only and calls pm_comb_build (the key has no PmPlan). pm_gpu_plan_key passes the pm_comb_build inputs next to pm_gpu_set_plan. Stage colour words that a colour source sets (PRIM, ENV, plan constants) hold 0 in a template (per-draw uniform). A stage colour no source sets holds 0xFFFFFFFF.
- COLOR_OPERATION (reg 0x100) and reg 0x130 are constants (0x00E40100, 0x80003C00). Nothing else writes them.
- Early depth words are constants (regs 0x61 = 1, 0x62 = 0, 0x118 = 0, 0x6A = 0).
- Stencil op word is stale in the old path while the stencil test is off. The host test compares it only when the test is on.
- nd_tex_get patches the filter and wrap into the param word (the old path sets them at the bind, after the load).

## Not covered by an oracle
- Lights (nd_lights): no ndref scene holds a lit draw (13 scenes probed: the 8 files and kzn_19, isk_04, tik_03, mim_05, sbk_00). Known-answer host test only.
- zmark pass 3 (pass 3 of nd_tmpl): no captured draw uses it. Words follow the old path code by reading.
- nd_tex data address word (word 3): not derived. S3 reads it at emit.

## Next: S2 (NDFRAME) from docs/handoff/plan_native_slices.md
S2-1 nd_gbi.h (macro redirect with bridge dual-write), S2-2 pass state calls and force-include, S2-3 N64-same check. The host test for S2-2 compares native background draws with the ndref stream (nd_stream_cmp). That needs a host emit of NdRec draws into a decoded stream. The S1 pieces to build on: NdRec, nd_tmpl words (the template words are the register writes), nd_tex blocks, nd_mtx pool, nd_order.

## Console (user)
S1 changes no drawing behaviour. If a console check is wanted: PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.131 -t 150 prof

## Addendum 2026-11-04 (late): S2-1, S2-2 force include, S3-1 done
- 47cd86791: nd_gbi.h bridge form. 3ds/tools/gen_nd_gbi.sh (docker gcc -E -dM plus gen_nd_gbi.py) writes 3ds/include/nd_gbi_leaf.h: 68 leaf macros get a block form that writes the words into a temporary list, calls nd_words (nd_state.c fills nd_st), then writes the real packet. Compound and alias macros are untouched. 6 leaves are `_Static_assert` (ucode, sprite 2D, lines). Force-included (CMake) for main_loop.c, cam_main.c, background_gfx.c, background.c. Host test nd_gbi_test (57 words equal to plain gbi.h). Azahar: scenario ndst_yard (switch ndst logs nd_st every 100 frames), picture equal to yard_shot.
- 696b5fa3f: nd_dlc.c compiles a static list into NdObj (groups, boxes, Vtx copies flag 0, indices, geometry ops). Host test on mac_01 and mac_02: 262 lists, 388 groups, 3853 triangles, 83 lists refused for ops other than vertices, triangles and geometry mode (the old compiler refuses them too). Not used by a draw yet.
- Prof tag: every prof window prints `prof f<n> w<n> at <map> <area>:<map> m<mode> sp<story> b<battle> e<entry> chg0 pos0 x y z pos x y z`, windows restart on a map change and on the pos= placement. 3ds/tools/prof_by_scene.py compares logs per scene. Scenario prof_scene.
- Save tool (Haiku agent, 23c6ca9ce): 3ds/tools/savetool.py, debug switch savefile=<path> (needs warp=), scenario savefile_check.

## Finding for S3-2 (decide before coding)
The old raw path is already index-list based. The CPU cost (prof lg 22 to 29 ms on the O3DS XL, Toad Town) is the walker: op decode, state sync, G_PM_MODEL headers, dlc_try. The lowest risk first native step reuses pm_gpu: nd_emit consumes DRAW_IDX records and calls pm_gpu_grange and the existing flush_draw (which owns the TEV rebuild, texture bind, uniforms and the sh_* shadows). Producers (model.c appendGfx_model under TARGET_3DS) then write NdRec (nd_obj_draw with a texture id, a render mode, a matrix) and skip the G_PM_MODEL words. Writing PICA words from templates (nd_tmpl words, nd_tex blocks, nd_mtx pool) directly into the command buffer is a second step, after the producers are native and the pictures are equal. nd_tmpl, nd_tex and nd_mtx are ready for it and host tested against ndref.

## S3-2 findings (2026-11-04, measured)
- Console, Toad Town (phase sp0c opsplit mr0, build with S2 bridge): sp0c per frame tex 5011 us, dlc 2490, rest 1424, flush 1298. mr0: plain models 56 per frame, per model ops 142 us, close 31 us, outside dlc 174 us (console x2.86 outside dlc 499 incl. the factor). ms0: 29 opaque plain models, 11 texture changes per frame.
- Azahar ratios (sp0x, tex_op ticks per op code per frame): G_LOADTLUT 1273 us for 94 calls (13.5 us each), G_SETTILE 996/303, G_SETCOMBINE 840/124, G_SETTIMG 368/199, G_SETOTHERMODE_H 356/182, G_SETTILESIZE 329/106. Texture cache hits cost 3 us per call (36 per frame): pm_tex_load is not the cost.
- Tried and removed: sorting the plain opaque model blocks of the main list by texture handle (blocks have lights, so they are sortable). Result: no fewer closes (cl 35 to 34). Reason: a raw batch holds one game matrix at palette entry 0 (grange_batch_open needs the same gm), so every model is its own draw whatever the texture order. Merging models into one draw needs a per-draw matrix (the NdRec mtx slot) or per-vertex palette indices, i.e. the native emit.
- So the saving has to come from the cost per model (ops 142 us on the console), not from fewer draws: the model words (texture list of 5 to 8 words, combine, render mode, matrix, DL) cost more than the close (31 us).
- Next: run the sp0x line on the console (netloader was offline during the session): PM_ROW=PUSH sh 3ds/push_3ds.sh --no-build --ip 192.168.1.131 -t 100 warp=mac_01:0 story=ch0_toad_town phase sp0c opsplit loglevel=debug. The sp0x line splits the tex bucket per op code on the real hardware.

## Console sp0x, Toad Town, phase on (2026-11-04, push 192.168.1.131, build 38955a32d tree, no sort)
- tex ops us/calls per frame: G_LOADTLUT 5808/116, G_SETCOMBINE 4096/206, G_SETTILE 3468/459, G_SETTILESIZE 2214/180, G_SETOTHERMODE_H 1027/314, G_SETTIMG 977/288. sp0c tex 11403, dlc 6204, rest 3385, flush 1191 (phase on runs slower than play). mr0: 90 plain models per frame, ops 199 us, close 51 us per model.
- The time of an op includes the draw close that its state change forces: the first state op of a model (LOADTLUT on a palette change, SETCOMBINE on a plan change, SETTILE on a wrap change) closes the draw of the model before it. 90 closes at 51 us is about 4.6 ms of the 11.4 ms tex bucket. The other 6.8 ms is interpretation: sync_combine (sc_sig hash, memo, plan replay), tile and TMEM bookkeeping, ensure_tile.
- So two levers, both inside the native emit: (1) the close (flush_draw: tev rebuild, state setters, DrawElements): template words and one draw per record; (2) the per-model state work: one descriptor bind (nd_tex_get) and one cached combine per template instead of 5 to 8 tex_op words and a sync_combine per model.
- G_LOADTLUT at 50 us per call is mostly the forced close, not the palette work (tlut_v20 is a two entry compare).

## S3 M1 done: compiled shape objects in the static pools (2026-11-04)
- nd_dlc.c compiles a list into NdObj with the vertex copies in the static vertex area (pm_gpu_gstat_alloc) and the triangle indices in a new static index area (pm_gpu_gidxs_alloc, 128 K entries, reset with the vertex area). Game gate: debug switch ndobj, scenario ndobj_mac01: 113 lists, 187 groups, 1924 triangles equal to the DlcEnt (counts, boxes, geometry ops, group op ranges, index deltas), bad 0. Host test also compiles into pool arrays.
- Seam rules (plan 13) for every next milestone: (1) the draw matrix is nd_mtx_pcomp(post, game), the same product and order as pal_mul (bit equal on 239 ndref products, nd_seam_test step 2). (2) vertices stay raw s16 Vtx copies (nd_dlc copies the bytes, flag word 0, no snapping). (3) the vertex shader is not changed and gets no new per vertex arithmetic: any scale goes in the matrix. (4) post matrix, viewport and tilt (rebuild_3d) stay as they are. (5) near clip plane (sh_user_clip), depth map -1 and decal bias come from nd_tmpl words (host tested against the old state). (6) only opaque depth tested records may be reordered. Gate G9: nd_seam_test and the mac01_seam picture before and after each milestone.

## Console, build c640f84f9 (nd_dlc compile in the walker), Toad Town, phase sp0c opsplit pmu (2026-11-04)
- Regression check passed on the O3DS XL: skip 15410 and grp 37500 per window are the same as before M2. sp0c tex 12019, dlc 6808, rest 3639, flush 1309 (pmu on adds a little).
- PMU (sp0 window): pmu_ic 173304, pmu_dc 62841, cyc 17086739. At a miss cost of 50 to 70 cycles on the O3DS (no L2), instruction cache misses are about 50 to 70 percent of the cycles of the model class, data cache misses about 20 percent. The cost of the walker is its code footprint (the model path runs far more than the 16 KB L1I per model), not its logic. Cure: a hot path of a few KB that copies words, which is the nd_emit design (hw_guidelines: hot loops fit 16 KB L1I and L1D).
- Next (M3 and M4): native emit of command words. Oracle plan: capture the command words of each close of the old path into ndref (kind 6) and host test nd_emit against them. Seam rules above apply.

## S3 M3 done: native raw close (2026-11-04, commit d813af1f5)
- nd_emit.c writes the command words of a raw pm4 draw (template, TEV colours, texture unit blocks and enable words, palette entries, uvso/cmul/cadd/zmix, draw tail) with a shadow; host test nd_emit_test compares 9013 registers and 6576 uniform words with the real command stream of the old path (ndref v4 kind 6, 231 raw draws). nd_cmd_test: 12192 template registers and 632 texture blocks equal the stream. One draw of 632 has a stale texture unit in the old path (slot reused without a rebind): the native path writes the right words.
- pm_gpu.c nd_raw_native (called from flush_draw): eligible batch = raw palette batch, program pm4, not lit, main target, no stencil pass, at most unit 0. It stages program, buffer, scissor and user clip through citro3d as before, calls C3Di_UpdateContext, builds the words and adds them with GPUCMD_AddRawCommands. nd_leave_native (at a frame begin, a split and before a citro3d draw) sets the context dirty bits for the effect block, texenv and texture units and clears the program binding, so citro3d re-emits what the walker staged. Texture words come from pm_tex_native_unit0 (same residency work as pm_tex_bind, no citro3d call).
- Azahar: yard_shot, mac01 and ndst_yard pictures equal; ndst log shows native 2505 and 3800 draws per 100 frames, refused 0. Not yet run on the console: netloader was closed at the push.
- Not covered: lit batches (pm5), two texture units, off-screen targets, zmark and decal passes, the CPU vertex path: they still use the old close.
