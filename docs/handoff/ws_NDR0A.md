# ws_NDR0A (Haiku): snapchk, the core 1 combine snap at each mesh triangle and its core 0 compare (stage S4; counter: snapchk bad base)
Designer: oracle FIN. Branch from main (no NST2F file). Worktree build3ds/wt/NDR0A, branch NDR0A. PM_ROW=NDR0A PM_LANE=NDR0A.
Edit only: new 3ds/include/nd_snap.h, 3ds/src/nd_snap.c, 3ds/src/nd_snap_in.c, 3ds/tools/nd_snap_test.c, 3ds/tests/host.d/ndr0a.sh, 3ds/tools/azt.d/NDR0A.json,
3ds/tools/bench.d/NDR0A.json; 3ds/src/nd_geo.c (2 lines), 3ds/src/nd_geom.c (1 line), 3ds/src/nd_rec.c (2 places), 3ds/src/nd_psolk.c, 3ds/include/nd.h (1 line).
## Facts (code read, main 59a0ffc265)
- Core 0 combine inputs: comb_in_walker (nd_comb.c:199-205). The key test nd_pso_in_eq (nd_psokey.c:111) skips leg_mode. nd_pso_fix (:123) reads box_pat, mode5_prev.
- nd_mesh_tri1/2 (nd_geo.c:32, :35) write one TRI unit; core 0 draws it in draw_tri; box_fold_sync runs at nd_geom.c:339. A state run is NDC_NAPI with runner
  nd_nr_run (nd_rec.c:397). nd_comb_core writes pm_comb_vprim (nd_decide.c:234): core 1 never calls it. pm_comb_vprim is 0 outside particles (nd_walk.c:465-473).
## Edits
1. nd_snap.h: `typedef struct NdSnapChk { NdCombIn in; int32_t key, leg, mode5, pat, plan_on; } NdSnapChk;` and `_Static_assert(sizeof(NdSnapChk) % 8u == 0u, "units");`.
   Declare nd_snap_in, nd_snap_mesh, nd_snapchk_run, nd_snapchk_tri, `extern unsigned char nd_snap_arm;`.
2. nd_snap_in.c (pure, host-built): `void nd_snap_in(const NdDs* s, int leg, int pat, NdCombIn* in)`: memset 0; w0 = s->w.comb_w0, w1, omode_h, prim, env = s->w.envc,
   prim_lo8 = (uint8_t)s->w.prim_lod, key_c = s->keyc, key_s = s->keys, conv_k4; tst[0..15] from s->w.tst1 in the tst_f order (nd_comb.c:45-46); leg_mode = leg,
   tex_on, use_t1, t1_valid = s->w.td[1].valid, dual_on, box_prod, box_pat = pat, box_folded = 0, comb_set, comb_alpha_shade.
3. nd_psolk.c: rename nd_pso_find to `int nd_pso_find_i(...)`: `(int)m` on a hit, -1 at the end. Add `int nd_pso_find(const NdCombIn* w, unsigned vprim, NdCombOut* o)
   { return nd_pso_find_i(w, vprim, o) >= 0; }`. nd.h after :522: declare nd_pso_find_i.
4. nd_rec.c after nd_rec_cnt_take: `unsigned nd_rec_cmds(void) { return ndc_n[NDC_MODEL] + ndc_n[NDC_SPR] + ndc_n[NDC_ENT] + ndc_n[NDC_TEXRECT] + ndc_n[NDC_NAPI] +
   ndc_n[NDC_CALL]; }`. stream_run after `nd_stream_cur = p;` (:644): `{ const unsigned op = h >> 24; if (op != NDC_JUMP && (op != NDC_NAPI ||
   p[1] != (uint32_t)(uintptr_t)nd_nr_run)) nd_snap_arm = 0; }`.
5. nd_snap.c core 1 (switch snapchk only):
   ```
   static int sn_leg, sn_mode5, sn_pat; static NdSnapChk sn_last; static unsigned sn_cmds = ~0u;
   void nd_snap_mesh(void) {
       if (!PMDL("snapchk")) return;
       NdSnapChk r; NdCombOut o; memset(&r, 0, sizeof r);
       nd_snap_in(&nd_ds, sn_leg, sn_pat, &r.in);
       if (sn_cmds == nd_rec_cmds() && !memcmp(&r.in, &sn_last.in, sizeof r.in)) return; /* no new command, same inputs */
       r.key = nd_pso_find_i(&r.in, 0u, &o); r.plan_on = -1;
       if (r.key >= 0) { nd_pso_fix(&o, &r.in, sn_mode5); sn_leg = o.leg; sn_mode5 = o.mode5; sn_pat = o.pat; r.plan_on = o.plan_on; }
       r.leg = sn_leg; r.mode5 = sn_mode5; r.pat = sn_pat; sn_last = r;
       nd_rec_napi(nd_snapchk_run, &r, sizeof r / 8u); sn_cmds = nd_rec_cmds(); }
   ```
6. nd_snap.c core 0: `unsigned char nd_snap_arm;`, static NdSnapChk sc_cur. nd_snapchk_run: copy rec to sc_cur, nd_snap_arm = 1, recs++. nd_snapchk_tri: nd_snap_arm = 0;
   comb_in_walker(&mine) (declare it, nd_comb.c); one compare macro per field with its name ("tst" once for the 16). With sc_cur.key >= 0 also leg vs leg_mode,
   mode5 vs comb_mode5, pat vs box_pat, plan_on vs plan_active (nd_int.h:263); else miss++. Logs as nd_dschk.c:56-84: the first 8 `snapchk bad f<n> <names>`,
   every 100 frames `snapchk f<n> recs <r> cmp <c> bad <b> fbad <s> miss <m>`.
7. nd_geo.c: first line of nd_mesh_tri1 and nd_mesh_tri2: `nd_snap_mesh();`. nd_geom.c after :339: `if (nd_snap_arm) nd_snapchk_tri();`.
8. nd_snap_test.c: set comb_w0, keys, tst1.comb2_ad, dual_on, td[1].valid; check the 5 fields; print `ndr0a in bad <b>`. `--fault` expects tst[15] at tst[14].
   ndr0a.sh: copy of host.d/nst1.sh for nd_snap_test.c and nd_snap_in.c. azt.d and bench.d NDR0A.json: copies of the spot_mac01_dschk and wscmb_pause_dschk
   entries (NST3.json), dschk replaced by snapchk in name, debug, until (`snapchk f200 `), regex and text.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 4 Azahar starts)
1. Edits 1-8. `sh 3ds/tools/nd_host_test.sh` PASS with `ndr0a in bad 0` and the fault seen. Commit `NDR0A 1: snapchk combine snap`.
2. bench.py base spot_mac01_drec,nimgm_anim_drec (merge-base, not charged). rowloop --row NDR0A --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run spot_mac01_snapchk, wscmb_pause_snapchk; grep the snapchk lines > build3ds/out/ndr0a_snapchk.txt. Pass: recs and cmp above 0 in both. bad is the base.
## Fix rights: the listed files; compile errors. recs 0, a crash, a DIFF: stop, send the output path to the oracle. Wait. Stop: last commit
  `NDR0A 2: gate. 2 benches byte-equal; snapchk bad <2> fbad <2> miss <2>` deletes this page. One SubagentHandback with the bad name lines.
