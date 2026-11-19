# ws_NAT4J (Haiku): stream TEXTURE and OMH units become native state records (as NAT4I). Counter: c0gate calls +2 (the skip state syncs move into the runners; NAT6 deletes them), expected `c0gate calls 124 interp 34 state 23`. Gate: drec byte-equal.
Designer: oracle NATIVE2. WAIT: `Merge NAT4I`. Branch from main after it. Worktree build3ds/wt/NAT4J, branch NAT4J. PM_ROW=NAT4J PM_LANE=NAT4J.
Edit only: 3ds/src/nd_rec.c, 3ds/src/nd_set.c, 3ds/tools/nd_st_test.c. If NAT4H is not merged yet and your c0gate base is 122, the expected value stays 124.
## Facts (main 4260a4f73b plus NAT4I)
- FACT nr_set (nd_walk.c:246-262) runs a stream TEXTURE or OMH unit: `same = nd_unit_same(u)`, cut = the core 1 mark, `if (same && !cut) return;`, pm_gbi_cur_op, gv_epoch++ on the cut, nd_unit_apply when not same.
- FACT nd_unit_same: OMH nd_set.c:126 (bits equal under the mask and sc_ok; when same it also does `nd_ws.om_kh |= m`), TEXTURE :139 (tex_on, tex_ss, tex_ts equal, sc_ok, sc_sig_now() == sc_sig).
- FACT nd_unit_apply: OMH :163 `set_om(1, ...)` (static, :84-97: mask merge, om_kh, texture filter, cycle sync), TEXTURE :173 `nd_tx_texon(a, b); nd_comb_sync();` (set_texture nd_tex_set.c:222-226 stores the 3 fields only).
- FACT core 1 marks OMH on a change or sc_lost, TEXTURE on a change, sc_lost or sc_dirty (nd_ds_ep_rule nd_ds.c:403-404). The OMH a field is shift | length << 8 (16 bits), the TEXTURE a field is on (0 or 1).
- FACT stunit (nd_st_test.c:151-153) checks that the cyc shape (OMH) is a state run. After this row cyc is a native record. The `rm` shape (OML) stays a state run (mark + unit) until NAT4K.
## Edits (new lines end with `/* NAT4J */`)
1. nd_rec.c, the NAT4I weak line: add `__attribute__((weak)) void nd_tex_run(const uint32_t* r) { (void)r; } __attribute__((weak)) void nd_omh_run(const uint32_t* r) { (void)r; }`.
2. nd_rec.c nd_rec_nr, the NAT4I block: the condition gets `|| id == ND_FS_TEXTURE || id == ND_FS_OMH`. Before the line `else { const unsigned k = (w0 >> 16) ...` (the MW branch) add:
```c
        else if (id == ND_FS_TEXTURE) rec_nat(nd_tex_run, (w0 & 0xffffffu) | mk, w1); /* NAT4J */
        else if (id == ND_FS_OMH) rec_nat(nd_omh_run, (w0 & 0xffffu) | mk, w1); /* NAT4J */
```
3. nd_set.c, at the end of the file (after the NAT4I runners):
```c
void nd_tex_run(const uint32_t* r) { const unsigned on = r[0] & 0xffffffu, ss = r[1] >> 16, ts = r[1] & 0xffffu; const int mk = (int)(r[0] >> 31); /* NAT4J: TEXTURE, nr_set + nd_unit_apply */
    const int same = nd_ws.tex_on == (int)on && nd_ws.tex_ss == ss && nd_ws.tex_ts == ts && sc_ok && sc_sig_now() == sc_sig;
    if (same && !mk) return; pm_gbi_cur_op = ND_FS_TEXTURE; if (mk) gv_epoch++; if (same) return;
    nd_ws.tex_on = (int)on; nd_ws.tex_ss = ss; nd_ws.tex_ts = ts; nd_comb_sync(); }
void nd_omh_run(const uint32_t* r) { const uint32_t m = nd_op_om_mask(r[0] & 0xffffu), v = r[1]; const int mk = (int)(r[0] >> 31); /* NAT4J: OMH, nr_set + nd_unit_apply */
    const int same = (v & m) == (nd_ws.omode_h & m) && sc_ok; if (same) nd_ws.om_kh |= m;
    if (same && !mk) return; pm_gbi_cur_op = ND_FS_OMH; if (mk) gv_epoch++;
    if (!same) set_om(1, r[0] & 0xffu, (r[0] >> 8) & 0xffu, m, v); }
```
4. nd_st_test.c: add `void nd_omh_run(const uint32_t* r);` next to the NAT4I prototype. Replace the NAT4F run check (`run_one(n_cyc, &b);` and its `if`) by:
```c
    run_one(n_rm, &b); /* NAT4J: OML stays a state run: the mark, then the unit */
    if (b.nw < 6u || b.w[0] != NDC_HDR(NDC_NAPI, 2, 0) || b.w[1] != (uint32_t)(uintptr_t)nd_nr_run || b.w[2] != ND_FS_W0(ND_FS_MISC, ND_FS_MISC_EP << 16)
        || (b.w[4] >> 24) != ND_FS_OML || b.nu != 4u || b.u[2] != b.w[4] || b.u[3] != b.w[5]) { printf("stunit FAIL run\n"); return 1; }
```
   In the NAT4I nat check, after its `if`, add: `run_one(n_cyc, &b); if (b.nw < 4u || b.w[0] != NDC_HDR(NDC_NAPI, 1, 1) || b.w[1] != (uint32_t)(uintptr_t)nd_omh_run || b.w[2] != (0x80000000u | G_MDSFT_CYCLETYPE | 2u << 8) || b.w[3] != G_CYC_1CYCLE || b.nu != 0u) { printf("stunit FAIL nat\n"); return 1; } /* NAT4J */`. The two stunit.sh seeds stay as they are (run seed: FAIL run, nat seed: FAIL nat).
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat4j_host.txt 2>&1`: PASS. Commit `NAT4J 1: TEXTURE and OMH as native state records`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat4j_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT4J --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT4J: native TEXTURE OMH records" > build3ds/out/nat4j_loop.txt 2>&1`.
4. `grep -H "c0src f\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT4J_cmp_*.txt > build3ds/out/nat4j_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT4J/pm_3ds.elf > build3ds/out/nat4j_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (4 drec equal). c0gate `calls 124 interp 34 state 23`. Fix rights: compile errors, missing prototypes. A DIFF, a crash or a tool fault: stop, send nat4j_c0src.txt and the first DIFF line to oracle NATIVE2 with SendMessage, wait.
## Hand-back
Last commit `NAT4J 2: gate. 4 drec equal; TEXTURE OMH native records; c0gate calls 124` with `git add -f build3ds/out/nat4j_c0src.txt build3ds/out/nat4j_loop.txt`. It deletes this page. One SubagentHandback: hash, the gate line per bench, the c0gate line.
