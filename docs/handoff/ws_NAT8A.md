# ws_NAT8A (Haiku: 3 files, prof lines only, no behavior change): two prof lines show the core 1 snap, list table and alt range cost.
Designer: oracle NATIVE7. WAIT: `Merge NAT5T` (same files nd_rec.c, nd_snap.c). Start commit: main after it. Worktree build3ds/wt/NAT8A, branch NAT8A. PM_ROW=NAT8A PM_LANE=NAT8A. Owns: 3ds/include/nd.h (enum line), 3ds/src/nd_snap.c, 3ds/src/nd_rec.c, 3ds/src/pm_phase.c. Counter: none moves (c0gate stays equal to the base).
## Facts (main 9576c03383; line numbers move after NAT5T, find each place by its symbol)
- FACT the NDT timers run only in sampled frames (nd.h:458-460, `pm_ph_on`). In a sampled frame the logic step runs inline on core 0 (pm_main.c:533 lp_serial, pm_logic.c:135), so a timer in core 1 code measures that step. The report reads them at pm_phase.c:604-606.
- FACT nd_snb, nd_snn, nd_snm are totals since boot (nd_snap.c:29, :45, :54, :58). No host unit links nd_snap.c or pm_phase.c. Host units link nd_rec.c (3ds/tests/host.d/*.sh).
## Edits (every new line ends with `/* NAT8A */`). One commit.
1. nd.h:452: `NDT_LIST, NDT_SNAP, NDT_LSC, NDT_ALT, NDT_N`. pm_phase.c:606: `csum += cm[k]` only when `k <= NDT_LIST` (rest stays the walk less the core 0 classes).
2. nd_snap.c: new globals `unsigned nd_lsf_n, nd_lsb_n, nd_lse_n;` after :29. Shape A (wrapper, the body stays as it is). Before (:40): `static int snap_build(const NdDs* s, int zc, NdSnapChk* rp) {`. After: rename it `snap_build0`, and after its closing line :67 add
   `static int snap_build(const NdDs* s, int zc, NdSnapChk* rp) { NDT_B(nt8); const int r = snap_build0(s, zc, rp); NDT_E(nt8, NDT_SNAP); return r; } /* NAT8A */`
   Do the same for `ls_find` (:74, rename `ls_find0`; the wrapper does `nd_lsf_n++;` first, class NDT_LSC, returns the unsigned) and for `nd_lsc_end_at` (:89-91, rename `static void lsc_end0`; wrapper `void nd_lsc_end_at(uint32_t* dst) { nd_lse_n++; NDT_B(nt8); lsc_end0(dst); NDT_E(nt8, NDT_LSC); }`). In ls_find0 put `nd_lsb_n++;` at the start of the line that takes the new block (:79).
3. nd_rec.c: new globals `unsigned nd_altu_n, nd_altr_n;`. Shape B (no early return). Before (:450-452): `void nd_alt_put(uint32_t w0, uint32_t w1) {` ... `st_put(w0, w1); }`. After:
   `void nd_alt_put(uint32_t w0, uint32_t w1) { nd_altu_n++; NDT_B(nt8); /* NAT8A */` then the old lines (the NAT5T swap line included), then `st_put(w0, w1); NDT_E(nt8, NDT_ALT); /* NAT8A */ }`.
   Same in nd_rec_alt (:435-447): `nd_altr_n++; NDT_B(nt8);` as the first statement, `NDT_E(nt8, NDT_ALT);` as the last statement.
   Host link: in the weak block (:613-623) add `__attribute__((weak)) int pm_ph_on; __attribute__((weak)) void pm_gpu_ndt_push(void) {} __attribute__((weak)) void pm_gpu_ndt_pop(int k) { (void)k; } /* NAT8A: strong in pm_phase.c, pm_gpu.c */`.
4. pm_phase.c. After pf_c1_now (:479-483): `extern unsigned nd_snb, nd_snn, nd_snm, nd_lsf_n, nd_lsb_n, nd_lse_n, nd_altu_n, nd_altr_n; static unsigned lg0_c[8];` and `static void pf_c1n_now(unsigned* x)` that writes those 8 values in that order into x[0..7]. pf_lg_mark (:484) also calls `pf_c1n_now(lg0_c);`.
   After the cnt line (:622-623), inside the same block (CS is defined there):
```c
        { /* NAT8A: core 1 recorder timers of the sampled step, ms x 100 per sampled frame; cal = ticks inside one empty timer pair */
            for (int k = 0; k < 16; k++) { pm_gpu_ndt_push(); pm_gpu_ndt_pop(NDT_ALT); }
            uint64_t c2[NDT_N]; pm_gpu_ndt_take(c2);
            pm_log_prof_line("prof f%u w%u c1t snap %u.%02u lsc %u.%02u alt %u.%02u cal %u", pf_total, pf_w,
                CS(cm[NDT_SNAP]), CS(cm[NDT_LSC]), CS(cm[NDT_ALT]), (unsigned)(c2[NDT_ALT] >> 4));
        }
```
   After the core2 drain line (:685-686), inside the same block (dn is defined there), 11 arguments:
   `{ unsigned c[8]; pf_c1n_now(c); pm_log_prof_line("prof f%u w%u c1n n %u snb %u snn %u snm %u lsf %u lsb %u lse %u altu %u altr %u", pf_total, pf_w, dn, c[0] - lg0_c[0], c[1] - lg0_c[1], c[2] - lg0_c[2], c[3] - lg0_c[3], c[4] - lg0_c[4], c[5] - lg0_c[5], c[6] - lg0_c[6], c[7] - lg0_c[7]); } /* NAT8A: window totals over n logic steps */`
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`)
1. Commit the edits. `python 3ds/tools/bench.py base fxparts_drec > build3ds/out/nat8a_base.txt 2>&1` (the base is the merge-base).
2. `sh 3ds/tools/rowloop.sh --row NAT8A --bench fxparts_drec --why "NAT8A: c1t c1n prof lines" > build3ds/out/nat8a_loop.txt 2>&1` (host test, lane build, 1 run, cmp against `git merge-base HEAD main`).
3. `PM_ROW=NAT8A PM_LANE=NAT8A python 3ds/tools/capture_check.py --row NAT8A --proof "c1t snap,c1n n" --profile prof warp=mac_01:0 pos=0:0:200 profdisk=1 > build3ds/out/nat8a_cap.txt 2>&1`. Then `grep -h "c1t \|c1n " <log path of the CAPTURE CHECK line> > build3ds/out/nat8a_lines.txt`.
4. Seeded fault: `python -c "import sys; sys.path.insert(0, '3ds/tools'); import capture_check as c; print(c.judge(open(sys.argv[1], encoding='utf-8', errors='replace').read(), ['c1t snapQ'], prof=True)[0])" <same log path>` must print `False`.
Budget: 1 build, 3 Azahar starts (base, run, capture).
## Gate (exact): host PASS; fxparts_drec byte-equal; CAPTURE CHECK OK; the seeded fault prints False; nat8a_lines.txt has at least one c1n line with snb above 0 and altu above 0; memcheck OK. Values are not judged.
Fix rights: compile errors, includes, host stubs, a lost `/* NAT8A */` mark. A DIFF, a FAIL, memcheck OVER, a crash or a tool fault: stop, send the output path and its first bad line to oracle NATIVE7 with SendMessage, wait.
## Hand-back: last commit `NAT8A 2: gate. <lines>` with `git add -f build3ds/out/nat8a_lines.txt build3ds/out/nat8a_loop.txt`. It deletes this page. One SubagentHandback: hashes, cmp line, CAPTURE CHECK line, first c1t and c1n line.
