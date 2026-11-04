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
