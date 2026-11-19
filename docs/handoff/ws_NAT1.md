# ws_NAT1 (Haiku): count the drec draws whose state the core 0 walker set (the c0src line). Counter: none (the data of the W1 pages, plan_native.md section 2). Gate: drec byte-equal.
Designer: oracle NATIVE. Branch from main. Worktree build3ds/wt/NAT1, branch NAT1. PM_ROW=NAT1 PM_LANE=NAT1.
Edit only: 3ds/src/pm_gpu.c, nd_comb.c, nd_set.c, nd_snap.c, nd_tex_set.c, nd_direct.c, 3ds/include/nd_int.h, 3ds/tools/nd_snap_link_stub.c.
## Facts (main 30eaaef405)
- Every drec record comes from one call: drec_draw at pm_gpu.c:1349 in nd_native_submit. ndt_mdl (pm_gpu.c:1169) is 1 for a model draw.
- The combine goes to the GPU from the walker (sync_combine_work nd_comb.c:337, the memo scm_replay :181) or from a snap (nd_snap_comb nd_snap.c:173, nd_direct_bind nd_direct.c:67-85).
- The render state: nd_rnd_sync nd_set.c:233 (walker) or nd_snap_render nd_snap.c:126 (snap). The bind: nd_tile_bind nd_tex_set.c:443 reads the snap blocks when nd_bsrc is set (nd_snap.h:30), else nd_ws. nd_direct_tex nd_direct.c:92 binds a stated desc.
- Host checks grep exact lines: nd_comb.c:182 (nrec2b.sh L1), nd_comb.c:261 (hwfix1.sh), nd_direct.c:71 (nrec2b.sh L2). Do not change these lines. nd_direct_test.c includes nd_direct.c and links nd_snap_link_stub.c.
## Edits (each new line ends with `/* NAT1 */`. Line numbers are main lines, find the line by its text)
1. nd_int.h, a new line after line 171 `extern int sc_ok, gp_prev_op, box_folded;`: `extern unsigned char nd_c0w; /* NAT1: bit 0 combine, bit 1 render, bit 2 bind set by the walker (pm_gpu.c) */`
2. pm_gpu.c, a new line after line 597 (`static FILE* drec_f; ...`): `unsigned char nd_c0w; static unsigned c0_d, c0_m, c0_c, c0_r, c0_b; extern unsigned nd_snb, nd_snn, nd_snm; /* NAT1 */`
3. pm_gpu.c drec_end, a new line before the line `    pm_log_evt("drec f%u draws %u bytes %ld%s", ...` (main 626):
   `    pm_log_evt("c0src f%u draws %u mdl %u wcomb %u wrnd %u wbind %u snb %u snn %u snm %u", pm_frame_count(), c0_d, c0_m, c0_c, c0_r, c0_b, nd_snb, nd_snn, nd_snm); /* NAT1 */`
4. pm_gpu.c, above `static void nd_native_submit(const NdDraw* dp, int tex) {` (main 1322), these 3 lines:
   `static __attribute__((cold, noinline)) void c0_count(const NdDraw* d) { /* NAT1: one drec draw by the source of its state */`
   `    if (!drec_f) return; if (ndt_mdl) { c0_m++; return; }`
   `    c0_d++; c0_c += nd_c0w & 1u; c0_r += (nd_c0w >> 1) & 1u; if (d->u[0].dim) c0_b += (nd_c0w >> 2) & 1u; }`
   And a new line before the line `    if (__builtin_expect(drec_on, 0)) drec_draw(d, ndt_mdl || nb.gi_kind == 1); ...` (main 1349): `    if (__builtin_expect(drec_on, 0)) c0_count(d); /* NAT1 */`
5. nd_comb.c: a new line after `void scm_replay(const ScMemo* m) {` (main 181): `    nd_c0w |= 1u; /* NAT1 */`. A new line before the nd_comb_apply line of sync_combine_work (main 337): `    nd_c0w |= 1u; /* NAT1 */`.
6. nd_snap.c: a new line after line 27 (`static unsigned sn_vprim; ...`): `unsigned nd_snb, nd_snn, nd_snm; /* NAT1: core 1 snap_build calls, new snaps, table misses */`.
   In snap_build: a new line after line 42 `    memset(&o, 0, sizeof o);`: `    nd_snb++; /* NAT1 */`. A new line after line 49 (the memo `return 0;` line): `    nd_snn++; /* NAT1 */`.
   Line 53 before: `    else nd_comb_core1(&r.in, sn_vprim, sn_mode5, &o); /* NDR0D: ...` after: `    else { nd_snm++; nd_comb_core1(&r.in, sn_vprim, sn_mode5, &o); } /* NDR0D: ...` (the comment stays).
   nd_snap_render: a new line after line 128 `    nd_snap_rpend = 0; /* DEL3A: ...`: `    nd_c0w &= ~2u; /* NAT1 */`.
   Line 173 before: `void nd_snap_comb(void) { nd_comb_apply(&sc_cur.o, ...` after: `void nd_snap_comb(void) { nd_c0w &= ~1u; /* NAT1 */ nd_comb_apply(&sc_cur.o, ...` (rest unchanged).
7. nd_set.c nd_rnd_sync: a new line after line 234 (`    const unsigned sg = ...`): `    nd_c0w |= 2u; /* NAT1 */`.
8. nd_tex_set.c nd_tile_bind: a new line after line 443 `void nd_tile_bind(int n) {`: `    if (nd_bsrc) nd_c0w &= ~4u; else nd_c0w |= 4u; /* NAT1 */`.
9. nd_direct.c: a new line after line 68 `        unsigned cc = 0;`: `        nd_c0w &= ~1u; /* NAT1: the core 1 decision */`. A new line after line 92 `int nd_direct_tex(const NdBindDesc* d) {`: `    nd_c0w &= ~4u; /* NAT1: a stated bind */`.
10. nd_snap_link_stub.c, a new last line: `unsigned char nd_c0w; /* NAT1: host link stub (nd_direct.c) */`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 6 Azahar starts of 8)
1. Edits 1-10. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat1_host.txt 2>&1`: PASS as on main. Commit `NAT1 1: c0src counter`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, one call each: `python 3ds/tools/bench.py base <B> > build3ds/out/nat1_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT1 --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec --no-host --why "NAT1: c0src counter, drec equal" > build3ds/out/nat1_loop.txt 2>&1`.
4. `grep -h "c0src f" build3ds/azt/logs/spot_mac01_drec.txt build3ds/azt/logs/entcape_kmr03_drec.txt build3ds/azt/logs/fxparts_drec.txt > build3ds/out/nat1_c0src.txt`.
## Gate (exact)
Host PASS. Rowloop exit 0 (build rc 0, the 3 drec cmp equal). nat1_c0src.txt has 3 lines, each with draws above 0.
Fix rights: compile errors in the edited files, and a host link error that names nd_c0w (edit 10). A DIFF, a missing c0src line or a tool fault: stop, send the output path to oracle NATIVE with SendMessage.
## Hand-back
Last commit `NAT1 2: gate. 3 drec equal; c0src lines` with `git add -f build3ds/out/nat1_c0src.txt build3ds/out/nat1_loop.txt`. It deletes this page.
One SubagentHandback: the commit hash, the 3 c0src lines with their bench names.
