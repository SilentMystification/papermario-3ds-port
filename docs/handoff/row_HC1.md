# Row HC1: cold split of the core 0 draw path (plan_hotcode.md stage HC1)

## User facts
User (2026-11-09): "We need to shrink the hot code by a very significant amount." Console XL de415917f mac_01: 37.4k I-miss, 20.2k
D-miss per frame. HC0 (merged): M1 union 122.9 KB, M3 code run per frame mac_01 112.5 KB (381 functions).

## Start state
`sh 3ds/tools/wt_new.sh HC1`, branch HC1. PM_ROW=HC1, PM_LANE=HC1. One Haiku agent. Read plan_hotcode.md sections 0, 1 and 2 (HC1).
Rows HC2 (link order script) and HC9A (pm_tex slot index) run in parallel: do not edit 3ds/ld/* or pm_tex_slot_find.

## Steps (no behaviour change)
1. Run `python 3ds/tools/bench.py run hotset_size --row HC1` on the base lane build: record M1 per class and the call-site list (log,
   debug, timer calls per hot function).
2. Move each debug, census, log and rare branch out of the hot functions into `__attribute__((cold, noinline))` helpers; test the
   switch at the call site through one global debug word (as pm_walk_dbg). Named sites: pm_tex_stale_chk and texsrc_chk at
   nd_tex.c:114 and :133; rect_sig_check at pm_gpu.c:682 and pm_gpu_rect.c:98; the PMDL tests in rec_run, sync_combine, ensure_tile,
   nd_texrect, pm_gpu_tex_rect, pm_gpu_fill_rect, pm_gpu_spr_build, ent_run, nd_geom_vtx, nd_raw_native; rec_run's waterlog, ndm and
   miss diagnostics; nd_raw_native's ndm, orb and plan checks; the NDT_BD/NDT_E timers and walk_dbg in nd_walk_state.
3. Move the pm_gpu.c debug and census groups (ndm_*, sbkw, rect_sig_*, flushlog, ndref, orb, cen) to a new 3ds/src/pm_gpu_dbg.c
   compiled -Os (as nd_cen.c).
4. Mark the rare walk arms cold: G_SETCIMG, G_MODIFYVTX, G_SETCONVERT, G_SETKEY*, G_PM_ND, bad pointers.
5. Commit in pieces (one per file group), each with its byte gate.

## Gates (light)
- nd_host PASS.
- Byte-equal against the stored base shots (`bench.py base` / `cmp`): spot_mac01_det, battle_start_f182_cold.
- hotset_size: M1 union down by at least 12 KB. fncov_mac01_det (census lane): M3 down by at least 10%. Report both before/after.
- memcheck OK. Console (user): one prof push at mac_01 0 0 200 (pmu_i per frame vs de415917f).
