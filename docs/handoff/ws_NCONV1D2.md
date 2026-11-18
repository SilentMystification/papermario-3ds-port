# ws_NCONV1D2 (Haiku): a list draw takes its combine, render state and tile words from its range snap, in path P1 (dlc_seg) and path P2 (draw_tri). Stage S5. Counter: none of C1-C12. Gate: byte-equal, no expected-diff. New counter: lsnap `bst`.
Designer: oracle NCONV1D2. Branch from main (5eddc668ee or later). Worktree build3ds/wt/NCONV1D2, branch NCONV1D2. PM_ROW=NCONV1D2 PM_LANE=NCONV1D2.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_snap.c, 3ds/include/nd_snap.h, 3ds/src/nd_geom.c, 3ds/src/nd_shape.c. Find each line by its text.
## Facts (code read, main 5eddc668ee)
- FACT NCONV1D1 hand-back: fxops_snapchk lsnap cmp 1583, wst 1577. The old `wst` compares the walker tiles at the range start. It does not show the tiles that the draw binds.
- FACT nd_geom.c:213: gv_epoch_enter binds from the snap blocks (nd_ep_bs) only when nd_snap_rpend is 1, else from the walker tiles (nd_ws.td, nd_ws.tst1).
- The new counter `bst` counts list triangles in an open epoch whose bound tile words (nd_ep_bs, NULL = walker) differ from the range snap. This row makes every list epoch bind from the snap, so bst goes to 0.
- FACT: no host unit links nd_snap.c, nd_geom.c or nd_shape.c (oracle_handoff.md). Add no host test. Step 1 is the seeded-fault self-test of bst: the code before step 2 binds walker tiles.
## Step 1 edits (the counter only)
1a. nd_snap.c:245, in the declaration line: `lr_tmis, lr_nlog;` becomes `lr_tmis, lr_nlog, lr_bst;`. After the nd_lsr_tri function (after its closing `}`), add:
   `void nd_lsr_bst(void) { if (PMDL("snapchk") && ep_epoch == gv_epoch && nd_snap_bnew(nd_ep_bs)) lr_bst++; } /* NCONV1D2: a list triangle whose epoch binds other tile words than its range snap */`
1b. nd_snap.c:270, the lsnap line: `wst %u tmis %u` becomes `wst %u bst %u tmis %u`, and `lr_wst, lr_tmis` becomes `lr_wst, lr_bst, lr_tmis`.
1c. nd_snap.h:93: at the end of the line `... void nd_lsc_unit(uint32_t w0);` add ` void nd_lsr_bst(void);`.
1d. nd_geom.c:357. Before: `        nd_snap_rpend = 0; /* NDR0F1B */`. After: `        nd_snap_rpend = 0; /* NDR0F1B */ if (nd_ls_on) nd_lsr_bst(); /* NCONV1D2 */`.
1e. nd_shape.c:435. Before: `    if (ep_epoch != gv_epoch) gv_epoch_enter();`. After: `    if (ep_epoch != gv_epoch) gv_epoch_enter(); if (nd_ls_on) nd_lsr_bst(); /* NCONV1D2 */`.
## Step 2 edits (consume the snap)
2a. nd_snap.c nd_lsr_tri. Before (after `lr_cur = r;`): the 6 lines from `if (!PMDL("snapchk")) return;` to the `if (sc_bad != b0) {...}` line. After (keep the closing `}`):
   `    memcpy(&sc_cur, (const char*)lr_b + ND_LS_HDR(n) + r * sizeof sc_cur, sizeof sc_cur);`
   `    nd_snap_arm = 1; lr_cmp++; lr_c1 += lr_p1; /* NCONV1D2: the next draw step consumes the range snap (nd_snapchk_tri) */`
   In the declaration line delete `lr_bad, lr_wst, ` and `lr_nlog, `. Lsnap line: `p1 %u bad %u wst %u bst %u` becomes `p1 %u bst %u`, and `lr_c1, lr_bad, lr_wst, lr_bst` becomes `lr_c1, lr_bst`.
