# ws_NST2G (Haiku): dschk on main with FALLBK: adopt after the boot fallback, keep the core 0 box fold at a direct adopt (stage S3 end; counter: dschk bad, epbad)
Designer: oracle FIN. WAIT: branch from main after the NDR0B merge (NDR0B edits nd_dschk.c; main holds FALLBK 91d6172eb3). Worktree build3ds/wt/NST2G,
branch NST2G. PM_ROW=NST2G PM_LANE=NST2G. Edit only: 3ds/src/nd_dschk.c, 3ds/src/nd_walk.c (1 line), 3ds/tools/fxrest.d/dschk_base.txt.
## Facts (code read, main 2548d439f4)
- pm_gbi_fallback (nd_walk.c:773-807) runs on core 0 only, while no frame drew (pm_main.c:464). Its units: cycle FILL, fill colour 0x22292229, a fill
  rectangle, cycle 1CYCLE, a projection matrix, 3 vertices, 1 triangle (:796-803). Core 1 never sees them: omode_h, om_kh and fill differ after it.
- dir_adopt sets `box_folded = 0` (nd_dschk.c:75) and copies the unfolded tst1 and use_t1. sc_sig_now reads box_folded (nd_comb.c:191-193), so the TEXTURE
  and COMB skips (nd_set.c:139-141) fail and nd_st_epoch rises under dschk only (NST2F fxchk_a epbad 69440 -> 70162).
- dschk_base.txt holds NST2F or NDR0B values from runs without FALLBK. This row writes the first base with FALLBK.
## Edits
1. nd_dschk.c: `unsigned char nd_dschk_fb; /* NST2G: set after pm_gbi_fallback; the next record is adopted (fb_adopt) */` and
   `static void fb_adopt(const NdDs* c) { nd_ws.omode_h = c->w.omode_h; nd_ws.om_kh = c->w.om_kh; nd_ws.fill = c->w.fill; }`.
   nd_dschk_run after the dir line (:93): `if (nd_dschk_fb) { adopt_count(theirs); fb_adopt(theirs); nd_dschk_fb = 0; }`.
2. nd_walk.c after `nd_ops_run(dl, ND_LIST_MAX, 0);` (:806): `{ extern unsigned char nd_dschk_fb; nd_dschk_fb = 1; }`.
3. dir_adopt: delete `box_folded = 0;` (:75). Replace `nd_ws.tst1 = c->w.tst1; nd_ws.use_t1 = c->w.use_t1;` by:
   ```
   {   /* NST2G: a folded core 0 keeps its folded combine; box_save takes the core 1 (unfolded) values, so dschk_fill and the unfold give them */
       const NdTst k = nd_ws.tst1; const int ku = nd_ws.use_t1;
       nd_ws.tst1 = c->w.tst1; nd_ws.use_t1 = c->w.use_t1;
       if (box_folded) {
           box_save[0] = c->w.tst1.comb_a; box_save[1] = c->w.tst1.comb_b; box_save[2] = c->w.tst1.comb_c; box_save[3] = c->w.tst1.comb_d;
           nd_ws.tst1.comb_a = k.comb_a; nd_ws.tst1.comb_b = k.comb_b; nd_ws.tst1.comb_c = k.comb_c; nd_ws.tst1.comb_d = k.comb_d;
           nd_ws.tst1.comb_aa = k.comb_aa; nd_ws.tst1.comb_ab = k.comb_ab; nd_ws.tst1.comb_ac = k.comb_ac; nd_ws.tst1.comb_ad = k.comb_ad;
           nd_ws.use_t1 = ku;
       }
   }
   ```
   Update the comment above dir_adopt (:64-65): it no longer clears box_folded.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 6 Azahar starts; the 4 dschk scenes of azt.d/NST3.json)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh` PASS. Commit `NST2G 1: dschk adopt after the boot fallback, box fold kept at a direct adopt`.
2. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NST2G --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run the 4 dschk scenes; grep the dschk lines > build3ds/out/nst2g_dschk.txt. Write the grep into dschk_base.txt (the first base with FALLBK).
   Pass (oracle round 1): bad 0 in the last line of each; epbad is recorded (the counter of row NST2H). Append each scene's grep right after its run
   (the lane log is overwritten). Else: commit the grep, send the `dschk bad` name lines to the oracle, wait.
## Fix rights: the listed files; compile errors. A DIFF, a crash: stop, send the output path to the oracle. Wait.
Stop: last commit `NST2G 2: gate. dschk bad <4> epbad <4> fbad <4>; adopt <4> adiff <4>` deletes this page. One SubagentHandback.
