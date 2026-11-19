# ws_NAT4I (Haiku): stream SCISSOR, GEOM and MW units become native state records. Core 1 writes the value after the unit. Core 0 runs no unit for them. Counter: c0gate unchanged, expected `c0gate calls 122 interp 34 state 23`. Gate: drec byte-equal.
Designer: oracle NATIVE2. WAIT: none. Start commit main (4260a4f73b or later). Worktree build3ds/wt/NAT4I, branch NAT4I. PM_ROW=NAT4I PM_LANE=NAT4I.
Edit only: 3ds/src/nd_rec.c, 3ds/src/nd_set.c, 3ds/tools/nd_st_test.c, 3ds/tests/host.d/stunit.sh. NAT4H runs at the same time on other files.
## Facts (main 4260a4f73b)
- FACT nd_rec_nr (nd_rec.c:407-417) applies the unit to nd_ds (:413) and records it in a state run (st_put :416). nr_set (nd_walk.c:246-262) runs it on core 0: skip test, pm_gbi_cur_op, gv_epoch++ on the core 1 mark, nd_unit_apply.
- FACT setters: set_geom nd_set.c:33-36, set_scissor :47-63, set_mw :108-119. Skip tests: GEOM :128-131, MW LIGHTCOL :143-145, SCISSOR none. Marks (nd_ds_ep_rule nd_ds.c:402-408): SCISSOR always, GEOM on a change, MW always but LIGHTCOL, LIGHTCOL on a change.
- FACT a NAPI header with flag 1 keeps the snap arm (nd_rec.c:579). NDC_NRUN is not in nd_rec_cmds (:548): the snap memo stays. st_take 1 keeps the shadow (:180).
- FACT nd_r11_walk (nd_model.c:288) and nd_r11_op (:265-274) refuse GEOM, SCISSOR, MW and every non-nd_nr_run record: no change for model ranges.
## Edits (new lines end with `/* NAT4I */`)
1. nd_rec.c, directly above `void nd_rec_nr(uint32_t w0, uint32_t w1) {` (:407):
```c
__attribute__((weak)) void nd_sci_run(const uint32_t* r) { (void)r; } __attribute__((weak)) void nd_geo_run(const uint32_t* r) { (void)r; } __attribute__((weak)) void nd_mw_run(const uint32_t* r) { (void)r; } /* NAT4I: strong in nd_set.c */
static void rec_nat(NdNapiRun run, uint32_t a, uint32_t b) { /* NAT4I: a native state record in the place of a stream unit; header flag 1 keeps the snap arm */
    st_take = 1; uint32_t* p = rec_take(4); st_take = 0; ndc_n[NDC_NRUN]++;
    p[0] = NDC_HDR(NDC_NAPI, 1, 1); p[1] = (uint32_t)(uintptr_t)run; p[2] = a; p[3] = b; }
```
2. nd_rec_nr, after the line `rec_open(); ndc_sw++; if (sh_drop(t)) return; ...` (:414), before the NAT4G mark line:
```c
    if (id == ND_FS_SCISSOR || id == ND_FS_GEOM || id == ND_FS_MW) { const uint32_t mk = nd_ds_ep_rule(w0, ch, lost, dirty) ? 0x80000000u : 0u; /* NAT4I: no mark, no unit */
        if (id == ND_FS_SCISSOR) rec_nat(nd_sci_run, ((uint32_t)nd_ds.sci_x0 & 0xffffu) | (uint32_t)nd_ds.sci_y0 << 16, ((uint32_t)nd_ds.sci_x1 & 0xffffu) | (uint32_t)nd_ds.sci_y1 << 16);
        else if (id == ND_FS_GEOM) rec_nat(nd_geo_run, nd_ds.w.geom, mk);
        else { const unsigned k = (w0 >> 16) & 0xffu, g = w0 & 0xffffu; rec_nat(nd_mw_run, (w0 & 0xffffffu) | mk, (k == ND_FS_MW_SEG && g < 16u) ? nd_ds.w.segbase[g] : k == ND_FS_MW_NUMLIGHT ? nd_ds.lt_n : w1); }
        return; }
```
3. nd_set.c, at the end of the file (each runner is nr_set plus the setter of its unit, nd_walk.c:254-260):
```c
/* NAT4I: the native state records of nd_rec.c rec_nat (core 1 values after the unit; bit 31 = the core 1 mark) */
void nd_sci_run(const uint32_t* r) { pm_gbi_cur_op = ND_FS_SCISSOR; gv_epoch++; sci_x0 = (int16_t)r[0]; sci_y0 = (int16_t)(r[0] >> 16); sci_x1 = (int16_t)r[1]; sci_y1 = (int16_t)(r[1] >> 16); pm_gpu_set_scissor(sci_x0, sci_y0, sci_x1, sci_y1); }
void nd_geo_run(const uint32_t* r) { const unsigned g = r[0]; const int mk = (int)(r[1] >> 31);
    const int same = g == nd_ws.geom && pm_gpu_mode_is((nd_ws.geom & G_ZBUFFER) != 0, (nd_ws.geom & G_LIGHTING) != 0);
    if (same && !mk) return; pm_gbi_cur_op = ND_FS_GEOM; if (mk) gv_epoch++;
    if (!same) { nd_ws.geom = g; pm_gpu_set_mode((g & G_ZBUFFER) != 0, (g & G_LIGHTING) != 0); } }
void nd_mw_run(const uint32_t* r) { const unsigned k = (r[0] >> 16) & 0xffu, g = r[0] & 0xffffu; const uint32_t v = r[1]; const int mk = (int)(r[0] >> 31);
    const int same = k == ND_FS_MW_LIGHTCOL && g < 9u && lt[g].col[0] == (u8)(v >> 24) && lt[g].col[1] == (u8)(v >> 16) && lt[g].col[2] == (u8)(v >> 8);
    if (same && !mk) return; pm_gbi_cur_op = ND_FS_MW; if (mk) gv_epoch++; if (same) return;
    if (k == ND_FS_MW_NUMLIGHT) { lit_freeze(); lt_n = v; } else if (k == ND_FS_MW_LIGHTCOL && g < 9u) { lt[g].col[0] = (u8)(v >> 24); lt[g].col[1] = (u8)(v >> 16); lt[g].col[2] = (u8)(v >> 8); }
    else if (k == ND_FS_MW_FOG) { fog_fm = (s16)(v >> 16); fog_fo = (s16)v; } else if (k == ND_FS_MW_SEG && g < 16u) nd_ws.segbase[g] = v; }
```
4. nd_st_test.c: add `void nd_sci_run(const uint32_t* r);` above main. After the NAT4F `run` check (:152-153) add: `run_one(n_scis, &b); if (b.nw < 4u || b.w[0] != NDC_HDR(NDC_NAPI, 1, 1) || b.w[1] != (uint32_t)(uintptr_t)nd_sci_run || b.w[2] != (10u | 20u << 16) || b.w[3] != (300u | 200u << 16) || b.nu != 0u) { printf("stunit FAIL nat\n"); return 1; } /* NAT4I */`.
5. stunit.sh, after the run seed block (:12-16), a seed of the same form: sed `s/p\[0\] = NDC_HDR(NDC_NAPI, 1, 1);/p[0] = NDC_HDR(NDC_NRUN, 1, 1);/` on nd_rec.c into /tmp/nd_rec_nseed.c, link as LIBS_RS does, want exactly `stunit FAIL nat`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat4i_host.txt 2>&1`: PASS (stunit ok and both seeds). Commit `NAT4I 1: SCISSOR, GEOM and MW as native state records`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat4i_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT4I --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT4I: native SCISSOR GEOM MW records" > build3ds/out/nat4i_loop.txt 2>&1`.
4. `grep -H "c0src f\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT4I_cmp_*.txt > build3ds/out/nat4i_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT4I/pm_3ds.elf > build3ds/out/nat4i_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (4 drec equal). c0gate `calls 122 interp 34 state 23`. Fix rights: compile errors, missing prototypes or includes. A DIFF, a crash or a tool fault: stop, send nat4i_c0src.txt and the first DIFF line to oracle NATIVE2 with SendMessage, wait.
## Hand-back
Last commit `NAT4I 2: gate. 4 drec equal; SCISSOR GEOM MW native records` with `git add -f build3ds/out/nat4i_c0src.txt build3ds/out/nat4i_loop.txt`. It deletes this page. One SubagentHandback: hash, the gate line per bench, the c0gate line.
