# ws_NDR0E1 (Haiku): the snap carries the coloured combine decision; snapchk compares it with the GPU state at the triangle (stage S4; counter: snapchk bad)
Designer: oracle FIN. Branch from main (NDR0D merged). Worktree build3ds/wt/NDR0E1, branch NDR0E1. PM_ROW=NDR0E1 PM_LANE=NDR0E1.
Edit only: 3ds/src/nd_comb.c (sync_combine_work), 3ds/include/nd.h (1 line), 3ds/src/pm_gpu.c (4 getters), 3ds/include/pm_gpu.h (4 lines), 3ds/include/nd_snap.h, 3ds/src/nd_snap.c.
## Facts (code read, main 3601590c71)
- NDR0D gate: snapchk bad 0 (nimgm_strip f400 recs 938, wscmb_pause f200 recs 32). The snap holds the inputs and leg, mode5, pat, plan_on, not the GPU values.
- sync_combine_work (nd_comb.c:310-331) decides (nd_comb_pick), colours the plan with core 0 values (:324) and calls the GPU setters (:317-330).
- The GPU setters return at once on an equal value: pm_gpu_set_combine (pm_gpu.c:4419), pm_gpu_set_plan (:4398), pm_gpu_set_const (:4460); pm_gpu_set_env flushes only on a new value (:4454). So a later apply of the snap is a no-op when the GPU already holds the snap values. This row measures that.
## Edits
1. nd_comb.c: add above sync_combine_work (the tail of :316-330 as a function; nd.h after :490 declares it):
   ```
   void nd_comb_apply(const NdCombOut* o, uint32_t w0, uint32_t w1, int use_t1, uint32_t env) {   /* NDR0E1: the GPU part of sync_combine_work */
       sl_const = o->cc_on; sl_env = o->env_on;
       if (o->cc_on) { sl_cc = o->cc; pm_gpu_set_const(o->cc); } if (o->env_on) pm_gpu_set_env(env);
       leg_mode = o->leg; comb_mode5 = o->mode5; box_pat = o->pat; plan_active = 0;
       ls_use = o->use; if (o->use >= 0) ls_why = o->why; ls_cyc = o->cyc; ls_dual = o->dual; ls_frame = rm_frame;
       if (o->plan_on) { cur_plan = o->plan; pm_gpu_plan_key(w0, w1, o->cyc, use_t1); pm_gpu_set_plan(&cur_plan); plan_active = 1; return; }
       pm_gpu_set_combine(leg_mode);
   }
   ```
   sync_combine_work body after `nd_comb_pick(&in, &o);`: `if (o.plan_on) pm_comb_colours(&o.plan, nd_ws.prim, nd_ws.envc, (u8)nd_ws.prim_lod, keyc, keys);` `nd_comb_apply(&o, nd_ws.comb_w0, nd_ws.comb_w1, nd_ws.use_t1, nd_ws.envc);`. Delete the old :316-330. The GPU calls keep their order (byte-equal).
2. pm_gpu.c after pm_gpu_set_const (:4459-4463): `int pm_gpu_comb_now(void) { return gs.combine; }`, `const PmPlan* pm_gpu_plan_now(void) { return &gs.plan; }`, `unsigned pm_gpu_const_now(void) { return cconst; }`, `unsigned pm_gpu_env_now(void) { return gs.env_col; }`. Declare the 4 in pm_gpu.h after :85 (`/* NDR0E1 */`).
3. nd_snap.h: NdSnapChk gets `NdCombOut o;` after `pad`. If the size assert fails, add `uint32_t pad2;` after o.
4. nd_snap.c core 1 (nd_snap_mesh), after the decision line: `if (o.plan_on) pm_comb_colours_k(&o.plan, r.in.prim, r.in.env, r.in.prim_lo8 & 0xffu, r.in.key_c, r.in.key_s, r.in.conv_k4 & 255u);` and `r.o = o;` (core 1 reads no pm_k4: NDR0D).
5. nd_snap.c core 0 (nd_snapchk_tri), after the plan_on line (:101). Externs: cur_plan, sl_const, sl_cc (nd_comb.c:33, :83-84), ls_use, ls_cyc, ls_dual (:42).
   ```
   {   /* NDR0E1: the core 0 combine state and the GPU state against the snap decision */
       const NdCombOut* o = &sc_cur.o;
       if (plan_active && o->plan_on && memcmp(&cur_plan, &o->plan, sizeof o->plan)) sc_add("cplan");
       if (sl_const != o->cc_on || (o->cc_on && sl_cc != o->cc)) sc_add("sl");
       if (ls_use != o->use || ls_cyc != o->cyc || ls_dual != o->dual) sc_add("ls");
       if (pm_gpu_comb_now() != (o->plan_on ? 8 : o->leg)) sc_add("gcomb");
       else if (o->plan_on && memcmp(pm_gpu_plan_now(), &o->plan, sizeof o->plan)) sc_add("gplan");
       if (o->cc_on && pm_gpu_const_now() != o->cc) sc_add("gconst"); if (o->env_on && pm_gpu_env_now() != sc_cur.in.env) sc_add("genv");
   }
   ```
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 4 Azahar starts)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh` PASS. Commit `NDR0E1 1: snap combine decision, GPU state compare`.
2. bench.py base spot_mac01_drec,nimgm_anim_drec (merge-base, not charged). rowloop --row NDR0E1 --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run nimgm_strip_snapchk and wscmb_pause_snapchk; grep -a the snapchk lines > build3ds/out/ndr0e1_snapchk.txt. Pass: recs above 0 and bad 0 in the last counter line of both (GUESS: bad 0). Else: commit the grep, send the output path (with the `snapchk bad` name lines) to the oracle, wait.
Fix rights: the listed files; compile errors. A DIFF, a crash: stop, send the output path to the oracle. Wait. Stop: last commit `NDR0E1 2: gate. 2 benches byte-equal; snapchk bad <2> fbad <2> miss <2>; names <list>` deletes this page. One SubagentHandback.
