# ws_NST2B (Haiku): model records feed the core 1 state nd_ds (stage S3; counter: dschk bad lower than base)
Designer: oracle NSTO. WAIT: branch from main after the NST2A merge. Worktree build3ds/wt/NST2B, branch NST2B. PM_ROW=NST2B PM_LANE=NST2B.
Edit only: 3ds/src/nd_ds.c, 3ds/include/nd_ds.h, 3ds/src/nd_rec.c, 3ds/src/nd_dschk.c, 3ds/src/nd_recs.c (2 lines), 3ds/tools/nd_ds_test.c,
3ds/tests/host.d/nst1.sh, 3ds/tools/fxrest.d/dschk_base.txt.
## Facts (code read, main 4b6423aa11)
- nd_rec_model (nd_rec.c:220-230) sends no unit through nd_rec_nr. Payload q = p + 2: pair Rk is q[2k], q[2k+1]. Kept R13 units: count q[26],
  units from q[28]. Core 0 miss order: rec_apply (nd_recs.c:72-103). Matrix ops, R11 ranges (row NST2D), geometry and pop are not NdDs state.
- Core 0 hit: hit_apply (nd_recs.c:198, callers :241, :327) writes the scalar state only (hit_delta :140). It skips the lights, the tile and
  load units, the panner sizes and the omode_h filter bits (:125). dschk copies nd_ds when rec_take records a command: apply after rec_take.
## Edits
1. nd_ds.h: `typedef void (*NdDsUnitFn)(NdDs* s, uint32_t w0, uint32_t w1);` `void nd_ds_model(NdDs* s, const uint32_t* q, const uint32_t* rm, unsigned nrm, NdDsUnitFn unit);`
2. nd_ds.c: nd_ds_model as below (rec_apply order; rec_lights nd_mtx_set.c:202; nd_bind_run nd_tex_set.c:552). Include "macros.h" for
   PM_RM_SHROUD (include/macros.h:300). T(id, a, b) = `nd_ds_apply(s, NDO_W0(id, a), b)`. FOGP(x) = T(NDO_MW, NDO_MW_FOG << 16, x).
   ```
   const uint32_t f = q[6]; if (f & 4u) T(NDO_PRIM, 0, q[12]);  if (f & 8u) T(NDO_ENV, 0, q[13]);  if (f & 64u) { T(NDO_FOG, 0, q[8]); FOGP(q[11]); }
   if (q[7] && q[4] <= 7u) { const unsigned n = q[4] ? q[4] : 1u; const Lightsn* L = (const Lightsn*)(uintptr_t)q[7]; T(NDO_MW, NDO_MW_NUMLIGHT << 16, NUML(n));
       for (unsigned i = 1; i <= n + 1u; i++) T(NDO_MOVEMEM, (((uint32_t)sizeof(Light) - 1u) / 8u) << 19 | (3u * i + 3u) << 8 | G_MV_LIGHT,
           (uint32_t)(uintptr_t)(i <= n ? (const void*)&L->l[i - 1u] : (const void*)&L->a)); }
   if (q[26]) { for (unsigned k = 0; k < q[26]; k++) unit(s, q[28 + 2 * k], q[29 + 2 * k]); }
   else if (q[1]) for (const uint32_t* u = (const uint32_t*)(uintptr_t)q[1]; (u[0] >> 24) != NDO_END; u += 2) nd_ds_apply(s, u[0], u[1]);
   if (q[9]) { const uint32_t* c = (const uint32_t*)(uintptr_t)q[9]; nd_ds_apply(s, c[0], c[1]); }
   for (unsigned k = 0; k < nrm; k++) unit(s, rm[2 * k], rm[2 * k + 1]);
   if (f & 32u) { T(NDO_OML, NDO_A_RENDERMODE, (uint32_t)(PM_RM_SHROUD) | q[15]); T(NDO_FOG, 0, q[8]); if (f & 16u) T(NDO_BLEND, 0, q[14]); FOGP(q[11]); }
   else if (f & 2u) { T(NDO_FOG, 0, q[8]); FOGP(q[11]); }  if (q[18]) nd_ds_apply(s, q[18], q[19]);  if (q[20]) nd_ds_apply(s, q[20], q[21]);
   ```
3. nd_rec.c: move the nd_rec_nr body after the untyped check, except rec_op_t (nd_ds_apply, NST3 ep rule, NST2A NDO_CALL list), into
   `static void ds_unit(NdDs* s, uint32_t w0, uint32_t w1)`. nd_rec_nr calls `ds_unit(&nd_ds, w0, w1);`. In nd_rec_model after :229 add
   `{ const unsigned i = (q[0] >> 16) & 0xffu; nd_ds_model(&nd_ds, q, i < ND_RM_N ? nd_rm_tab[i] : NULL, i < ND_RM_N ? nd_rm_n[i] : 0u, ds_unit); }`
4. nd_recs.c: `extern unsigned char nd_dschk_hit;` and `nd_dschk_hit = 1;` as the first line of hit_apply (:198).
5. nd_dschk.c: define `unsigned char nd_dschk_hit;`. In nd_dschk_run, before core 0 fills its NdDs: `if (nd_dschk_hit) { hit_adopt(c); nd_dschk_hit = 0; }`
   (c = the core 1 NdDs of the record). hit_adopt copies from c into core 0: nd_ws.timg, tlut, tlut_n, tst1 (whole), the omode_h bits
   `3u << G_MDSFT_TEXTFILT`, nd_tm = c->tm, lt[], lt_n, lk[][], dual_on. Use the field names of nd_ds.h on main.
6. nd_ds_test.c: one record: f = 4|8|64|2, one light, bind list {TIMG, TILE, END}, comb unit, rm {OML}, R9 TILESIZE. Check prim, envc, fogc,
   fog_fm, lt_n, lt[0].col, timg, tile_fmt[0], comb_w0, omode_l, tile_sz[0]. Print `nst2b model fields <n> bad <b>`. Fault: clear bit 8 of f.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 6 Azahar starts)
1. Edits 1-6. Host PASS with `nst2b model fields <n> bad 0` and the fault seen. Commit `NST2B 1: model records feed nd_ds; dschk hit adopt`.
2. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NST2B --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run the 4 dschk scenes (azt.d/NST3.json). Grep their dschk lines > build3ds/out/nst2b_dschk.txt. Pass: in each scene the last
   `dschk f`/`dschk end` bad is not higher than dschk_base.txt of the merge-base, and the sum of the 4 is lower (GUESS). Write the grep into dschk_base.txt.
## Fix rights: the listed files; fix compile errors. A higher bad, a crash, a DIFF: stop, send the output path to oracle NSTO. Wait. Stop: last commit `NST2B 2: gate. 2 benches byte-equal; dschk bad <4 values> (base <4 values>)` deletes this page. One SubagentHandback.
