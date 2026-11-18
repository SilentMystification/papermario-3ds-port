# ws_NREC4 (Haiku): particle runs carry an instance snap table. Core 0 particle draws consume it. Counter: none of C1-C12 (stage S6 consume). Gate: fxp drec byte-equal, snapchk bad 0, lsnap tmis 0.
Designer: oracle NREC. WAIT: branch from main after the NCONV2A merge (NREC2, NCONV2A and this row edit nd_snap.c). Worktree build3ds/wt/NREC4, branch NREC4. PM_ROW=NREC4 PM_LANE=NREC4.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_snap.c, 3ds/include/nd_snap.h, 3ds/include/nd_api.h, 3ds/src/nd_ds.c (line 16, nd_ds_parts), 3ds/src/nd_rec.c (nd_parts_end, weak block), 3ds/src/nd_walk.c (nd_parts_run, nd_parts_run_v), 3ds/src/nd_geom.c (nd_part_draw), 3ds/tools/nd_ds_test.c. New files 3ds/tools/azt.d/NREC4.json, 3ds/tools/bench.d/NREC4.json.
## Facts (code read, main 27f33ea1da; find the quoted text, line numbers move)
- FACT nd_rec.c:402-416: nd_parts_end records b->u (u[1] = 0) with nd_rec_napi, then nd_ds_parts. nd_parts_run and nd_parts_run_v read u[0] and u + 2 only (nd_walk.c:463, :469, :510, :517): u[1] is free for the table address.
- FACT nd_rec.c:397: the call unit of instance k holds the NdGTab of a flat particle list (NDG_F_PARTS: only NDG_VTX and NDG_TRI ops to NDG_END, nd_gtab.h:44). nd_part_draw (nd_geom.c:371) visits each NDG_TRI op once. The fallback list run gives one draw_tri per NDG_TRI op (a TRI2 is 2 ops).
- FACT core 0 runs particles with pm_comb_vprim = 1 (nd_walk.c:468, :516). snap_build decides with vprim 0u (nd_snap.c:52, :55).
- FACT nd_ds_test.c:141-160 runs nd_ds_parts with nd_lsc_on 0 and call words 0xDEAD, 0xBEEF: the new code must read the table only when nd_lsc_on is 1.
## Edits
1. nd_snap.c: add `static unsigned sn_vprim; /* NREC4: 1 while a particle table is taken */` after `static NdTgt sn_tgt;`. In snap_build change `nd_pso_find_i(&r.in, 0u, &o)` and `nd_comb_core1(&r.in, 0u, sn_mode5, &o)` to `sn_vprim` in place of `0u`. After nd_lsc_begin add `void nd_lsc_begin_v(unsigned v) { nd_lsc_begin(); sn_vprim = v; } /* NREC4 */`. After nd_lsc_unit add `void nd_lsc_tris(unsigned n) { if (!n) return; nd_lsc_unit(ND_FS_W0(ND_FS_TRI1, 0)); ls_ord += n - 1u; } /* NREC4: n list triangles, one snap test */`. First statement of nd_lsc_end_at: add `sn_vprim = 0;`.
2. nd_snap.h: after `void nd_lsr_bst(void);` add ` void nd_lsc_tris(unsigned n);`. nd_api.h: after `void nd_lsc_begin(void);` add ` void nd_lsc_begin_v(unsigned vprim); /* NREC4 */`.
3. nd_ds.c:16: append `__attribute__((weak)) void nd_lsc_tris(unsigned n) { (void)n; } /* NREC4 */`. nd_ds_parts. Before: `for (unsigned j = 0; j < m; j++) if (j != pre) unit(s, p[2u * j], p[2u * j + 1u]);` After:
```c
        for (unsigned j = 0; j < m; j++) {
            if (j == pre) { if (nd_lsc_on) { unsigned t = 0; for (const NdGOp* o = ((const NdGTab*)(uintptr_t)p[2u * j + 1u])->op; o->k != NDG_END; o++) t += o->k == NDG_TRI; nd_lsc_tris(t); } continue; } /* NREC4: the list triangles of the instance */
            unit(s, p[2u * j], p[2u * j + 1u]);
            if (nd_lsc_on) nd_lsc_unit(p[2u * j]); /* NREC4 */
        }
```
4. nd_rec.c nd_parts_end. Before: `nd_ds_parts(&nd_ds, b->u, nd_ds_unit); /* NST2C: ... */` After: `nd_lsc_begin_v(1u); nd_ds_parts(&nd_ds, b->u, nd_ds_unit); nd_lsc_end_at(nd_rec_last_pay() + 1u); /* NST2C: ... NREC4: the instance snap table in payload word 1 */` (keep the old comment). In the weak block after the NREC2 stubs add `__attribute__((weak)) void nd_lsc_begin_v(unsigned v) { (void)v; } /* NREC4 */`.
5. nd_walk.c: above nd_parts_run add `static __attribute__((cold, noinline)) void pr_lsr_note(void) { static unsigned char seen; if (!seen) { seen = 1; pm_log_evt("lsnap parts f%u", pm_frame_count()); } } /* NREC4 */`. In nd_parts_run and in nd_parts_run_v, after `pm_walk_side++;` add `nd_lsr_begin((const uint32_t*)(uintptr_t)u[1]); if (u[1]) pr_lsr_note(); /* NREC4 */`. Before `pm_comb_vprim = 0;` (the line after the loop) add `nd_lsr_end(); /* NREC4 */`.
6. nd_geom.c nd_part_draw. Between `if (ep_osc != gv_epoch) { off_screen_check(); box_fold_sync(); ep_osc = gv_epoch; }` and `if (ep_epoch != gv_epoch) gv_epoch_enter();` add the snap steps of dlc_seg (nd_shape.c:434-439) with draw_tri's ordinal:
```c
            if (nd_ls_on) { nd_ls_ord++; nd_lsr_tri(nd_ls_ord - 1u); } /* NREC4: the instance snap */
            const unsigned char sn = nd_ls_on && nd_snap_arm;
            if (sn && ep_epoch == gv_epoch && nd_snap_bnew(nd_ep_bs)) gv_epoch++;
            nd_snap_rpend = nd_ls_on; if (sn) { nd_snapchk_tri(); if (ep_epoch == gv_epoch && nd_snap_rpend) { nd_snapchk_bind(); nd_snapchk_rnd(); } }
```
   After `if (ep_epoch != gv_epoch) gv_epoch_enter();` add `nd_snap_rpend = 0; if (nd_ls_on) nd_lsr_bst(); /* NREC4 */`.
