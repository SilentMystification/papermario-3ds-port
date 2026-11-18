# ws_NDR5 (Haiku): nd_xf_push and nd_xf_pop record one native transform record, core 0 runs the matrix stack (UD2) (stage S4; counter: none)
Designer: oracle FIN. WAIT: branch from branch REN1 at its gate commit (REN1 edits nd_xf.c). Worktree build3ds/wt/NDR5, branch NDR5. PM_ROW=NDR5 PM_LANE=NDR5.
Edit only: 3ds/src/nd_xf.c, 3ds/src/nd_rec.c (1 function), 3ds/include/nd_api.h (1 line).
## Facts (code read, main 2548d439f4)
- nd_xf_push and nd_xf_pop (nd_xf.c:12-19) write one MTX or POPMTX unit (nd_at.h:31-32) into the state run. Core 0 runs it in op_geo (nd_walk.c):
  no skip test, no gv_epoch, `pm_gbi_cur_op = op;`, then `load_mtx(a, b, 0)` (MTX) or `gbi_popmtx(b)` (POPMTX). nd_walk_state has no run start rule (:394).
- A record closes the state run (rec_take, nd_rec.c:188) and sets ds_sclost = 1 (:201). The dschk ep rule (b) then sets ep at the next OMH, PRIM, ENV,
  TEXTURE or COMB unit (:253). With st_take set, rec_take skips the shadow reset, the dschk record and ds_sclost (:189-201).
- nd_op_n64 (nd_op.h:137) maps a unit id to its F3DEX2 op. The effect sites with NDXF_PUSH(ND_NR_AT, ...) are particle record units: no edit (row NREC4).
## Edits
1. nd_rec.c after nd_rec_napi: `void nd_rec_xf(NdNapiRun run, const uint32_t* rec) { st_take = 1; nd_rec_napi(run, rec, 1u); st_take = 0; }
   /* NDR5: a transform record is taken as a state op: the shadow stays, no dschk record, ds_sclost stays (rec_take :188-201) */`.
   nd_api.h after nd_rec_napi (:11): declare it.
2. nd_xf.c (the unit words stay the NDXF_PUSH and NDXF_POP words; core 1 still applies them to nd_ds):
   ```
   static void nd_xf_run(const uint32_t* r) {   /* core 0: op_geo's MTX and POPMTX arms */
       const unsigned id = r[0] >> 24;
       pm_gbi_cur_op = nd_op_n64[id - ND_FS_BASE];
       if (id == ND_FS_MTX) load_mtx(r[0] & 0xffffffu, r[1], 0); else gbi_popmtx(r[1]);
   }
   static void xf_rec(uint32_t w0, uint32_t w1) {
       const uint32_t r[2] = { w0, w1 };
       nd_ds_unit(&nd_ds, w0, w1);              /* core 1 state and ep rule, as nd_rec_nr did */
       nd_rec_xf(nd_xf_run, r);
   }
   void nd_xf_push(const void* m, unsigned p) { NDXF_PUSH(xf_rec, m, p); }
   void nd_xf_pop(unsigned which) { (void)which; NDXF_POP(xf_rec); }
   ```
   Includes: nd_ds.h (nd_ds_unit, nd_ds), nd_int.h (load_mtx :79, gbi_popmtx :74), `extern unsigned pm_gbi_cur_op;` (nd_walk.c:29).
   If REN1 left NDO_ names in nd_xf.c, use ND_FS_ (nd_fs.h). Update the file comment (:1-3): the record replaces the unit.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 4 Azahar starts)
1. Edits 1-2. `sh 3ds/tools/nd_host_test.sh` PASS. Commit `NDR5 1: transform record`.
2. bench.py base spot_mac01_drec,nimgm_anim_drec (merge-base, not charged). rowloop --row NDR5 --bench the same 2: LOOP PASS, byte-equal.
3. bench.py base spot_mac01_dschk,wscmb_pause_dschk (merge-base, not charged; read the dschk lines of the base logs the tool names). Then bench.py run
   the same 2; grep the dschk lines of both > build3ds/out/ndr5_dschk.txt. Pass: the last line of each run equals its base in cmds, bad, fbad and epbad
   (the record adds no dschk compare; dschk_base.txt predates FALLBK, do not compare with it). Do not write dschk_base.txt.
## Fix rights: the listed files; compile errors. A DIFF, a dschk value that differs from its base, a crash: stop, send the output path to the oracle. Wait.
Stop: last commit `NDR5 2: gate. 2 benches byte-equal; dschk bad <2> fbad <2> epbad <2>` deletes this page. One SubagentHandback.
