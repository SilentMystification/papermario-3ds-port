# ws_NAT2A (Haiku): a direct draw takes the render state of its live snap, not the walker render decision. Counter: c0gate calls -1, expected `c0gate calls 126 interp 37 state 23`. c0src fxparts wrnd falls (key hud_rect_run).
Designer: oracle NATIVE. WAIT: start from main after `Merge NAT1B` (NAT1B edits nd_direct.c lines 69, 86-88, 94-101: the same hunk). Worktree build3ds/wt/NAT2A, branch NAT2A. PM_ROW=NAT2A PM_LANE=NAT2A. Wave with NAT2G (no shared file).
Edit only: 3ds/src/nd_direct.c (one line in nd_direct_bind), 3ds/tools/nd_snap_link_stub.c (line 5), 3ds/tools/nd_direct_test.c (line 142 and 1 new line). No edit in nd_walk.c, nd_set.c, nd_comb.c.
## Facts (main 9cd355b1af, NAT1B tip 7c774f4a74)
- FACT NAT1B c0who fxparts: hud_rect_run 133 drec draws carry a walker decision. hud_rect_run (src/hud_element.c:552) calls hud_rect_direct, and that calls nd_direct_bind (:531).
- FACT nd_direct_bind takes the core 1 combine decision of the live snap (nd_direct.c:65 nd_snap_dir_o, :67-85), then `nd_sr_forget(); nd_rnd_sync();` (:87-88): the render state is a walker decision on nd_ws.
- FACT the live snap of a direct record is built on core 1 from the same pso words (nd_snap_pso nd_snap.c:79-86, the copy of nd_direct_bind :52-64). snap_build computes its render state and mix as nd_rnd_sync does (nd_snap.c:48-49, NDR0F1).
  nd_snap_render (nd_snap.c:129) applies them. nd_direct_bind returns 0 before :87 when no live snap exists (:66), so sc_cur is the live snap at :88.
- FACT host: nd_direct_test.c includes nd_direct.c. Its stub nd_rnd_sync counts n_render (nd_direct_test.c:54), and line 142 checks `n_render >= 1`. nd_snap_link_stub.c:5 is the empty nd_snap_render stub.
## Edits (new and changed lines end with `/* NAT2A */`. Find each line by its text)
1. nd_direct.c nd_direct_bind: the line `    nd_rnd_sync();` that follows `    nd_sr_forget(); /* nd_comb_sync of the stream path ...` becomes
   `    nd_snap_render(); /* NAT2A: the render state of the live snap (core 1, nd_snap_pso) */`. Keep the nd_sr_forget line (it forces the apply).
2. nd_snap_link_stub.c line 5 before: `void nd_snap_render(void) {}` after: `unsigned nd_snap_render_n; void nd_snap_render(void) { nd_snap_render_n++; } /* NAT2A: nd_direct_test.c counts it */`
3. nd_direct_test.c: a new line after line 54 (`void nd_rnd_sync(void) { n_render++; }`): `extern unsigned nd_snap_render_n; /* NAT2A */`.
   The CHECK line (main line 142): replace `n_render >= 1` with `nd_snap_render_n >= 1` (the rest of the line stays).
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 4 Azahar starts of 8)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat2a_host.txt 2>&1`: PASS (the 3 CACHECHECK lines are known). Commit `NAT2A 1: direct draws take the snap render state`.
2. For B in fxparts_drec, wscmb_pause_drec, one call each: `python 3ds/tools/bench.py base <B> > build3ds/out/nat2a_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT2A --bench fxparts_drec,wscmb_pause_drec --no-host --why "NAT2A: direct render from the snap" > build3ds/out/nat2a_loop.txt 2>&1`.
4. `grep -H "c0src f\|c0who\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/fxparts_drec.txt build3ds/azt/logs/wscmb_pause_drec.txt build3ds/out/loop_NAT2A_cmp_*.txt > build3ds/out/nat2a_c0src.txt`.
5. `python 3ds/tools/c0gate.py build3ds/lane-NAT2A/pm_3ds.elf > build3ds/out/nat2a_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (the 2 drec cmp equal). c0gate last line `c0gate calls 126 interp 37 state 23`. nat2a_c0src.txt has 2 c0src lines.
Fix rights: compile errors in the edited files. A host check other than line 142, a DIFF or a tool fault: stop, send the output path to oracle NATIVE with SendMessage, and wait.
## Hand-back
Last commit `NAT2A 2: gate. 2 drec equal; c0gate calls 126; c0src lines` with `git add -f build3ds/out/nat2a_c0src.txt build3ds/out/nat2a_loop.txt`. It deletes this page.
One SubagentHandback: the commit hash, the c0gate last line, the 2 c0src lines, the c0who lines.
