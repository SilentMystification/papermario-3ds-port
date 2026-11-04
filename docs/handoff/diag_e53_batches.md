# E53 batch diagnosis (census_mac01, lane nds1, frame 200)

Scope: find the source of the +7 batches of the E5-3 learned replay patch (docs/handoff/nds1_e53_wip.patch).
Row E53D. Starts used: 3 of 4 (runs 1 to 3). Budget NDS1 reset before the first start. No patch, no debug code in the commit.

## Results (census_mac01, cen f200 summary)

| Run | Build | Batches | Tris | Distinct | Live | Learn begin | Learn end | Replay (play) |
|---|---|---|---|---|---|---|---|---|
| 1 | Patch, E53D counters | 71 | 810 | 54 | 57 | 0 | 0 | 14 |
| 2 | Patch, dlc_nat_group off | 64 | 810 | 53 | 64 | 0 | 0 | 0 |
| 3 | Patch, nat off, TEXW and use_t1 learn refusals off | 64 | 810 | 52 | 64 | 0 | 0 | 0 |
| HEAD (prior run) | HEAD 33fe90ab1 | 64 | 810 | 52 | - | - | - | - |

Learn windows, cumulative since boot (run 1): 72 calls, 57 ok in run 3 (56 ok in run 1). Refusals in run 1: pm_gpu_rec_end not 1 = 15, TEXW word check = 1, use_t1 = 0, capacity = 0.
Nat replay, cumulative since boot (run 1): 2242 replays ok, refused view 1770, slot 6645.

## Findings

1. The learn windows add no batches at frame 200. The learn begin and learn end flushes hold no pending batch (begin 0, end 0 in all runs).
2. The +7 batches come from the learned replay in dlc_nat_group (pm_dlc.inc.c dlc_try, call of dlc_nat_group, then pm_gpu_rec_play in pm_gbi.c). Run 1 has 14 batches from the replay cause and 57 live. Run 2 has 64 live and 0 replay.
3. The live path draws the replayed group as 7 batches (64 minus the 57 live batches of run 1, assuming the other groups are the same). The replay draws it as 14 batches. The replay issues one draw entry per learned batch (nd = 14 at frame 200). The learned batch structure comes from the learn frame, not from the current frame.
4. The earlier note "only 1 learned replay" is wrong. The log line 'E5-3 learned replay draws' prints only the first 4 replays (lnr_runs <= 4). The counter shows 2242 replays ok since boot.
5. The TEXW rule in dlc_learn_end (learn refused when a unit 0 flush differs from the template) refuses 1 learn. It changes 1 distinct tuple (53 instead of 52). It does not change the batch count. Run 3 removes it and gives 52, equal to HEAD.
6. The use_t1 refusal refused 0 learns at frame 200 or in the run. It is not the cause.

## Smallest change that keeps batches at 64 (run 2 and run 3 measured)

Remove the replay. The learn-end capture is then unused.
- 3ds/src/pm_dlc.inc.c, dlc_try: replace `(!rec_on && dlc_nat_group(e, (DlcGrp*)g, gmi, gg)) || dlc_static_group(e, g, gmi, gg)` with `dlc_static_group(e, g, gmi, gg)`. Delete the dlc_nat_group prototype and the DLC_LNR and lnr_* fields and their use in dlc_learn.
- 3ds/src/pm_gbi.c: delete dlc_nat_group. In dlc_learn_end, delete the NatRec and census capture (ln_* writes, pm_gpu_rec_pool, nat_rec_write). Restore the HEAD TEXW block (log only, no refusal) and the HEAD use_t1 refusal (no `|| use_t1`).
- 3ds/src/pm_gpu.c: delete pm_gpu_rec_sa, pm_gpu_nat_addr_at, pm_gpu_rec_pool. Restore `if (off_active || !ia) return 0;` in pm_gpu_rec_play.
- 3ds/src/pm_native.c and 3ds/include/pm_native.h: revert the NAT_TEX_TMPL change in nat_emit and the nat_pool_ptr accessor.

Result: HEAD census (64 / 810 / 52). The worktree state equal to HEAD (33fe90ab1) with the E5-3 patch dropped meets the census gate.

## Open items
- The replay of a learned group adds GPU draw entries for the group (7 extra batches at frame 200 in this scene). This is a draw-call cost, not only a census number. Not measured on the console.
- The replay path needs a learn-to-frame batch structure check before any future replay. Not done.
