# ws_NAT4D (Haiku): core 0 runs no combine-class unit of a stream state run: the mark cuts, the next snap adopt sets the fields and the GPU calls (as ls_run does for lists). Counter: c0gate unchanged, expected `c0gate calls 122 interp 34 state 23`. Gate: drec byte-equal against main after NAT4C.
Designer: oracle NATIVE. WAIT: `Merge NAT4C`. Branch from main after it. Worktree build3ds/wt/NAT4D, branch NAT4D. PM_ROW=NAT4D PM_LANE=NAT4D.
Edit only: 3ds/src/nd_walk.c (nr_set, the NAT4C block), 3ds/src/nd_snap.c (sc_adopt line 176), 3ds/src/nd_rect.c (nd_frect_draw lines 335-338). No other row edits these files now.
## Facts (main 946f0a76ff and the NAT4C page)
- FACT after NAT4C a stream class unit (OML, PRIM, ENV, FOG, BLEND, RARE, COMB) cuts only on a core 1 mark, and runs its handler (nd_unit_apply nd_set.c:160) only when nd_unit_same says it changes a field.
- FACT the handler effects: nd_ws.omode_l, prim, prim_lod, envc, comb words and tst1, fogc, blendc, keyc, keys, conv_k4, use_t1, dual_on, box_prod, box_folded, comb_alpha_shade (nd_set.c:84-176, nd_comb.c:495-551),
  plus the GPU calls pm_gpu_set_prim, pm_gpu_set_env, pm_comb_set_k4 and the flush of a combine change.
- FACT sc_adopt (nd_snap.c:171-185) writes all those fields from the armed snap at each snap triangle (nd_snapchk_tri), and ls_gpu (:136-140) makes those GPU calls, today only when a list runs ls_run (lr_geo, :176).
  ls_gpu compares nd_ws with the snap first, so a call with equal fields does nothing.
- FACT the fill rectangle reads nd_ws.prim and nd_ws.fill for its colour (nd_rect.c:335-336) and takes the snap combine with no adopt (NAT2H, :338).
## Edits (new and changed lines end with `/* NAT4D */`)
1. nd_walk.c nr_set, the NAT4C block: `cut = ep_mark != 0; ep_mark = 0; if (!cut) return; }` becomes `if (ep_mark) gv_epoch++; ep_mark = 0; return; } /* NAT4D: a stream class unit is not run on core 0: the mark cuts, the next snap adopt sets its fields */`.
   Keep the epchk count lines before it. (nd_unit_same is still called first: keep that order, its result is now used only by the count.)
2. nd_snap.c line 176: `    if (lr_geo) ls_gpu(` becomes `    ls_gpu(` (the arguments stay; append ` /* NAT4D: also for stream snaps */`).
3. nd_rect.c nd_frect_draw: move the NAT2H line (`const unsigned char fsn = nd_snap_live; ...`) above line 335 (`unsigned color = ...`) and change its body to `if (fsn) nd_snapchk_tri();` (nd_snapchk_tri clears the arm, applies the combine and adopts the fields). The render line (339) stays.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8: one bench more than usual)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat4d_host.txt 2>&1`: PASS. Commit `NAT4D 1: stream class units not run on core 0`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat4d_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT4D --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT4D: stream class units" > build3ds/out/nat4d_loop.txt 2>&1`.
4. `grep -H "epchk\|c0src f\|msnap nos\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT4D_cmp_*.txt > build3ds/out/nat4d_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT4D/pm_3ds.elf > build3ds/out/nat4d_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (4 drec equal). c0gate `calls 122 interp 34 state 23`.
Fix rights: compile errors. A DIFF is not yours to judge (a reader of a skipped field before the next adopt, for example a model or sprite draw): stop, send nat4d_c0src.txt and the first DIFF line to oracle NATIVE with SendMessage, wait.
## Hand-back
Last commit `NAT4D 2: gate. 4 drec equal; stream class units off core 0` with `git add -f build3ds/out/nat4d_c0src.txt build3ds/out/nat4d_loop.txt`. It deletes this page. One SubagentHandback: hash, the c0src lines.
