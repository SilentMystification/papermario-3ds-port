# ws_NCONV1D2 (Haiku): a list draw takes its combine, render state and binds from its snap, in path P1 and path P2 (stage S5. Counter: none of C1-C12. Gate: byte-equal, no expected-diff)
Designer: oracle NCONV1D. WAIT: branch from main after the NCONV1D1 merge, and only when its hand-back shows `lsnap` bad 0, tmis 0, over 0, p1 above 0 and wst 0 in both list scenes. A wst above 0 means the walker tiles of a list are stale: the oracle asks the user for an expected-diff gate first.
Worktree build3ds/wt/NCONV1D2, branch NCONV1D2. PM_ROW=NCONV1D2 PM_LANE=NCONV1D2. Edit only (Edit tool, keep CRLF): 3ds/src/nd_snap.c, 3ds/src/nd_geom.c, 3ds/src/nd_shape.c.
## Facts (code read, main after NCONV1D1. Find each line by its text.)
- FACT nd_geom.c draw_tri: the lines `nd_snap_rpend = nd_snap_arm;` and `if (nd_snap_arm) { nd_snapchk_tri(); ...}` consume an armed snap (combine apply, render state and binds at the epoch enter, the bind epoch bump of NDR0G2). P2 draws through draw_tri.
- FACT nd_shape.c:428-433: dlc_seg (path P1) has no draw_tri. It enters the epoch itself (`if (ep_epoch != gv_epoch) gv_epoch_enter();`). So it takes the same steps. dlc_seg is shared with map shapes: nd_ls_on gates every new line.
- After NCONV1D1, nd_lsr_tri loads the range snap into sc_cur at each range start and compares it under snapchk. This row replaces that compare with the arm: nd_snapchk_tri compares and applies.
- A walker epoch bump inside a snap range (a light, for example) needs the snap too: `nd_snap_rpend` is `arm | nd_ls_on`.
## Edits
1. nd_snap.c nd_lsr_tri. Before (after `lr_cur = r;`): `if (!PMDL("snapchk")) return;` then the memcpy, `const unsigned b0 = sc_bad, w0 = sc_wst;`, `lr_cmp++; lr_c1 += lr_p1; sc_compare(); nd_snapchk_rnd(); lr_wst += sc_wst - w0;` and the `if (sc_bad != b0) {...}` line.
   After: `lr_cur = r; memcpy(&sc_cur, (const char*)lr_b + ND_LS_HDR(n) + r * sizeof sc_cur, sizeof sc_cur); nd_snap_arm = 1; lr_cmp++; lr_c1 += lr_p1; }`. Delete `lr_nlog` from the declaration (unused). The `bad` and `wst` of the lsnap line now stay 0: the snapchk line counts them.
2. nd_geom.c draw_tri: `nd_snap_rpend = nd_snap_arm;` becomes `nd_snap_rpend = nd_snap_arm | nd_ls_on; /* NCONV1D2 */`.
3. nd_shape.c dlc_seg. Before: `if (nd_ls_on) nd_lsr_seg(nd_ls_ord);` and the next line `if (ep_epoch != gv_epoch) gv_epoch_enter();`. After:
   `if (nd_ls_on) nd_lsr_seg(nd_ls_ord); const unsigned char sn = nd_ls_on && nd_snap_arm; /* NCONV1D2: the steps of draw_tri */`
   `if (sn && ep_epoch == gv_epoch && nd_snap_bnew(nd_ep_bs)) gv_epoch++; nd_snap_rpend = nd_ls_on; if (sn) { nd_snapchk_tri(); if (ep_epoch == gv_epoch) nd_snapchk_rnd(); }`
   `if (ep_epoch != gv_epoch) gv_epoch_enter(); nd_snap_rpend = 0;`
## Steps (output to build3ds/out/, read the last 20 lines. 1 build, at most 8 Azahar starts)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nc1d2_host.txt 2>&1`: PASS. Commit `NCONV1D2 1: list draws consume their snap`.
2. One call each: `python 3ds/tools/bench.py base <b>` and `sh 3ds/tools/rowloop.sh --row NCONV1D2 --bench <b> --no-host` for b = fxops_drec, entcape_kmr03_drec, nimgm_strip_drec. For a DIFF: `python 3ds/tools/drecsum.py <b> --row NCONV1D2` and `grep -a "bindsnap\|drec frame" build3ds/azt/logs/<b>.txt`.
3. One call each: `python 3ds/tools/bench.py run <s> --row NCONV1D2 --why "NCONV1D2 consume"` for s = fxops_snapchk, entcape_kmr03_snapchk. `grep -a "snapchk\|lsnap" build3ds/azt/logs/<s>.txt > build3ds/out/nc1d2_<s>.txt`.
Gate (exact). Host PASS. The 3 drec benches are byte-equal. In both runs the last `snapchk` line has bad 0 (it counts the list compares now) and the last `lsnap` line has tmis 0, over 0 and cmp above 0. The sum of p1 over both runs is above 0. Report wst, cmp, p1 per run.
Stop: any DIFF (no expected-diff gate has the user's yes for this row: send the drecsum line, the first DIFF draw and the bindsnap lines to the oracle), a crash, a fatal line, bad above 0, a tool fault. Commit the outputs, send the paths to the oracle with SendMessage, wait. Fix rights: the 3 listed files, compile errors.
Pitfalls: one host test at a time (rowloop takes --no-host), PM_CACHE=off only as a prefix of one command, one bench per rowloop call. The perf cost is OPEN: the end prof push measures it.
Hand-back: last commit `NCONV1D2 2: gate. 3 drec equal; snapchk bad 0, lsnap cmp <n> p1 <n>` deletes this page. One SubagentHandback: commit hash, the lsnap and snapchk lines of both runs.