7. nd_ds_test.c: after the NST2C (a) block add case NREC4 (declare `extern unsigned char nd_lsc_on;`): `nd_lsc_on = 1`, a static NdGTab with ops {NDG_VTX a 3}, {NDG_TRI 0 1 2}, {NDG_TRI 0 2 1}, {NDG_END}, a parts record of 2 instances m 3 {PRIM, CALL (that table), ENV}. Define in the test strong `nd_lsc_tris` (adds n to a sum) and `nd_lsc_unit` (counts calls). Expect sum 4 (`fault`: 5) and units 4. Then `nd_lsc_on = 0`. Print `nrec4 parts tris <s> units <u> bad <b>`; add b to bad.
8. azt.d/NREC4.json: scenario `fxp_snapchk`, a copy of fxp_drec (azt.d/PMOVE.json) with debug `warp=mac_01:0 pos=0:0:200 fxtest=28,56,61,69,90,112,132 loglevel=debug det snapchk`, until `lsnap f600 `, cap 30, deadline 25. bench.d/NREC4.json: bench `fxp_snapchk`, the form of fxops_snapchk (bench.d/NCONV1D1.json), until `lsnap f600 `. One commit for both.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-7. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nrec4_host.txt 2>&1`: PASS, `nrec4 parts tris 4 units 4 bad 0`. Commit `NREC4 1: particle runs take and consume an instance snap table`. Edit 8: commit `NREC4 2: fxp_snapchk scene and bench`.
2. `python 3ds/tools/bench.py base fxp_drec --commit $(git merge-base HEAD main) > build3ds/out/nrec4_base.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row NREC4 --bench fxp_drec --no-host --why "NREC4 particle snaps" > build3ds/out/nrec4_rowloop.txt 2>&1` (it builds).
3. `PM_ROW=NREC4 PM_LANE=NREC4 python 3ds/tools/bench.py run fxp_snapchk --row NREC4 --why "NREC4 lsnap" > build3ds/out/nrec4_run.txt 2>&1`. Then `grep -a "lsnap\|snapchk f\|snapchk bad\|partsrun\|ndc: stream" build3ds/azt/logs/fxp_snapchk.txt > build3ds/out/nrec4_lsnap.txt`.
Gate (exact): host PASS. fxp_drec byte-equal (rowloop exit 0). A `lsnap parts f` line. Last snapchk line: bad 0. Last lsnap line: tmis 0. Report lists, cmp, over, the partsrun line (data).
Stop: a DIFF, no `lsnap parts` line, bad or tmis above 0, a crash, a fatal line, a tool fault. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Fix rights: the files of this page, compile errors only. Pitfalls: one host test at a time. One bench per rowloop call. PM_CACHE=off only as a prefix of one command.
Hand-back: last commit `NREC4 3: gate. fxp drec equal; snapchk bad 0; lsnap tmis 0` deletes this page. One SubagentHandback: commit hash, the last lsnap and snapchk lines, the `lsnap parts` line.
