# ws_NAT1F (Haiku): c0diff compares every combine output that a draw reads (mode5, pat, cc, env_on too). Counter: c0gate unchanged, expected `c0gate calls 126 interp 37 state 23`. Gate: drec byte-equal.
Designer: oracle NATIVE. Branch from main (5ed280011f or later, after `Merge NAT2J`). Worktree build3ds/wt/NAT1F, branch NAT1F. PM_ROW=NAT1F PM_LANE=NAT1F. NAT3A holds until this data.
Edit only: 3ds/src/nd_snap.c (nd_snap_cdiff, main lines 243-247), 3ds/src/nd_comb.c (lines 181, 191, 341), 3ds/src/pm_gpu.c (the c0dcase line in c0_dwho). No edit in nd_walk.c, nd_set.c, nd_direct.c.
Line overlap: nd_comb.c lines 181, 191, 341 are the NAT1D lines. NAT3A (on hold, not merged) edits nd_comb.c lines 342-361. Merge this row first.
## Facts (main after NAT2J)
- FACT NAT3A drec DIFF (all 3 scenes): draw 11 or 8, register 0x1204 (cmul word 3, the alpha multiplier: drec_draw pm_gpu.c:660-661) 1.0 to 0.0.
- FACT cmul comes from nd_gv_params (nd_model.c:24-55): cm[3] is 0 on the env path (comb5 && !plan_active), the plan path or the a255 path, else 1. Its inputs are comb_mode5, plan_active, cur_plan.vtxname, comb_alpha_shade (pm_gbi_gv_at nd_geom.c:101-125).
- FACT nd_snap_cdiff (nd_snap.c:244) compares plan_on, leg and the plan bytes only. comb_mode5 and box_pat are not compared. nd_comb_apply writes them (nd_comb.c:317), and the decision carries mode5 from the call before when leg 4 is chosen (nd_decide.c:230-231).
  So NAT1D wdiff 0 did not cover mode5 and pat. GUESS: the NAT3A DIFF is a walker mode5 that differs from the snap's.
## Edits (new and changed lines end with `/* NAT1F */`)
1. nd_snap.c nd_snap_cdiff: the signature becomes `int nd_snap_cdiff(int plan_on, int leg, const PmPlan* plan, int mode5, int pat, int cc_on, unsigned cc, int env_on)`.
   Add the word `unsigned nd_cd_x;` to line 243 (`nd_cd_x`: bits walker mode5 0, pat 1, cc_on 2, env_on 3, and the same snap bits at 8-11).
   Body: after the nd_cd_off line add `nd_cd_x = (unsigned)(mode5 != 0) | (unsigned)(pat != 0) << 1 | (unsigned)(cc_on != 0) << 2 | (unsigned)(env_on != 0) << 3 | (unsigned)(s->mode5 != 0) << 8 | (unsigned)(s->pat != 0) << 9 | (unsigned)(s->cc_on != 0) << 10 | (unsigned)(s->env_on != 0) << 11;`
   The return becomes `return s->plan_on != plan_on || (plan_on ? nd_cd_off != 0xffffu : s->leg != leg) || s->mode5 != mode5 || s->pat != pat || s->cc_on != cc_on || (cc_on && s->cc != cc) || s->env_on != env_on;`
2. nd_comb.c line 181: the prototype gets the same 4 new parameters. Line 191 call: `nd_snap_cdiff(m->plan_on, m->leg, &m->plan, m->mode5, m->pat, m->cc_on, m->cc, m->env_on)`.
   Line 341 call: `nd_snap_cdiff(o.plan_on, o.leg, &o.plan, o.mode5, o.pat, o.cc_on, o.cc, o.env_on)`. (ScMemo fields: nd_comb.c:45. NdCombOut fields: nd.h:481-485.)
3. pm_gpu.c c0_dwho, the c0dcase line: add `nd_cd_x` to its extern list and ` x %03x` with `nd_cd_x` after `s %03x` in the format and the arguments.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 6 Azahar starts of 8)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat1f_host.txt 2>&1`: PASS (hwfix1.sh, nrec2b.sh lines stay). Commit `NAT1F 1: c0diff with mode5, pat, cc, env`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat1f_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT1F --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec --no-host --why "NAT1F: full c0diff" > build3ds/out/nat1f_loop.txt 2>&1`.
4. `grep -H "c0src f\|c0diff\|c0dwho\|c0dcase\|drec frame" build3ds/azt/logs/spot_mac01_drec.txt build3ds/azt/logs/entcape_kmr03_drec.txt build3ds/azt/logs/fxparts_drec.txt > build3ds/out/nat1f_c0src.txt`.
5. `python 3ds/tools/c0gate.py build3ds/lane-NAT1F/pm_3ds.elf > build3ds/out/nat1f_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (3 drec equal). c0gate `calls 126 interp 37 state 23`. Report each c0diff wdiff and every c0dcase line (not judged).
Fix rights: compile errors. A DIFF or a tool fault: stop, send nat1f_c0src.txt to oracle NATIVE with SendMessage, and wait.
## Hand-back
Last commit `NAT1F 2: gate. 3 drec equal; c0diff <a> <b> <c>` with `git add -f build3ds/out/nat1f_c0src.txt build3ds/out/nat1f_loop.txt`. It deletes this page. One SubagentHandback: hash, the c0diff and c0dcase lines.
