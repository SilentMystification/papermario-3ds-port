# ws_NDR2D (Haiku): nd_direct_bind takes the combine decision of the live snap; the core 0 pso decision (nd_pso_decide, dec_*) is deleted (stage S4, apply; counter: none of C1-C12)
Designer: oracle FIN. WAIT: branch from main after the NDR2B and NDR2C merges (every caller of nd_direct_bind records with nd_dir_rec: fill, trect, HUD rect, complex HUD, stencil),
and only when their hand-backs give snapchk `dir` bad 0. Worktree build3ds/wt/NDR2D, branch NDR2D. PM_ROW=NDR2D PM_LANE=NDR2D.
Edit only: 3ds/src/nd_direct.c, 3ds/include/nd_api.h, 3ds/src/nd_snap.c, 3ds/include/nd_snap.h, 3ds/tools/nd_direct_test.c.
## Facts (main 136c960247; find each line by its text)
- nd_direct_bind (nd_direct.c:97-142) decides with nd_pso_decide (:70-84: dec_tab, dec_make :45-56, dec_key :60-67, struct NdDec :35-41, DEC_N :42). dec_make's ok test (:51):
  `!(e->out.plan_on && (e->out.plan.uses_tex1 || e->out.plan.uses_noise)) && !e->out.pat`. comb_in_walker (:26) and pm_comb_vprim (:32) are read only by dec_make and dec_key.
- nd_api.h:71-72 declare NdDec and nd_pso_decide. nd_direct_test.c:122-145 tests the decision cache; its stub nd_comb_decide (:44-52) gives the test decisions.
- After NDR2A-C every nd_direct_bind caller records a snap right before its record, and a fallback walk drops it (nd_dir_drop). nd_snap_live is 1 at each nd_direct_bind (GUESS: the gate proves it).
## Edits (keep CRLF: the Edit tool)
1. nd_snap.c core 0: `const NdCombOut* nd_snap_dir_o(void) { return nd_snap_live ? &sc_cur.o : NULL; } /* NDR2D: the decision of the live snap */`. Delete nd_snapchk_dir, `sc_dir` and the ` dir %u` part of the tick line.
   nd_snap.h: declare nd_snap_dir_o; delete the nd_snapchk_dir declaration.
2. nd_direct.c nd_direct_bind. Before: `const unsigned char sn = nd_snap_arm; nd_snap_arm = 0; ...`, `const NdDec* d;`, `d = nd_pso_decide(id);`, `if (!d) return 0; ...`, `if (sn) nd_snapchk_dir(&d->out); ...`, `const NdCombOut* o = &d->out;`.
   After: `nd_snap_arm = 0; /* NDR2A: the arm of this record */` and, in place of the decide lines,
   ```
   const NdCombOut* o = nd_snap_dir_o(); /* NDR2D: the core 1 decision (nd_snap_pso) */
   if (!o || (o->plan_on && (o->plan.uses_tex1 || o->plan.uses_noise)) || o->pat) return 0; /* the ok test of the former dec_make */
   ```
   The block `{ /* what sync_combine does with the decision (scm_replay) */ ... }` keeps its body; its first line `const NdCombOut* o = &d->out;` goes (o is above). The rest of nd_direct_bind stays.
3. nd_direct.c: delete struct NdDec, DEC_N, dec_tab, dec_make, dec_key, nd_pso_decide with their comments, the comb_in_walker prototype (:26) and `extern int pm_comb_vprim;` (:32). nd_api.h: delete :71-72.
4. nd_direct_test.c: delete the stub nd_snapchk_dir; add `static int snap_none; static NdCombOut snap_o;` and
   `const NdCombOut* nd_snap_dir_o(void) { NdCombIn in; if (snap_none) return NULL; comb_in_walker(&in); nd_comb_decide(&in, &snap_o); return &snap_o; } /* NDR2D: the decision core 1 would snap */`.
   Delete `const NdDec *da, *da2, *db;` and the lines from `/* the pso cache:` to `CHECK(nd_pso_decide(it1) == NULL);` but keep `it1 = nd_pso_n64(&t1);`. After the `nd_direct_bind(it1, ...) == 0` check add
   `snap_none = 1; CHECK(nd_direct_bind(ia, 1u, 2u) == 0); snap_none = 0; /* NDR2D: no live snap: the record walks */`.
## Steps (output to build3ds/out/, read the last 20 lines; 1 build, 7 Azahar starts)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/ndr2d_host.txt 2>&1`: PASS. `grep -c "nd_pso_decide\|dec_make\|NdDec" 3ds/src/nd_direct.c 3ds/include/nd_api.h 3ds/tools/nd_direct_test.c`: 0 each.
   Commit `NDR2D 1: direct binds take the snap decision; nd_pso_decide deleted`.
2. One call each: `python 3ds/tools/bench.py base <b>` and `sh 3ds/tools/rowloop.sh --row NDR2D --bench <b> --no-host` for b = wscmb_pause_drec, axns_sam2_drec, curt_title_drec, wscmb_fsel_drec, sten_enter_det.
3. One call each: `python 3ds/tools/bench.py run <s> --row NDR2D --why "NDR2D snapchk"` for s = nimgm_strip_snapchk, wscmb_pause_snapchk. `grep -a snapchk` of both logs > build3ds/out/ndr2d_snapchk.txt.
Gate (exact): host PASS; 5 benches byte-equal; snapchk bad 0; wst not above the NDR2C hand-back (a record that walks instead of its direct draw changes drec).
Stop: a DIFF, a crash, a bad line, a tool fault: commit the outputs, send the paths to the oracle with SendMessage, wait. Fix rights: the listed files; compile errors.
Pitfalls: one host test at a time in a worktree (rowloop takes --no-host). PM_CACHE=off only as a prefix of one command, never exported. One bench per rowloop call. Each scene in its own call, no timeout option.
Keep each file's line endings (CRLF). A snap record is about 640 bytes (GUESS), the stream has 16 chunks of 32 KB per context (FACT nd_rec.c:30-31): the memo must stop a snap when the inputs stay the same.
Hand-back: last commit `NDR2D 2: gate. 5 benches byte-equal; snapchk bad <2>` deletes this page. One SubagentHandback: commit hash, gate values.
