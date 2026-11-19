# ws_NAT2J (Haiku): every task stream starts with a fresh snap (the core 1 memo resets at a task start), and c0src stops counting native sprite draws. Counter: c0gate unchanged, expected `c0gate calls 126 interp 37 state 23`. c0diff: wdiff to 0.
Designer: oracle NATIVE. Branch from main after `Merge NAT2H`. Worktree build3ds/wt/NAT2J, branch NAT2J. PM_ROW=NAT2J PM_LANE=NAT2J. Runs beside NAT2C.
Edit only: 3ds/src/nd_snap.c (1 new line after line 25), 3ds/src/nd_rec.c (task_open main line 197 and 1 new line after line 549), 3ds/src/pm_gpu.c (lines 598, 1332, 4666).
Overlap with NAT2C: nd_rec.c lines 224 and 550 (this row: 197 and a new line after 549, so merge NAT2C first or put the weak line after NAT2C's), pm_gpu.c line 3867 (far). No edit in nd_walk.c, nd_set.c, nd_comb.c, nd_direct.c.
## Facts (main a5f5178446 after NAT2H)
- FACT NAT2H c0dcase kind 1: the first draw of a frame (f200, f201, f412, f456, f500), walker w 000, sc_cur s 002 or 102, in a stream state run. Kind 2: dk op 5 (NDC_SPR), w 102 s 003.
- FACT the core 1 snap memo spans tasks: snap_take skips the record when the inputs equal sn_last and no command came (nd_snap.c:51, sn_cmds). A state run is no command (st_put counts NDC_NRUN, nd_rec_cmds nd_rec.c:541).
  So the first draw of a new task stream may have no snap, and core 0 replays each task by itself (eye 2, key frame, nd_cmd.h:5-6). A task starts at task_open (nd_rec.c:196-198, task_head NULL).
- FACT kind 2 is the native sprite draw itself: pm_gpu_spr_submit (pm_gpu.c:4650-4668) calls nd_native_submit with its own NdDraw (pm_gpu_spr_build reads the snap words, :4627-4629), not the walker GPU combine. c0_count counts it as a walker draw because ndt_mdl is 0 there.
## Edits (new lines end with `/* NAT2J */`)
1. nd_snap.c, a new line after line 25 (`static unsigned sn_cmds = ~0u;`): `void nd_snap_task(void) { sn_cmds = ~0u; } /* NAT2J: a task stream starts with its own snap */`
2. nd_rec.c task_open (line 197) before: `    if (!task_head) { task_head = wp; sh_all(); }` after: `    if (!task_head) { task_head = wp; sh_all(); nd_snap_task(); } /* NAT2J */`.
   A new line above task_open: `void nd_snap_task(void); /* NAT2J: nd_snap.c */`. A new line after the weak nd_snap_mesh (line 549): `__attribute__((weak)) void nd_snap_task(void) {} /* NAT2J: host units link nd_rec.c without nd_snap.c */`
3. pm_gpu.c line 598: append ` static unsigned char c0_spr; /* NAT2J: a native sprite submit */` to the line. Line 1332: `if (ndt_mdl) { c0_m++; return; }` becomes `if (ndt_mdl || c0_spr) { c0_m++; return; }`.
   Line 4666 (`    nd_native_submit(d, tex);` in pm_gpu_spr_submit) becomes `    c0_spr = 1; nd_native_submit(d, tex); c0_spr = 0; /* NAT2J */`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 4 Azahar starts of 8)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat2j_host.txt 2>&1`: PASS (the 3 CACHECHECK lines are known). Commit `NAT2J 1: a fresh snap per task, sprite draws not counted`.
2. For B in spot_mac01_drec, fxparts_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat2j_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT2J --bench spot_mac01_drec,fxparts_drec --no-host --why "NAT2J: snap per task" > build3ds/out/nat2j_loop.txt 2>&1`.
4. `grep -H "c0src f\|c0diff\|c0dwho\|c0dcase\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/spot_mac01_drec.txt build3ds/azt/logs/fxparts_drec.txt build3ds/out/loop_NAT2J_cmp_*.txt > build3ds/out/nat2j_c0src.txt`.
5. `python 3ds/tools/c0gate.py build3ds/lane-NAT2J/pm_3ds.elf > build3ds/out/nat2j_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (2 drec equal). c0gate `calls 126 interp 37 state 23`. Report each c0diff wdiff (expected 0) and every c0dcase line (not judged).
Fix rights: compile errors. A DIFF or a tool fault: stop, send nat2j_c0src.txt to oracle NATIVE with SendMessage, and wait.
## Hand-back
Last commit `NAT2J 2: gate. 2 drec equal; c0diff <spot> <fxparts>` with `git add -f build3ds/out/nat2j_c0src.txt build3ds/out/nat2j_loop.txt`. It deletes this page. One SubagentHandback: hash, the c0diff and c0dcase lines.
