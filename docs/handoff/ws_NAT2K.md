# ws_NAT2K (Haiku): model and sprite draws take the combine decision words of their own core 1 snap (plan_on, mode5, vtx, vtxname, uses_tex1), not the walker's last decision. Counter: c0gate unchanged, expected `c0gate calls 126 interp 37 state 23`. Gate: drec byte-equal.
Designer: oracle NATIVE. Branch from main (faa8b534f3 or later). Worktree build3ds/wt/NAT2K, branch NAT2K. PM_ROW=NAT2K PM_LANE=NAT2K. NAT3A (held, branch tip 24aac54bd1) waits for this row.
Edit only: 3ds/include/nd_msnap.h, nd_msnapw.h, 3ds/src/nd_msnap.c, 3ds/src/nd_snap.c (1 new function after line 67), 3ds/src/nd_recs.c (1 new line before main line 244), 3ds/src/pm_gpu.c (before line 4617). No edit in nd_walk.c, nd_set.c, nd_comb.c, nd_direct.c.
## Facts (main faa8b534f3)
- FACT NAT3A DIFF: drec register 0x1204 (cmul word 3) 1.0 to 0.0 in all 3 scenes. Model draws take cmul from pm_gbi_gv_at (nd_model_set.c:38, pm_gpu.c:3938 path), sprites too (pm_gpu.c:4617).
  pm_gbi_gv_at reads the walker globals plan_active, comb_mode5, cur_plan.vtx, cur_plan.vtxname (nd_geom.c:101-125). dual_bind reads plan_active and cur_plan.uses_tex1 (nd_comb.c:410-413, via pm_gbi_bind_tile1 nd_recs.c:244).
  The model decision passes the global comb_mode5 as its history (nd_comb_pick nd_comb.c:261).
- FACT in main the walker decision (nd_comb_sync) sets these globals from the units of the record (rec_apply nd_recs.c:241). NAT3A removed it, so the model and sprite draws read the last snap of another draw.
- FACT c0diff counts no model draw (c0_count skips ndt_mdl and native sprites, pm_gpu.c:1332), so NAT1D-NAT1F could not see this.
- FACT core 1 builds the model snap after nd_ds_model applied the record (nd_rec.c:224-225), so a core 1 decision there sees the walker's input state. sn_last holds the newest snap decision (nd_snap.c:63).
## Edits (new lines end with `/* NAT2K */`)
1. nd_msnap.h NdMSnap: after the NAT2C line add `    uint32_t d_plan, d_mode5, d_vtx, d_vtxname, d_t1, d_pad; /* NAT2K: core 1 combine decision words (plan_on, mode5, plan vtx, vtxname, uses_tex1) */`.
   Add to nd_msnapw.h (nd_recs.c:14 includes it) the prototype `void nd_msnap_dec(const uint32_t* q); /* NAT2K: the decision words of the record's snap into the walker globals */`.
2. nd_snap.c, a new line after line 67 (snap_take): `const NdCombOut* nd_snap_now(void) { NdSnapChk t; (void)snap_build(&nd_ds, 0, &t); sn_cmds = ~0u; return &sn_last.o; } /* NAT2K: the decision of nd_ds now; the next snap_take records again */`
3. nd_msnap.c: a new line after the includes: `extern int plan_active, comb_mode5; extern PmPlan cur_plan; const NdCombOut* nd_snap_now(void); /* NAT2K */`.
   In nd_msnap_rec, after `    nd_msnap_take(&nd_ds, m);` add:
   `    { const NdCombOut* o = nd_snap_now(); m->d_plan = (uint32_t)o->plan_on; m->d_mode5 = (uint32_t)o->mode5; m->d_vtx = o->plan.vtx; m->d_vtxname = o->plan.vtxname; m->d_t1 = o->plan.uses_tex1; } /* NAT2K */`
   In nd_msnap_use, the nos line (`if (!s) { ms_nos++; ...`): before its closing `}` add `ms_wk.d_plan = (uint32_t)plan_active; ms_wk.d_mode5 = (uint32_t)comb_mode5; ms_wk.d_vtx = cur_plan.vtx; ms_wk.d_vtxname = cur_plan.vtxname; ms_wk.d_t1 = cur_plan.uses_tex1;`.
   After nd_msnap_key add: `void nd_msnap_dec(const uint32_t* q) { const NdMSnap* s = ms_of(q); if (!s) return; plan_active = (int)s->d_plan; comb_mode5 = (int)s->d_mode5; cur_plan.vtx = s->d_vtx; cur_plan.vtxname = (unsigned char)s->d_vtxname; cur_plan.uses_tex1 = (unsigned char)s->d_t1; } /* NAT2K */`
4. nd_recs.c: a new line before main line 244 (`        const uint32_t* qw = (const uint32_t*)q; uint32_t cw_[4]; ...`), inside the MDL-1b block: `        nd_msnap_dec((const uint32_t*)q); /* NAT2K: the decision words of this record (the walker set them in rec_apply) */`. (Its prototype is in nd_msnapw.h, edit 1.)
5. pm_gpu.c, a new line before line 4617 (`    pm_gbi_gv_at(mode.has_tx ? mode.tw1 : sw_tss, ...`):
   `    { const NdCombOut* so = nd_snap_dir_o(); if (so) { plan_active = so->plan_on; comb_mode5 = so->mode5; cur_plan.vtx = so->plan.vtx; cur_plan.vtxname = so->plan.vtxname; cur_plan.uses_tex1 = so->plan.uses_tex1; } } /* NAT2K: the sprite snap decision */`
   (nd_snap_dir_o: nd_snap.c, the live snap or NULL. Add its prototype, and the plan_active, comb_mode5, cur_plan externs, when pm_gpu.c lacks them.)
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 6 Azahar starts of 8)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat2k_host.txt 2>&1`: PASS (nrec1a too). Commit `NAT2K 1: model and sprite decision words from core 1`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat2k_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT2K --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec --no-host --why "NAT2K: model decision words" > build3ds/out/nat2k_loop.txt 2>&1`.
4. `grep -H "c0src f\|c0diff\|msnap nos\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT2K_cmp_*.txt > build3ds/out/nat2k_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT2K/pm_3ds.elf > build3ds/out/nat2k_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (3 drec equal). c0gate `calls 126 interp 37 state 23`. Fix rights: compile errors, missing externs or includes. A DIFF or a tool fault: stop, send nat2k_c0src.txt to oracle NATIVE with SendMessage, wait.
## Hand-back
Last commit `NAT2K 2: gate. 3 drec equal; model and sprite decision words from core 1` with `git add -f build3ds/out/nat2k_c0src.txt build3ds/out/nat2k_loop.txt`. It deletes this page. One SubagentHandback: hash, the c0src lines.
