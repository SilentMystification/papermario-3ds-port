# ws_MSNAP2 (Haiku): a model record takes its 5 decision words from the decision cache, not from a full snap build
Designer: oracle NATIVE17 (plan_perf.md section 11, finding C2). Start now from main. Parallel with PROF3 (other files). Worktree build3ds/wt/MSNAP2, branch MSNAP2.
PM_ROW=MSNAP2 PM_LANE=MSNAP2. Lines: main f0a9f642e4; grep the quoted text.
Why Haiku: one new function made from lines of snap_build0, one call changed. The equivalence proof is below. The gate is byte-equal.
Objective: core 1 builds no full snap per model record. Counter: snapi `snn` per 100 frames falls by about the model record count (GUESS -90 per frame).
Console number: c1t snap 4.78 ms (FACT build 7680) to about 2.5 ms (GUESS -2.2 ms core 1: 93 model records x about 24 us).
## Proof (FACT code read)
- nd_msnap_rec (nd_msnap.c:22) calls nd_snap_now (nd_snap.c:87) only to fill d_plan, d_mode5, d_vtx, d_vtxname, d_t1 from the returned decision.
  nd_snap_now runs snap_build (memset, nd_snap_in, render state, 2 bind blocks, about 500 B of hash, intern, 760 B stream copy) and discards the snap.
- The memo exit of snap_build0 (nd_snap.c:60) never runs here: nd_rec_model counts the record (ndc_n[NDC_MODEL]++, nd_rec.c:232) before
  nd_msnap_rec (:238), so nd_rec_cmds() (nd_rec.c:706) differs from sn_cmds.
- An intern hit (nd_snap.c:64-65) returns a snap with the same in, m5, vprim: the decision cache gives the same o (the decision is a function of in, m5, vprim).
- Side effects to keep: sn_leg, sn_mode5, sn_pat from o (nd_snap.c:76) and sn_cmds = ~0u (:87). sn_lp is read only by the memo test, which sn_cmds = ~0u disables.
- The model snap was data in the stream that no runner applied. Later snaps that hit it in the intern table now get their own copy: same words, other address.
## Edits (nd_snap.c and nd_msnap.c only)
1. nd_snap.c after nd_snap_now, a new function (the SD part of snap_build0 :61-79 on a local in, without the snap):
```c
static NdCombOut sd_now; /* MSNAP2: the decision of nd_ds now, for the model snap words */
const NdCombOut* nd_snap_dec(void) {
    NdCombIn in; nd_snap_in(&nd_ds, sn_leg, sn_pat, &in);
    const int m5 = sn_mode5; const uint32_t hi = si_hw(2166136261u ^ (uint32_t)m5 ^ sn_vprim << 8, &in, sizeof in);
    /* copy the lines from `if (!sd_t && !sd_tried)` to `sn_leg = r.o.leg; ...` with r.in -> in, r.o -> sd_now, r.key -> a local key */
    sn_cmds = ~0u; return &sd_now;
}
```
2. nd_msnap.c:13 and :22: `nd_snap_now()` to `nd_snap_dec()` (and its prototype). nd_snap_now then has no caller: delete it (nd_snap.c:87).
## Checklist (hw_guidelines.md 6): no 760 B build, hash or stream copy per model record. The SD table read is the same as before.
## Tests (build3ds/out/, last 20 lines). Gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=MSNAP2 sh 3ds/build.sh`; c0gate unchanged.
2. `bench.py run spot_mac01_drec,entcape_kmr03_drec --row MSNAP2 --why "MSNAP2 gate"`; `bench.py cmp <b> --row MSNAP2` byte-equal for both.
3. Note the last `snapi f` line (new, hit, snn, dec) of base and tip logs of spot_mac01_drec. Expect: snn lower, dec equal or lower.
Fix rights: compile errors (static helpers si_hw, sd_t, SdEnt are in nd_snap.c already). A DIFF, a crash: stop, send the path to oracle NATIVE17.
Stop: last commit `MSNAP2 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hash, host, the 2 cmp lines, the snapi lines base and tip.
