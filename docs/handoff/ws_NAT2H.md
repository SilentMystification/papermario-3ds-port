# ws_NAT2H (Haiku): a stream fill rectangle takes a rect snap on core 1, and core 0 draws it with the snap combine and render state. Counter: c0gate unchanged, expected `c0gate calls 126 interp 37 state 23`. c0diff: the nd_nr_run cases fall.
Designer: oracle NATIVE. WAIT: start from main after `Merge NAT2D` and `Merge NAT1E` (the c0dcase lines). Worktree build3ds/wt/NAT2H, branch NAT2H. PM_ROW=NAT2H PM_LANE=NAT2H.
Edit only: 3ds/src/nd_rec.c (1 new line after line 408), 3ds/src/nd_rect.c (nd_frect_draw, lines 337-338). No edit in nd_walk.c, nd_set.c, nd_comb.c, nd_direct.c.
## Facts (main 04d851951c, NAT1E 5e42d76427)
- FACT NAT1E: every c0dcase has the walker decision of a stream state run (wk nd_nr_run), plan bytes equal. Kind 1: the first draw of a frame (spot_mac01 w 000 s 002, fxparts f412, f456, f500 w 000 s 102).
  Kind 2: dk op 5 (NDC_SPR), w 102 s 003. So the walker decided for a draw that had no fresh snap, and sc_cur held an older snap.
- FACT a stream fill rectangle is an ND_FS_RECT unit without ND_FS_RECT_TEX (nd_rec_nr nd_rec.c:405, run by nr_ctl nd_walk.c:301 -> nd_frect_draw nd_rect.c:316). Core 1 takes no snap for it (NAT2G covers TRI units only).
  nd_frect_draw uses the walker state: nd_rnd_sync for a 1-cycle fill (nd_rect.c:338) and the GPU combine of the last decision.
- FACT a texture rectangle already does this: core 1 nd_snap_rect at nd_rec_texrect (nd_rec.c:264), core 0 nd_snap_comb and nd_snap_render through rc_sn (nd_rect.c:198, :244-245).
- FACT a snap record arms nd_snap_arm and nd_snap_live (nd_snapchk_run nd_snap.c:119). A stream state run (runner nd_nr_run) keeps them (stream_run nd_rec.c:571, the disarm rule). With no new record, the memo says the inputs equal the live snap.
## Edits (new lines end with `/* NAT2H */`)
1. nd_rec.c, a new line after line 408 (the NAT2G line):
   `    if ((w0 >> 24) == ND_FS_RECT && !(w1 & ND_FS_RECT_TEX)) nd_snap_rect(); /* NAT2H: a fill rectangle takes a snap, as a texture rectangle (nd_rec_texrect :264) */`
2. nd_rect.c nd_frect_draw, lines 337-338 before:
   `    nd_img_check();` / `    if (((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) != 3) nd_rnd_sync(); /* PT71: ... */`
   after (the PT71 comment stays at the end of the second line):
   `    nd_img_check();`
   `    const unsigned char fsn = nd_snap_live; if (fsn) { nd_snap_arm = 0; nd_snap_comb(); } /* NAT2H: the combine of the fill snap */`
   `    if (((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) != 3) { if (fsn) nd_snap_render(); else nd_rnd_sync(); } /* PT71: ... */`
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 4 Azahar starts of 8)
1. Edits 1-2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat2h_host.txt 2>&1`: PASS (the 3 CACHECHECK lines are known). Commit `NAT2H 1: fill rectangles take a snap`.
2. For B in spot_mac01_drec, fxparts_drec: `PM_AZT_BASE_SLOT=2 python 3ds/tools/bench.py base <B> > build3ds/out/nat2h_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT2H --bench spot_mac01_drec,fxparts_drec --no-host --why "NAT2H: fill rectangle snaps" > build3ds/out/nat2h_loop.txt 2>&1`.
4. `grep -H "c0src f\|c0diff\|c0dwho\|c0dcase\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/spot_mac01_drec.txt build3ds/azt/logs/fxparts_drec.txt build3ds/out/loop_NAT2H_cmp_*.txt > build3ds/out/nat2h_c0src.txt`.
5. `python 3ds/tools/c0gate.py build3ds/lane-NAT2H/pm_3ds.elf > build3ds/out/nat2h_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (2 drec equal). c0gate `calls 126 interp 37 state 23`. Report the c0diff wdiff of each bench and every c0dcase line (not judged).
Fix rights: compile errors. A DIFF or a tool fault: stop, send build3ds/out/nat2h_c0src.txt to oracle NATIVE with SendMessage, and wait.
## Hand-back
Last commit `NAT2H 2: gate. 2 drec equal; c0diff <spot> <fxparts>` with `git add -f build3ds/out/nat2h_c0src.txt build3ds/out/nat2h_loop.txt`. It deletes this page. One SubagentHandback: hash, the c0diff and c0dcase lines.
