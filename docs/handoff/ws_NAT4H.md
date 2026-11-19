# ws_NAT4H (Haiku): the fill colour and the primitive depth come from the snaps; core 0 runs no FILL or PRIMZ stream unit. Counter: c0gate unchanged, expected `c0gate calls 122 interp 34 state 23`. Gate: drec byte-equal.
Designer: oracle NATIVE. WAIT: `Merge NAT4G`. Branch from main after it. Worktree build3ds/wt/NAT4H, branch NAT4H. PM_ROW=NAT4H PM_LANE=NAT4H.
Edit only: 3ds/include/nd_snap.h, 3ds/include/nd_msnap.h, 3ds/src/nd_snap.c, 3ds/src/nd_msnap.c, 3ds/src/nd_walk.c (nr_set class line), 3ds/src/pm_gpu.c (the NAT2K sprite block, main line 4618).
## Facts (main after NAT4G)
- FACT readers of nd_ws.fill: nd_rect.c:325 (depth clear), :337 (fill colour). Readers of nd_ws.prim_z: pm_gbi_gv_at (nd_geom.c:112: triangle, model nd_model_set.c:38 and sprite pm_gpu.c:4617 draws), nd_rect.c:208, :313, pm_gbi_gvkey (nd_comb.c:279).
- FACT a fill rectangle and a texture rectangle adopt their snap (NAT2H, NAT4D: nd_snapchk_tri -> sc_adopt nd_snap.c:171-185); triangles adopt at nd_snapchk_tri; model draws take nd_msnap_dec (NAT2K); sprites take the NAT2K block.
- FACT snap_build skips a new snap when its compared inputs equal sn_last (nd_snap.c:52): fill and prim_z must join that compare, or a FILL change gets no snap.
- FACT NdSnapChk ends with `uint32_t pad3;` (nd_snap.h:22) and NdMSnap has `d_pad` (nd_msnap.h:12): both sizes stay multiples of 8 with the edits below.
## Edits (new and changed lines end with `/* NAT4H */`)
1. nd_snap.h NdSnapChk: after `uint32_t pad3; ...` add `    uint32_t fill, primz; /* NAT4H: nd_ws.fill and prim_z of core 1 */`.
2. nd_snap.c snap_build: after the `r.mixk = nd_snap_mix(...)` line (49) add `    r.fill = s->w.fill; r.primz = s->w.prim_z; /* NAT4H */`. In the memo test (line 52) add ` && r.fill == sn_last.fill && r.primz == sn_last.primz` before `) return 0;`.
   sc_adopt, after line 182 (`nd_ws.omode_l = sc_cur.oml; fogc = ...`) add `    nd_ws.fill = sc_cur.fill; nd_ws.prim_z = sc_cur.primz; /* NAT4H */`.
   At the end of nd_snap.c add `unsigned nd_snap_primz(void) { return sc_cur.primz; } /* NAT4H: the sprite path */`.
3. nd_msnap.h: rename `d_pad` to `d_primz` (comment: NAT4H prim_z of core 1 after the record). nd_msnap.c: in nd_msnap_rec after the NAT2K line add `    m->d_primz = nd_ds.w.prim_z; /* NAT4H */`. In the nos branch add `ms_wk.d_primz = nd_ws.prim_z;`. In nd_msnap_dec add `nd_ws.prim_z = s->d_primz;`.
4. pm_gpu.c, the NAT2K sprite block (main line 4618): inside `if (so) { ... }` add `nd_ws.prim_z = nd_snap_primz();` and its prototype `unsigned nd_snap_primz(void);` in the block's extern list.
5. nd_walk.c nr_set, the class line: the combine class `ca_` gets `|| i_ == ND_FS_FILL || i_ == ND_FS_PRIMZ` (they are no longer run on core 0 in a stream run; the mark still cuts).
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat4h_host.txt 2>&1`: PASS (nrec1a too). Commit `NAT4H 1: fill and prim_z from the snaps`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat4h_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT4H --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT4H: fill and prim_z from snaps" > build3ds/out/nat4h_loop.txt 2>&1`.
4. `grep -H "c0src f\|msnap nos\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT4H_cmp_*.txt > build3ds/out/nat4h_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT4H/pm_3ds.elf > build3ds/out/nat4h_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (4 drec equal). c0gate `calls 122 interp 34 state 23`. Fix rights: compile errors, missing prototypes. A DIFF or a tool fault: stop, send nat4h_c0src.txt and the first DIFF line to oracle NATIVE with SendMessage, wait.
## Hand-back
Last commit `NAT4H 2: gate. 4 drec equal; fill and prim_z from snaps` with `git add -f build3ds/out/nat4h_c0src.txt build3ds/out/nat4h_loop.txt`. It deletes this page. One SubagentHandback: hash, the c0src lines.
