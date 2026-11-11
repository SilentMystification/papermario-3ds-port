# Row HC8: sprite and entity fast paths (plan_hotcode.md HC8, classes H and I) - wave 3

## User facts
User (2026-11-09): shrink the hot code a lot; waves with no merge conflicts. HC0: M3 class H mac_01 10.9 KB, kmr_10 16.0 KB; class I
8.7 KB. M1 at 094ce15dc: H 15.5 KB, I 11.9 KB. Shared oracle plan (code read; step 1 measures the call counts again).

## Start state
`sh 3ds/tools/wt_new.sh HC8` now (HC4 merged at f3f548888; line numbers checked at 141c5e371). Steps 1-7 may run with HC7b (no shared file); step 8 (HC8b) after HC7b merges (both edit nd_cmd.h, nd_rec.c). PM_ROW=HC8, PM_LANE=HC8. One Haiku agent.
Files: 3ds/src/pm_gpu.c (pm_gpu_spr_build, pm_gpu_grange), nd_model.c (nd_model_state_spr memo), nd_ent.c, nd_shape.c; HC8b adds
nd_cmd.h, nd_rec.c, nd_cmd_test.c.

## Steps (each a commit, byte-equal)
1. Done (f9980895e): sprite memo counter; census fncov_mac01_det run 253145 and fncov_kmr10_det run 253967 on lane HC8_fncov. A census
   bench has no base: its counts are the first measurement (a normal build prints no fncov line, so `bench.py base fncov_*` is always
   broken; never run it). `git checkout 3ds/tools/azt_scenarios.json` (drop the "unverified" mark the broken base run wrote). Then
   `PM_LANE=HC8 python 3ds/tools/hotset.py --fncov build3ds/azt/logs/fncov_mac01_det.txt > build3ds/out/m3_mac01_before.txt` and the
   same for kmr10 (copy the two census logs to build3ds/out/ first): the M3 per class before steps 2-7; write class H and I in the commit.
2. Sprite memo (nd_model.c:389-410, SprMemo sprm[SPRM_N] at :396): 16 sets x 4 ways with LRU in the set (64 entries), key unchanged; compare the most-varying
   words first (prim, env, mixcol, comb_w0, comb_w1, then the rest); build the key from the walker globals and fill NdMCtx only on a
   miss (pm_gbi_gv_at stays). About 21 KB: heap at the first sprite if the memcheck margin is tight.
3. pm_gpu_spr_build (pm_gpu.c:3796): the shaded branch, the sprtex log and the spr5 census -> spr_build_rare (cold);
   the nd_model_state miss call site cold.
4. dlc_box_out (nd_shape.c:283): dlc_planes(m, pl[5][4]) once per dlc_try and per pm_dlc_vis_mask / native_ranges; dlc_box_out_pl
   loops the 5 planes with the same eps rule and the same float operation order (bit-equal); the stereo two-eye test -> cold
   dlc_box_out_stereo, used only when sx is set.
5. dlc_try (nd_shape.c:608): its build/hash block and the why/fallback arms -> dlc_try_slow, dlc_group_slow (cold).
6. pm_gpu_grange (pm_gpu.c:2924): init block -> grange_open_new (noinline); param compare and flush -> grange_flush_if_new (noinline).
7. ent_run (nd_ent.c:378): recompile, REF retry, entchk and the interpreter fallback loop -> ent_run_slow (cold).
8. HC8b only after HC4c merged: typed ops in EntKey segments (ent_compile), NDC_SPR typed fields (sprite decode on core 1).
   ent_hash stays on core 0.

## Gates (light)
- nd_host PASS. Byte-equal (`bench.py base`, run, `cmp`): spot_mac01_det and mdl5_iwa10_det (bases stored at b8bc06613; both scenes
  have sprites and entities). Not battle_start_f182_cold: its two cold runs of one build differ (base --fill at b8bc06613), so it
  cannot be a byte gate (row LOADMTX note). nok11_det only if its base prints "two runs equal, stored"; else drop it and say so.
- Size: M1 class H <= 3 KB, class I <= 5 KB; M3 mac_01 class H <= 4 KB, kmr_10 class H <= 6 KB with sprite memo hit rate > 95%.
- memcheck OK. Console (user): prof push mac_01 0 0 200, pmu I/D per frame.
