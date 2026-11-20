# ws_NAT6C (Haiku: 7 files, stream only, byte-equal): the last stream NRUN units become native records
WAIT: `Merge NAT6A` (text overlap: this row rewrites the 2 lines that NAT6A writes in nd_rec_nr; it also uses the NAT6A runners nd_oml_run, nd_prim_run and the flag cg_keep).
Objective: after NAT6A the stream NRUN records hold 2 kinds of units: the EP mark of a dropped marked combine unit (st_put in nd_rec_nr) and the unmarked combine class units of a custom gfx range (cg_keep). NAT6C writes the mark as a native record nd_epm_run and those units as native records (nd_oml_run, nd_prim_run, new nd_cbd_run). nd_r11_walk reads the 3 unit records. Core 0 does the same work: exact. Counter C13: no change (FACT: no new interp call site). interp and state stay. It prepares NAT6D.
Start commit: main after Merge NAT6A. Worktree build3ds/wt/NAT6C, branch NAT6C. PM_ROW=NAT6C PM_LANE=NAT6C. Designer: oracle NATIVE9 (id in the launch prompt). Regions (find each line by its text): nd_rec.c (the NAT6A weak stub line, the 2 NAT6A lines in nd_rec_nr), nd_walk.c (new lines after the NAT6A runner nd_prim_run), nd_model.c (nd_r11_walk, the NDC_NAPI case, :288-290 on e5ae433d74), nd_api.h (after the NAT6A prototype line), 3ds/tools/nd_host_test.sh (the weak stub block of /tmp/ndcombs.c, :192-194), 3ds/tools/nd_st_test.c (the "stunit FAIL run" check), 3ds/tests/host.d/stunit.sh:12. NAT6B regions (nd_alt_put, nr_set, ls_run end, nd_ds.c) do not overlap.
## Facts (main e5ae433d74 plus ws_NAT6A)
- FACT nr_ctl runs a MISC_EP unit as: pm_gbi_cur_op = ND_FS_MISC, no gv_epoch at the top, then ep_mark = 1, ep_mw = b, nd_epk_mk++, gv_epoch++ (nd_walk.c:313-316, :338).
- FACT nr_set at ep_stream and ex_depth 0 runs ENV, FOG, BLEND, FILL, PRIMZ, RARE, COMB as nd_unit_same (no state change, nd_set.c:136-147) and ep_mark = 0. NAT6A runs OML and PRIM with nd_oml_run and nd_prim_run.
- FACT nd_r11_walk (nd_model.c:277-305) feeds each unit of a custom gfx range to nd_r11_op and counts it (i < 16). An NDC_NAPI record with another runner returns 0 (:290), as a MISC_EP unit does (nd_r11_op default, :273).
- FACT stunit.sh:12 seeds `p[0] = NDC_HDR(NDC_NAPI, 1, 0);` (st_put) and wants `stunit FAIL run`. stunit.sh:17 seeds `NDC_HDR(NDC_NAPI, 1, 1)` (rec_nat) and wants `stunit FAIL nat`. The run check (nd_st_test.c:156-157) runs before the nat checks.
## Edits (each new line ends with `/* NAT6C */`)
1. nd_rec.c, nd_rec_nr. Before (NAT6A text):
```c
        if (!rc && !cg_keep && (id == ND_FS_OML || (id >= ND_FS_PRIM && id <= ND_FS_PRIMZ) || id == ND_FS_RARE || id == ND_FS_COMB)) { if (id == ND_FS_OML) rec_nat(nd_oml_run, w0 & 0xffffffu, w1); else if (id == ND_FS_PRIM) rec_nat(nd_prim_run, w0 & 0xffffffu, w1); return; } /* NAT6A */
        if (rc) { st_put(ND_FS_W0(ND_FS_MISC, ND_FS_MISC_EP << 16), rc | (lost ? 0x100u : 0u) | (dirty ? 0x200u : 0u) | (ch & 1u) << 10); /* NAT4B: the cut and its reason */
```
   After:
