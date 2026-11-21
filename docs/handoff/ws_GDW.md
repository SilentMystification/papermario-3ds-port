# ws_GDW (Haiku): core 0 wait census: the game thread's idle, pace and GX waits inside wk (GAMEDRAW)
Designer: oracle NATIVE18 (plan_gamedraw.md 1.5). Worktree build3ds/wt/GDW, branch GDW. PM_ROW=GDW PM_LANE=GDW. Start: main 353b31c792 or later; grep the quoted text.
Why Haiku: 3 tick sums and one print line, given below. No draw change.
FACT: wk is step_start to now (3ds/src/pm_main.c:760). It holds core 0 work and 3 waits that no line shows: the game thread's waits for the logic
(pm_wq_wait_walker, called from pm_logic.c:114, :128, :142, :166, :173; wwait counts only the :153 wait), sync60_pace and the GX queue wait of
C3D_FrameBegin (pm_gpu.c:2989-2991). So "core 0 = wk - wwait" is not core 0 work. Objective: line `prof f<F> w<W> c0i idle <ms> pace <ms> gx <ms>`
per drawn step. Core 0 work = wk - idle - pace - gx - wwait. Counter: none moves; the line decides GD8 (plan section 3).
## Edits (CRLF files: use the Edit tool)
1. 3ds/src/nd_wq.c:54. Before: `void pm_wq_wait_walker(long long ns) { LightEvent_WaitTimeout(&wq_ev_walker, ns); }`
   After: `unsigned long long pm_c0i_t[3]; /* GDW: game thread ticks: [0] waits for the logic, [1] sync60_pace, [2] C3D_FrameBegin (prof only) */`
   `void pm_wq_wait_walker(long long ns) { const unsigned t0 = pm_pf_on ? (unsigned)svcGetSystemTick() : 0u; LightEvent_WaitTimeout(&wq_ev_walker, ns); if (pm_pf_on) pm_c0i_t[0] += (unsigned)svcGetSystemTick() - t0; }`
   (pm_pf_on is declared at nd_wq.c:85: move that line above :54 if the compiler needs it.)
2. 3ds/src/pm_gpu.c:2989-2991. Before: `if (pm_interp_on > 0 && !pm_wd_wake_frames) sync60_pace();` and `raw1_reset(); sh_clear(); C3D_FrameBegin(fb_flags);`
   After: `extern unsigned long long pm_c0i_t[3]; extern int pm_pf_on; unsigned c0t_ = pm_pf_on ? (unsigned)svcGetSystemTick() : 0u; /* GDW */`
   `if (pm_interp_on > 0 && !pm_wd_wake_frames) sync60_pace();`
   `if (pm_pf_on) { const unsigned n_ = (unsigned)svcGetSystemTick(); pm_c0i_t[1] += n_ - c0t_; c0t_ = n_; }`
   `raw1_reset(); sh_clear(); C3D_FrameBegin(fb_flags);`
   `if (pm_pf_on) pm_c0i_t[2] += (unsigned)svcGetSystemTick() - c0t_;`
   (put the 2 externs at file scope if a declaration inside the function clashes.)
3. 3ds/src/pm_phase.c: `static unsigned long long c0i_m[3]; extern unsigned long long pm_c0i_t[3];` near :486. pf_lg_mark (:487): add
   `memcpy(c0i_m, pm_c0i_t, sizeof c0i_m);`. After the `core2 lgms` print (:699-700) add
   `{ unsigned c_[3]; for (int k = 0; k < 3; k++) c_[k] = pf_w ? (unsigned)((pm_c0i_t[k] - c0i_m[k]) * 100ull / PF_TPM / pf_w) : 0u;`
   `  pm_log_prof_line("prof f%u w%u c0i idle %u.%02u pace %u.%02u gx %u.%02u", pf_total, pf_w, c_[0] / 100u, c_[0] % 100u, c_[1] / 100u, c_[1] % 100u, c_[2] / 100u, c_[2] % 100u); } /* GDW */`
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=GDW sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`; run and cmp with `--row GDW`: byte-equal.
3. `python 3ds/tools/capture_check.py --row GDW --proof "c0i idle" --profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`: CAPTURE CHECK OK.
   Never add phase=on. The proof is the line (Azahar values do not count).
Fix rights: compile errors and declaration placement. A DIFF or a base fault: stop, send the output path to oracle NATIVE18.
Stop: commit `GDW 1: gate. <host> <c0gate> <cmp line> <CAPTURE CHECK line>` deletes this page. Hand-back: one SubagentHandback: commit hash, the results.
