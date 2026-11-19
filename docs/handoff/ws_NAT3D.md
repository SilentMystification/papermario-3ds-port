# ws_NAT3D (Haiku): a triangle epoch with no snap keeps the render state of the last snap: the walker render decision of gv_epoch_enter goes. Counter: c0gate calls -1, expected `c0gate calls 125 interp 37 state 23` (122 after NAT3A). Gate: drec byte-equal.
Designer: oracle NATIVE. WAIT: `Merge NAT2D` and the NAT1E c0dcase data (FACT NAT1D: wdiff above 0 in all 3 scenes).
When one exists, oracle NATIVE rewrites this page before the start. Worktree build3ds/wt/NAT3D, branch NAT3D. PM_ROW=NAT3D PM_LANE=NAT3D. Wave W2b with NAT3A (no shared file).
Edit only: 3ds/src/nd_geom.c line 221. No edit in nd_walk.c, nd_set.c, nd_comb.c, nd_direct.c.
## Facts (main b485faae44)
- FACT gv_epoch_enter (nd_geom.c:208) sets the render state at the first triangle of an epoch: `if (nd_snap_rpend) nd_snap_render();` (:220), `else nd_rnd_sync();` (:221, the walker decision on nd_ws).
- FACT nd_snap_rpend is set for an armed snap or a list range (draw_tri nd_geom.c:352, nd_part_draw :405, dlc_seg nd_shape.c:436). With no new snap, the inputs equal those of the last snap (the memo, nd_snap.c:51), and the GPU keeps its render state.
- FACT NAT1D c0diff bit 1 counts the drec draws whose walker render decision differs from sc_cur. Zero means this edit keeps the bytes.
- The other nd_rnd_sync callers stay for later rows: nd_rect.c:198, :312, :338, nd_direct.c:169.
## Edit
nd_geom.c line 221 `        else nd_rnd_sync();`: delete the line. Line 220 stays: `        if (nd_snap_rpend) nd_snap_render(); /* NDR0F2: the snap of this triangle */`.
Add the comment ` NAT3D: no snap, the render state of the last snap stays` inside the line 220 comment.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 6 Azahar starts of 8)
1. The edit. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat3d_host.txt 2>&1`: PASS (the 3 CACHECHECK lines are known). Commit `NAT3D 1: no walker render decision at the triangle epoch`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat3d_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT3D --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec --no-host --why "NAT3D: no walker render at the epoch" > build3ds/out/nat3d_loop.txt 2>&1`.
4. `grep -H "c0src f\|c0diff\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/spot_mac01_drec.txt build3ds/azt/logs/entcape_kmr03_drec.txt build3ds/azt/logs/fxparts_drec.txt build3ds/out/loop_NAT3D_cmp_*.txt > build3ds/out/nat3d_c0src.txt`.
5. `python 3ds/tools/c0gate.py build3ds/lane-NAT3D/pm_3ds.elf > build3ds/out/nat3d_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (3 drec equal). c0gate: calls 125 (122 when the branch holds NAT3A), interp 37 or lower, state 23. Report the 3 c0src wrnd values.
Fix rights: compile errors. A DIFF or a tool fault: stop, send build3ds/out/nat3d_c0src.txt to oracle NATIVE with SendMessage, and wait.
## Hand-back
Last commit `NAT3D 2: gate. 3 drec equal; c0gate calls <c>` with `git add -f build3ds/out/nat3d_c0src.txt build3ds/out/nat3d_loop.txt`. It deletes this page. One SubagentHandback: hash, c0gate last line, the 3 c0src lines.
