# ws_CPT0 (Haiku): key show waits and GPU ms per frame (prof line c0k)
Designer: oracle NATIVE19 (plan_critpath.md 2.4). Worktree build3ds/wt/CPT0, branch CPT0. PM_ROW=CPT0 PM_LANE=CPT0. Start: main 400d2ad3ab or later; grep the quoted text.
Why Haiku: 2 tick sums, 2 drawing time sums, one print line, given below. No draw change (all code runs only with pm_pf_on).
FACT: GDW times the waits of the L frame begin only (3ds/src/pm_gpu.c:2990-2995). The key show has its own sync60_pace and C3D_FrameBegin (:3154-3155).
C3D_GetDrawingTime is the time from C3D_FrameEnd to the end of the GX queue (citro3d renderqueue.c:238-240, :44-49). Read after a C3D_FrameBegin, it is the GPU
time of the frame before. Objective: line `prof f<F> w<W> c0k kpace <ms> kgx <ms> gpul <ms> gpuk <ms> nk <n> nb <n>`. Counter: none moves; it decides KICK3 and PIPE.
## Edits (CRLF files: use the Edit tool)
1. 3ds/src/pm_gpu.c:2934. Before: `static u64 s60_next;`
   After: `unsigned long long pm_c0k[6]; /* CPT0: [0] key pace ticks, [1] key FrameBegin ticks, [2] L tail us, [3] K frame us, [4] key shows, [5] begins after a key (prof only) */`
   `static int c0k_kf; /* CPT0: the last C3D_FrameEnd was a key show */`
   `extern int pm_pf_on; /* pm_phase.c */`
   `static u64 s60_next;`
2. pm_gpu.c:2995. Before: `    if (pm_pf_on) pm_c0i_t[2] += (unsigned)svcGetSystemTick() - c0t_;`
   After: the same line, then
   `    if (pm_pf_on && c0k_kf) { pm_c0k[3] += (unsigned)(C3D_GetDrawingTime() * 1000.f); pm_c0k[5]++; } /* CPT0: the GPU time of the K frame */`
   `    c0k_kf = 0;`
3. pm_gpu.c:3154-3155 (pm_gpu_key_show). Before:
   `    if (pm_interp_on > 0 && !pm_wd_wake_frames) sync60_pace();`
   `    if (!C3D_FrameBegin(0)) { pm_kc_why[4]++; return; } /* FPS60W2: ...` (keep the comment)
   After:
   `    unsigned k0_ = pm_pf_on ? (unsigned)svcGetSystemTick() : 0u; /* CPT0 */`
   `    if (pm_interp_on > 0 && !pm_wd_wake_frames) sync60_pace();`
   `    if (pm_pf_on) { const unsigned n_ = (unsigned)svcGetSystemTick(); pm_c0k[0] += n_ - k0_; k0_ = n_; }`
   `    if (!C3D_FrameBegin(0)) { pm_kc_why[4]++; return; } /* FPS60W2: ...` (unchanged)
   `    if (pm_pf_on) { pm_c0k[1] += (unsigned)svcGetSystemTick() - k0_; pm_c0k[2] += (unsigned)(C3D_GetDrawingTime() * 1000.f); pm_c0k[4]++; } /* CPT0: the key wait; the GPU tail of the L frame */`
4. pm_gpu.c, the line `    rd_sync(); C3D_FrameEnd(GX_CMDLIST_FLUSH);` inside pm_gpu_key_show (:3175, the one after `C3D_FrameDrawOn(target);`): add after it
   `    c0k_kf = 1; /* CPT0 */`
5. 3ds/src/pm_phase.c:487. After `static unsigned long long c0i_m[3]; extern unsigned long long pm_c0i_t[3]; /* GDW */` add the line
   `static unsigned long long c0k_m[6]; extern unsigned long long pm_c0k[6]; /* CPT0 */`
   In pf_lg_mark (:488) replace `memcpy(c0i_m, pm_c0i_t, sizeof c0i_m);` with `memcpy(c0i_m, pm_c0i_t, sizeof c0i_m); memcpy(c0k_m, pm_c0k, sizeof c0k_m);`
6. pm_phase.c:703. After the c0i print line (it ends with `} /* GDW */`) add:
   `            { unsigned long long d_[6]; for (int k = 0; k < 6; k++) d_[k] = pm_c0k[k] - c0k_m[k];`
   `              const unsigned kp = pf_w ? (unsigned)(d_[0] * 100ull / PF_TPM / pf_w) : 0u, kg = pf_w ? (unsigned)(d_[1] * 100ull / PF_TPM / pf_w) : 0u;`
   `              const unsigned gl = d_[4] ? (unsigned)(d_[2] / 10ull / d_[4]) : 0u, gk = d_[5] ? (unsigned)(d_[3] / 10ull / d_[5]) : 0u; /* us to ms x 100 */`
   `              pm_log_prof_line("prof f%u w%u c0k kpace %u.%02u kgx %u.%02u gpul %u.%02u gpuk %u.%02u nk %u nb %u", pf_total, pf_w, kp / 100u, kp % 100u,`
   `                  kg / 100u, kg % 100u, gl / 100u, gl % 100u, gk / 100u, gk % 100u, (unsigned)d_[4], (unsigned)d_[5]); } /* CPT0 */`
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=CPT0 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`; run and cmp with `--row CPT0`: byte-equal.
3. `python 3ds/tools/capture_check.py --row CPT0 --proof "c0k kpace" --profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`: CAPTURE CHECK OK.
   Never add phase=on. The proof is the line (Azahar values do not count).
Fix rights: compile errors and declaration placement (pm_pf_on: one file-scope extern is enough). A DIFF or a base fault: stop, send the output path to oracle NATIVE19.
Stop: commit `CPT0 1: gate. <host> <c0gate> <cmp line> <CAPTURE CHECK line>` deletes this page. Hand-back: one SubagentHandback: commit hash, the results.