```c
        if (!rc && (id == ND_FS_OML || (id >= ND_FS_PRIM && id <= ND_FS_PRIMZ) || id == ND_FS_RARE || id == ND_FS_COMB)) { if (id == ND_FS_OML) rec_nat(nd_oml_run, w0 & 0xffffffu, w1); else if (id == ND_FS_PRIM) rec_nat(nd_prim_run, w0 & 0xffffffu, w1); else if (cg_keep) rec_nat(nd_cbd_run, w0, w1); return; } /* NAT6A; NAT6C: a custom gfx range keeps the others as data records */
        if (rc) { rec_nat(nd_epm_run, 0u, rc | (lost ? 0x100u : 0u) | (dirty ? 0x200u : 0u) | (ch & 1u) << 10); /* NAT4B: the cut and its reason; NAT6C: a native record */
```
2. nd_rec.c, after the NAT6A weak stub line: `__attribute__((weak)) void nd_epm_run(const uint32_t* r) { (void)r; } __attribute__((weak)) void nd_cbd_run(const uint32_t* r) { (void)r; } /* NAT6C: strong in nd_walk.c */`. nd_api.h after the NAT6A line: `void nd_epm_run(const uint32_t* r); void nd_cbd_run(const uint32_t* r); /* NAT6C, nd_walk.c, core 0: the stream mark, a combine unit of a custom gfx range */`.
3. nd_walk.c after nd_prim_run:
```c
void nd_epm_run(const uint32_t* r) { pm_gbi_cur_op = ND_FS_MISC; ep_mark = 1; ep_mw = r[1]; nd_epk_mk++; gv_epoch++; } /* NAT6C: nr_ctl MISC_EP */
void nd_cbd_run(const uint32_t* r) { (void)r; ep_mark = 0; } /* NAT6C: nr_set of a pure combine unit at ep_stream, ex_depth 0 */
```
4. nd_model.c nd_r11_walk, in case NDC_NAPI before the line `if (p[1] != (uint32_t)(uintptr_t)nd_nr_run && ...) return 0;`:
```c
            if (p[1] == (uint32_t)(uintptr_t)nd_oml_run || p[1] == (uint32_t)(uintptr_t)nd_prim_run || p[1] == (uint32_t)(uintptr_t)nd_cbd_run) { /* NAT6C: a unit record of a custom gfx range */
                const uint32_t w0 = p[1] == (uint32_t)(uintptr_t)nd_oml_run ? ND_FS_W0(ND_FS_OML, p[2]) : p[1] == (uint32_t)(uintptr_t)nd_prim_run ? ND_FS_W0(ND_FS_PRIM, p[2]) : p[2];
                if (i >= 16u || !nd_r11_op(w0, p[3], pre, &pend, has_cb, cw, mo, nmo)) return 0; i++; break; }
```
5. nd_host_test.sh, after the NAT5G weak stub line of /tmp/ndcombs.c: `__attribute__((weak)) void nd_oml_run(const uint32_t* q) { (void)q; } __attribute__((weak)) void nd_prim_run(const uint32_t* q) { (void)q; } __attribute__((weak)) void nd_cbd_run(const uint32_t* q) { (void)q; } /* NAT6C: nd_r11_walk names them */`.
6. nd_st_test.c run check. Before: `if (b.nw < 4u || b.w[0] != NDC_HDR(NDC_NAPI, 1, 0) || b.w[1] != (uint32_t)(uintptr_t)nd_nr_run || b.w[2] != ND_FS_W0(ND_FS_MISC, ND_FS_MISC_EP << 16)` and `|| b.nu != 2u || b.u[0] != b.w[2])`. After: `if (b.nw < 4u || b.w[1] != (uint32_t)(uintptr_t)nd_epm_run || b.w[2] != 0u || b.nu != 0u) { printf("stunit FAIL run\n"); return 1; } /* NAT6C: the mark is an nd_epm_run record (the header word is not tested: the nat seed changes it) */`. Declare `void nd_epm_run(const uint32_t* r);` next to `void nd_omh_run` (:146).
7. stunit.sh:12. Before: `sed 's/p\[0\] = NDC_HDR(NDC_NAPI, 1, 0);/p[0] = NDC_HDR(NDC_NRUN, 1, 0);/' 3ds/src/nd_rec.c > /tmp/nd_rec_seed.c`. After: `sed 's/rec_nat(nd_epm_run, 0u,/rec_nat(nd_cbd_run, 0u,/' 3ds/src/nd_rec.c > /tmp/nd_rec_seed.c # NAT6C: the mark record with a wrong runner`. Keep its wanted output `stunit FAIL run`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, foreground only, 1 build, 8 Azahar starts)
1. Edits 1-7. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat6c_host.txt 2>&1`: PASS. Commit `NAT6C 1: the last stream NRUN units become native records`.
2. `python 3ds/tools/bench.py base spot_mac01_drec > build3ds/out/nat6c_base1.txt 2>&1`, the same for wscmb_pause_drec (nat6c_base2.txt). spot_mac01: custom gfx ranges (R11) and marks. wscmb_pause: combine marks of the pause and text stream. 3. `sh 3ds/tools/rowloop.sh --row NAT6C --bench spot_mac01_drec,wscmb_pause_drec --no-host --why "NAT6C: stream marks and custom gfx units as records" > build3ds/out/nat6c_loop.txt 2>&1`.
4. `mkdir -p build3ds/out/c0base && git archive $(git merge-base HEAD main) | tar -x -C build3ds/out/c0base`, then `python build3ds/out/c0base/3ds/tools/c0gate.py build3ds/lane-NAT6C/pm_3ds.elf` and `python 3ds/tools/c0gate.py build3ds/lane-NAT6C/pm_3ds.elf`, both into build3ds/out/nat6c_c0.txt.
## Gate (exact): host PASS; LOOP PASS (2 benches byte-equal); the 2 c0gate lines are equal. Fix rights: compile errors, prototypes, weak stubs, the stunit check text. A DIFF, a crash, other c0gate values or a tool fault: stop. Send the cmp path and its first DIFF line to oracle NATIVE9 with SendMessage. Wait.
## Hand-back: last commit `NAT6C 2: gate. <lines>` with `git add -f build3ds/out/nat6c_loop.txt build3ds/out/nat6c_c0.txt`. It deletes this page. One SubagentHandback: hashes, cmp line per bench, c0gate lines.
