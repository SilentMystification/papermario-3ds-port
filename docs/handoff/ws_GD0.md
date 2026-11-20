# ws_GD0 (Haiku): per core 1 draw class, the time with its recorder work in (c1G) and its unit count (c1u): the census of GAMEDRAW
Designer: oracle NATIVE18 (plan_gamedraw.md section 6). Worktree build3ds/wt/GD0, branch GD0. PM_ROW=GD0 PM_LANE=GD0. Start: main 4d12301af5 or later; grep the quoted text.
Why Haiku: 2 counters in the timer stack and 2 print lines, all given below. No draw change.
Objective: c1g (PROF3) gives each class (ent, mdl, ply, npc, wrk, fx, task, thud, ui) its time less the recorder regions inside it. Add per class the
whole time and the nd_rec_nr calls, per sampled step. New prof lines: `prof f<F> w<W> c1G ent <ms> mdl <ms> ... ui <ms>` and `prof f<F> w<W> c1u ent <n> ... ui <n>`.
Counter: none moves. The lines pick GD2 to GD6 (plan_gamedraw.md section 3).
## Edits (CRLF files: use the Edit tool)
1. 3ds/src/pm_gpu.c, after `static unsigned ndt_sp;` (:1177) add
   `static uint64_t ndt_gi[NDT_GUI - NDT_GENT + 1]; static unsigned ndt_gu[NDT_GUI - NDT_GENT + 1], ndt_u[4]; extern unsigned nd_rnr_n; /* GD0 */`
2. pm_gpu_ndt_push (:1182). Before: `if (ndt_sp < 4u) { ndt_t0[ndt_sp] = (uint32_t)svcGetSystemTick(); ndt_in[ndt_sp] = 0; }`
   After: `if (ndt_sp < 4u) { ndt_t0[ndt_sp] = (uint32_t)svcGetSystemTick(); ndt_in[ndt_sp] = 0; ndt_u[ndt_sp] = nd_rnr_n; }`
3. pm_gpu_ndt_pop (:1190). After `ndt_sum[k] += tot > in ? tot - in : 0u;` add
   `if (k >= NDT_GENT && k <= NDT_GUI) { ndt_gi[k - NDT_GENT] += tot; ndt_gu[k - NDT_GENT] += nd_rnr_n - ndt_u[i]; } /* GD0 */`
4. After pm_gpu_ndt_take (:1201) add `void pm_gpu_ndt_take_g(uint64_t o[9], unsigned u[9])`: copy ndt_gi to o and ndt_gu to u, then zero both.
   Prototype in 3ds/include/nd.h after `void pm_gpu_ndt_pop(int k);`.
5. 3ds/src/pm_phase.c window start, after `{ uint64_t ct[NDT_N]; pm_gpu_ndt_take(ct); }` (:491) add `{ uint64_t g_[9]; unsigned u_[9]; pm_gpu_ndt_take_g(g_, u_); } /* GD0 */`.
6. pm_phase.c, right after the c1g print (:636-638, same block):
       { uint64_t gi[9]; unsigned gu[9], gm[9]; pm_gpu_ndt_take_g(gi, gu); /* GD0 */
         for (int k = 0; k < 9; k++) { gm[k] = pf_cs(gi[k], pf_s); gu[k] = pf_s ? gu[k] / pf_s : 0u; }
         pm_log_prof_line("prof f%u w%u c1G ent %u.%02u mdl %u.%02u ply %u.%02u npc %u.%02u wrk %u.%02u fx %u.%02u task %u.%02u thud %u.%02u ui %u.%02u",
             pf_total, pf_w, CS(gm[0]), CS(gm[1]), CS(gm[2]), CS(gm[3]), CS(gm[4]), CS(gm[5]), CS(gm[6]), CS(gm[7]), CS(gm[8]));
         pm_log_prof_line("prof f%u w%u c1u ent %u mdl %u ply %u npc %u wrk %u fx %u task %u thud %u ui %u", pf_total, pf_w,
             gu[0], gu[1], gu[2], gu[3], gu[4], gu[5], gu[6], gu[7], gu[8]); }
   The divide runs once per window in a cold print path (hw_guidelines.md rule 4 covers hot code only).
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS.
2. `PM_LANE=GD0 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: calls 0 interp 0 state 0.
3. `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`, `bench.py run spot_mac01_drec --row GD0 --why "GD0 census only"`,
   `bench.py cmp spot_mac01_drec --row GD0`: byte-equal.
4. `python 3ds/tools/capture_check.py --row GD0 --proof "c1G ent,c1u ent" --profile prof warp=mac_01:0 pos=0:0:200 profdisk=1`: CAPTURE CHECK OK.
   Never add phase=on (plan_60fps.md section 5). Azahar timers read 0.00: the proof is the line.
Fix rights: compile errors; a missing line in the capture check (the print is not reached: read where the c1g line prints). A DIFF, a base fault: stop,
send the output path to oracle NATIVE18.
Stop: commit `GD0 1: gate. <host> <c0gate> <cmp line> <CAPTURE CHECK line>` deletes this page. Hand-back: one SubagentHandback: commit hash, the 4 results.
