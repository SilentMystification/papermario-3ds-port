# Row HC8: sprite and entity fast paths (plan_hotcode.md HC8, classes H and I) - wave 3

## User facts
User (2026-11-09): shrink the hot code a lot; waves with no merge conflicts. HC0: M3 class H mac_01 10.9 KB, kmr_10 16.0 KB; class I
8.7 KB. Shared oracle plan (code read; step 1 measures the call counts again).

## Start state
`sh 3ds/tools/wt_new.sh HC8` after HC4 merges (wave 3; after HC7 if both edit pm_gpu.c). PM_ROW=HC8, PM_LANE=HC8. One Haiku agent.
Files: 3ds/src/pm_gpu.c (pm_gpu_spr_build, pm_gpu_grange), nd_model.c (nd_model_state_spr memo), nd_ent.c, nd_shape.c; HC8b adds
nd_cmd.h, nd_rec.c, nd_cmd_test.c.

## Steps (each a commit, byte-equal)
1. fncov_mac01_det and fncov_kmr10_det on the base; add a sprite memo miss counter next to mw_ok; `bench.py base` for both gates.
2. Sprite memo (nd_model.c:383-390): 16 sets x 4 ways with LRU in the set (64 entries), key unchanged; compare the most-varying
   words first (prim, env, mixcol, comb_w0, comb_w1, then the rest); build the key from the walker globals and fill NdMCtx only on a
   miss (pm_gbi_gv_at stays). About 21 KB: heap at the first sprite if the memcheck margin is tight.
3. pm_gpu_spr_build (pm_gpu.c:3647): the shaded branch (:3672-3678), the sprtex log and the spr5 census -> spr_build_rare (cold);
   the nd_model_state miss call site cold.
4. dlc_box_out (nd_shape.c:278): dlc_planes(m, pl[5][4]) once per dlc_try and per pm_dlc_vis_mask / native_ranges; dlc_box_out_pl
   loops the 5 planes with the same eps rule and the same float operation order (bit-equal); the stereo two-eye test -> cold
   dlc_box_out_stereo, used only when sx is set.
5. dlc_try (nd_shape.c:546): build/hash (:551-559) and the why/fallback arms (:584-605) -> dlc_try_slow, dlc_group_slow (cold).
6. pm_gpu_grange (pm_gpu.c:2799): init block -> grange_open_new (noinline); param compare and flush -> grange_flush_if_new (noinline).
7. ent_run (nd_ent.c:373): recompile, REF retry, entchk and the interpreter fallback loop -> ent_run_slow (cold).
8. HC8b only after HC4c merged: typed ops in EntKey segments (ent_compile), NDC_SPR typed fields (sprite decode on core 1).
   ent_hash stays on core 0.

## Gates (light)
- nd_host PASS. Byte-equal against stored base shots: battle_start_f182_cold, nok11_det.
- Size: M1 class H <= 3 KB, class I <= 5 KB; M3 mac_01 class H <= 4 KB, kmr_10 class H <= 6 KB with sprite memo hit rate > 95%.
- memcheck OK. Console (user): prof push mac_01 0 0 200, pmu I/D per frame.
