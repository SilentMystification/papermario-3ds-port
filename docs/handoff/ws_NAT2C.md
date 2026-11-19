# ws_NAT2C (Haiku): the model cache key B reads the pre-record state that core 1 puts into the model snap, not nd_ws. Counter: c0gate unchanged, expected `c0gate calls 126 interp 37 state 23` (removes the hot nd_ws reader pm_gpu.c:3867).
Designer: oracle NATIVE. Branch from main (72150ce49c or later). Worktree build3ds/wt/NAT2C, branch NAT2C. PM_ROW=NAT2C PM_LANE=NAT2C. Runs beside NAT2H.
Edit only: 3ds/include/nd_msnap.h, nd_msnapw.h, 3ds/src/nd_msnap.c, 3ds/src/nd_rec.c (new lines at main 224 and 550), 3ds/src/pm_gpu.c (pm_gpu_model_keyb, line 3867).
Overlap: NAT2H edits nd_rec.c after line 408 only (140+ lines apart). No edit in nd_walk.c, nd_set.c, nd_comb.c, nd_direct.c, nd_rect.c, nd_geom.c.
## Facts (main 72150ce49c)
- FACT pm_gpu_model_keyb (pm_gpu.c:3859-3872) puts nd_ws.omode_h, omode_l, om_kh, om_kl, geom (masked) into key words c[1]-c[5]. rec_run calls it before rec_apply (nd_recs.c:311): the walker state BEFORE the record.
- FACT the model snap is taken after nd_ds_model applied the record (nd_rec.c:224-225, nd_msnap_rec nd_msnap.c:18): it holds the state AFTER the record, so its omh, oml, kh, kl, geom do not fit key B.
- FACT nd_msnap_take memsets the snap (nd_msnap_in.c:13). ND_MSNAP_U follows sizeof(NdMSnap) (nd_msnap.h:15), so nd_rec_model reserves the new room by itself. nd_msnap_diff (host test nrec1a) compares named fields only.
- FACT a record with no snap (ms_of NULL, nd_msnap.c:27) keeps the walker words as today.
## Edits (new lines end with `/* NAT2C */`)
1. nd_msnap.h: in NdMSnap, after `envc` add the words `uint32_t k_omh, k_oml, k_kh, k_kl, k_geom, k_pad; /* NAT2C: core 1 state before the record (model key B) */` (keep `uint32_t magic, ... envc;` before them, b[2] after).
   Add `void nd_msnap_pre(const NdDs* s); /* NAT2C */` to nd_msnap.h. Add `const uint32_t* nd_msnap_key(const uint32_t* q); /* NAT2C: k_omh .. k_geom of the record's snap, NULL when none */` to nd_msnapw.h (pm_gpu.c:26 includes it).
2. nd_msnap.c, a new line after `extern unsigned fogc, blendc;`: `static uint32_t ms_pre[5]; void nd_msnap_pre(const NdDs* s) { ms_pre[0] = s->w.omode_h; ms_pre[1] = s->w.omode_l; ms_pre[2] = s->w.om_kh; ms_pre[3] = s->w.om_kl; ms_pre[4] = s->w.geom; } /* NAT2C */`
   In nd_msnap_rec, after `nd_msnap_take(&nd_ds, m);` add `    m->k_omh = ms_pre[0]; m->k_oml = ms_pre[1]; m->k_kh = ms_pre[2]; m->k_kl = ms_pre[3]; m->k_geom = ms_pre[4]; /* NAT2C */`.
   After the ms_of line add: `const uint32_t* nd_msnap_key(const uint32_t* q) { const NdMSnap* s = ms_of(q); return s ? &s->k_omh : NULL; } /* NAT2C */`
3. nd_rec.c (it includes nd_msnap.h, line 21): a new line above main line 224 (`    { const unsigned i = (q[0] >> 16) & 0xffu; nd_ds_model(...`): `    nd_msnap_pre(&nd_ds); /* NAT2C: the state before the record */`.
   Host link (geounit.sh links nd_rec.c without nd_msnap.c): a new line after the weak nd_msnap_rec (main line 550): `__attribute__((weak)) void nd_msnap_pre(const NdDs* s) { (void)s; } /* NAT2C */`.
4. pm_gpu.c pm_gpu_model_keyb, line 3867 before: `    c[1] = nd_ws.omode_h & kh; c[2] = nd_ws.omode_l & kl; c[3] = nd_ws.om_kh & kh; c[4] = nd_ws.om_kl & kl; c[5] = nd_ws.geom & gk & rm_ga[rmi];`
   after: `    { const uint32_t* kw = nd_msnap_key(rec); const uint32_t w5[5] = { nd_ws.omode_h, nd_ws.omode_l, nd_ws.om_kh, nd_ws.om_kl, nd_ws.geom }; if (!kw) kw = w5; /* NAT2C: core 1 words, nd_ws only with no snap */`
   `      c[1] = kw[0] & kh; c[2] = kw[1] & kl; c[3] = kw[2] & kh; c[4] = kw[3] & kl; c[5] = kw[4] & gk & rm_ga[rmi]; }`
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 4 Azahar starts of 8)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat2c_host.txt 2>&1`: PASS (nrec1a too, the 3 CACHECHECK lines are known). Commit `NAT2C 1: model key B from the core 1 pre-record words`.
2. For B in spot_mac01_drec, entcape_kmr03_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat2c_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT2C --bench spot_mac01_drec,entcape_kmr03_drec --no-host --why "NAT2C: model key B from core 1" > build3ds/out/nat2c_loop.txt 2>&1`.
4. `grep -H "c0src f\|msnap nos\|drec draw .* DIFF" build3ds/azt/logs/spot_mac01_drec.txt build3ds/azt/logs/entcape_kmr03_drec.txt build3ds/out/loop_NAT2C_cmp_*.txt > build3ds/out/nat2c_c0src.txt`.
5. `python 3ds/tools/c0gate.py build3ds/lane-NAT2C/pm_3ds.elf > build3ds/out/nat2c_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (2 drec equal). c0gate `calls 126 interp 37 state 23`. Fix rights: compile errors, a missing include or prototype. A DIFF or a tool fault: stop, send nat2c_c0src.txt to oracle NATIVE with SendMessage, wait.
## Hand-back
Last commit `NAT2C 2: gate. 2 drec equal; model key B from core 1` with `git add -f build3ds/out/nat2c_c0src.txt build3ds/out/nat2c_loop.txt`. It deletes this page. One SubagentHandback: hash, the 2 c0src lines, the msnap nos lines (or none).
