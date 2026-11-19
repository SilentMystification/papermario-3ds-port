# ws_NAT2G (Haiku): every stream triangle unit takes a mesh snap on core 1, so core 0 draws it from a snap, not from a walker decision. Counter: c0gate unchanged, expected `c0gate calls 127 interp 37 state 23`. c0src: the wcomb share of key nd_nr_run falls.
Designer: oracle NATIVE. WAIT: start from main after `Merge NAT1B` (exact c0src). Worktree build3ds/wt/NAT2G, branch NAT2G. PM_ROW=NAT2G PM_LANE=NAT2G. Wave with NAT2A (no shared file).
Edit only: 3ds/src/nd_rec.c (nd_rec_nr only), 3ds/src/nd_geo.c (lines 40, 44), 3ds/src/nd_snap.c (2 lines at nd_lsc_end_at, main line 75). No edit in nd_walk.c, nd_set.c, nd_comb.c, nd_direct.c.
## Facts (main 9cd355b1af)
- FACT NAT1B c0who: the walker decision of 18 (spot_mac01), 18 (entcape_kmr03) and 80 (fxparts) drec draws came from stream state runs (runner nd_nr_run nd_walk.c:347, recorded by st_put nd_rec.c:370).
- FACT only nd_mesh_tri1 and nd_mesh_tri2 take a snap before a stream triangle (nd_geo.c:40, :44). Triangle units of other producers (nd_rec_list :520, nd_scomp.c rec_ops :139, nd_parts_add :342) reach nd_rec_nr (nd_rec.c:405) with no snap.
  So core 0 draws them with the walker decision (draw_tri nd_geom.c:353 applies a snap only when one is armed).
- FACT a snap record is a state op (nd_rec_st nd_rec.c:309): it splits the open state run in two (st_put :373 needs wp == st_end), and core 0 runs the two parts in order.
- FACT nd_mesh_tri1 records its triangle through nd_rec_nr (NDGEO_TRI1 on nd_nr, nd_geo.c:41). So after edit 1 the calls at nd_geo.c:40 and :44 are duplicates.
- FACT a list over LS_MAX (16 ranges) gets no snap table (nd_snap.c:75 `ls_ovl`). Edit 3 counts them for the NAT2D page.
## Edits (new lines end with `/* NAT2G */`)
1. nd_rec.c nd_rec_nr, a new line before line 408 `    nd_ds_unit(&nd_ds, w0, w1);`:
   `    if ((w0 >> 24) == ND_FS_TRI1 || (w0 >> 24) == ND_FS_TRI2) nd_snap_mesh(); /* NAT2G: every stream triangle has a snap, or the memo says the last one holds */`
2. nd_geo.c: delete line 40 `    nd_snap_mesh();` (in nd_mesh_tri1) and line 44 `    nd_snap_mesh();` (in nd_mesh_tri2). Keep the weak definition at line 36.
3. nd_snap.c, a new line above line 75 `void nd_lsc_end_at(uint32_t* dst) {`:
   `unsigned pm_frame_count(void); static __attribute__((cold, noinline)) void ls_over_note(void) { static unsigned n; if (n++ < 16u) pm_log_evt("lsover f%u ranges %u ord %u", pm_frame_count(), ls_n, ls_ord); } /* NAT2G */`
   Line 75: insert `if (ls_ovl) ls_over_note(); ` as the first statement of the body (before `nd_lsc_on = 0;`).
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 6 Azahar starts of 8)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat2g_host.txt 2>&1`: PASS (the 3 CACHECHECK lines are known, oracle_handoff.md:49). Commit `NAT2G 1: a snap for every stream triangle`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, one call each: `python 3ds/tools/bench.py base <B> > build3ds/out/nat2g_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT2G --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec --no-host --why "NAT2G: snap for every stream triangle" > build3ds/out/nat2g_loop.txt 2>&1`.
4. `grep -H "c0src f\|c0who\|lsover\|bindsnap\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/spot_mac01_drec.txt build3ds/azt/logs/entcape_kmr03_drec.txt build3ds/azt/logs/fxparts_drec.txt build3ds/out/loop_NAT2G_cmp_*.txt > build3ds/out/nat2g_c0src.txt`.
5. `python 3ds/tools/c0gate.py build3ds/lane-NAT2G/pm_3ds.elf > build3ds/out/nat2g_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (the 3 drec cmp equal). c0gate last line `c0gate calls 127 interp 37 state 23`. nat2g_c0src.txt has 3 c0src lines.
Fix rights: compile errors in the edited files. A DIFF is not yours to judge: stop, send build3ds/out/nat2g_c0src.txt (it holds the DIFF draw, the drec frame and bindsnap lines) to oracle NATIVE with SendMessage, and wait.
## Hand-back
Last commit `NAT2G 2: gate. 3 drec equal; c0src lines` with `git add -f build3ds/out/nat2g_c0src.txt build3ds/out/nat2g_loop.txt`. It deletes this page.
One SubagentHandback: the commit hash, the 3 c0src lines, the c0who lines, the lsover lines (or none).