2b. nd_geom.c:353. Before: `nd_snap_rpend = nd_snap_arm; /* NDR0F1B: ...` (keep the comment). After: `nd_snap_rpend = nd_snap_arm | nd_ls_on; /* NCONV1D2: a list triangle binds from its range snap. NDR0F1B: ...`.
2c. nd_shape.c dlc_seg. Before: `    if (nd_ls_on) nd_lsr_seg(nd_ls_ord);` and the line from 1e. After (6 lines in place of the 2):
   `    if (nd_ls_on) nd_lsr_seg(nd_ls_ord);`
   `    const unsigned char sn = nd_ls_on && nd_snap_arm; /* NCONV1D2: the snap steps of draw_tri (nd_geom.c) */`
   `    if (sn && ep_epoch == gv_epoch && nd_snap_bnew(nd_ep_bs)) gv_epoch++;`
   `    nd_snap_rpend = nd_ls_on; if (sn) { nd_snapchk_tri(); if (ep_epoch == gv_epoch && nd_snap_rpend) { nd_snapchk_bind(); nd_snapchk_rnd(); } }`
   `    if (ep_epoch != gv_epoch) gv_epoch_enter();`
   `    nd_snap_rpend = 0; if (nd_ls_on) nd_lsr_bst(); /* NCONV1D2 */`
## Steps (one command per step, output to build3ds/out/, read the last 20 lines. Bash `timeout: 600000`. 2 builds, 3 counted Azahar starts of 8)
1. Step 1 edits. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nc1d2_host1.txt 2>&1`: PASS. `PM_ROW=NCONV1D2 PM_LANE=NCONV1D2 sh 3ds/build.sh > build3ds/out/nc1d2_build1.txt 2>&1`.
2. `PM_ROW=NCONV1D2 PM_LANE=NCONV1D2 python 3ds/tools/bench.py run fxops_snapchk --row NCONV1D2 --why "NCONV1D2 bst before"`, then `grep -a "snapchk f\|lsnap f" build3ds/azt/logs/fxops_snapchk.txt > build3ds/out/nc1d2_before.txt`.
   Pass: the last lsnap line has bst above 0 (GUESS: above 0, the walker tiles differ in 1577 ranges). bst 0 is a fault of the counter: stop. Commit `NCONV1D2 1: lsnap bst; fxops bst <n> before`.
3. Step 2 edits. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nc1d2_host2.txt 2>&1`: PASS. Commit `NCONV1D2 2: list draws consume their range snap`.
4. `python 3ds/tools/bench.py base fxops_drec --commit $(git merge-base HEAD main) > build3ds/out/nc1d2_base.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row NCONV1D2 --bench fxops_drec --no-host --why "NCONV1D2 consume"` (it builds).
5. `PM_ROW=NCONV1D2 PM_LANE=NCONV1D2 python 3ds/tools/bench.py run fxops_snapchk --row NCONV1D2 --why "NCONV1D2 consume"`, then grep as in step 2 into build3ds/out/nc1d2_after.txt.
Gate (exact): host PASS twice. fxops_drec cmp byte-equal (rowloop exit 0). After run: last snapchk line bad 0. Last lsnap line bst 0, tmis 0, over 0, cmp above 0, p1 above 0.
Stop: a DIFF (the user has not said yes to an expected-diff gate for this row). Run `python 3ds/tools/drecsum.py fxops_drec --row NCONV1D2 > build3ds/out/nc1d2_drecsum.txt` and `grep -a "bindsnap\|drec frame" build3ds/azt/logs/fxops_drec.txt > build3ds/out/nc1d2_bindsnap.txt`. Also stop for: bst above 0 after step 2, bad above 0, a crash, a fatal line, a tool fault. Commit the outputs (`git add -f`), send the paths to the oracle with SendMessage, wait.
Fix rights: the 4 listed files, compile errors only. Pitfalls: one host test at a time. One bench per rowloop call. The perf cost is OPEN (the end prof push measures it).
Hand-back: last commit `NCONV1D2 3: gate. fxops drec equal; snapchk bad 0; lsnap bst <before> to 0, cmp <n> p1 <n>` deletes this page. One SubagentHandback: commit hash, the snapchk and lsnap lines of both fxops_snapchk runs.
