# ws_PROF2 (Haiku: counters, byte-equal): split core 1 list build (c1s dl 43.9 ms) and core 0 model records (cls rec 6.15 ms) into timer classes
Designer: oracle NATIVE15 (plan_perf.md section 5, rows MCOPY, MDLHIT2). Branch from main. Worktree build3ds/wt/PROF2, branch PROF2. PM_ROW=PROF2 PM_LANE=PROF2.
Facts (console build 7569, FACT capture): c1s dl 43.9 ms per step; c1t snap 5.6, alt 8.2 (alt includes about 300 ticks per unit of timer cost);
so about 30 ms of the list build has no timer. Core 0 cls rec 6.15 ms for about 93 model records per frame, no sub-split.
The class timers are exclusive (a push stops the outer class, pm_gpu.c:1170-1196), and run only in sampled steps (NDT_B, nd.h:485).
Objective: the next console capture names where core 1 and the model hit spend time. No draw change. c0gate stays 0 / 0 / 1.
Proxy (Azahar): shots byte-equal; host PASS. Console numbers to read later: the new `c1d` and `c0r` lines.
Edit only (line numbers main c4648e7561): nd.h:471 (NDT enum); nd_rec.c (the bodies of nd_rec_model, nd_rec_spr, nd_rec_ent_draw, nd_rec_texrect, nd_rec_nr);
nd_recs.c:258, :262, :266-267, :270; pm_phase.c:608, :629-631. Sources are CRLF.
## Edits
1. nd.h:471 enum: insert `NDT_RMDL, NDT_RENT, NDT_RSPR, NDT_RNR, NDT_MKEY, NDT_MMTX, NDT_MHIT,` before `NDT_N`.
2. nd_rec.c core 1 (the pattern of nd_rec_alt :475, :486): first line of the body `NDT_B(tq);`, before each return and at the end `NDT_E(tq, <class>);`.
   nd_rec_model: NDT_RMDL. nd_rec_ent_draw: NDT_RENT. nd_rec_spr and nd_rec_texrect: NDT_RSPR.
   nd_rec_nr: only the outer call times. Add `static unsigned char rnr_d; unsigned nd_rnr_n;` above it. Body start:
   `const int tq = pm_ph_on && !rnr_d; if (__builtin_expect(tq, 0)) pm_gpu_ndt_push(); rnr_d++; nd_rnr_n++;` and before every return and at the end:
   `rnr_d--; if (__builtin_expect(tq, 0)) pm_gpu_ndt_pop(NDT_RNR);`. Example: `if (...) { word_bad(w0); return; }` becomes `if (...) { word_bad(w0); rnr_d--; if (__builtin_expect(tq, 0)) pm_gpu_ndt_pop(NDT_RNR); return; }`.
   Put the push after `const uint32_t t[2] = { w0, w1 };` and keep every existing line.
3. nd_recs.c rec_run: wrap :258 (pm_gpu_model_keyb) and :262 (nd_mc_find_rec) in one `NDT_B(tk); ... NDT_E(tk, NDT_MKEY);`, :266 rec_matrix in NDT_MMTX,
   :267 the pm_gpu_model_hit call in NDT_MHIT (`NDT_B(th); const int hit_ = pm_gpu_model_hit(&mk, mce); NDT_E(th, NDT_MHIT); if (!hit_) return rec_miss(...);`),
   :270 hit_apply in NDT_MHIT too (a second pair).
4. pm_phase.c:608: csum also adds the 3 core 0 classes: `if (k <= NDT_LIST || k == NDT_MKEY || k == NDT_MMTX || k == NDT_MHIT) csum += cm[k];` (rest stays the unsplit walk).
   After the c1t line (:631) add 2 lines:
   `prof f%u w%u c1d rmdl %u.%02u rent %u.%02u rspr %u.%02u rnr %u.%02u n %u` (cm of the 4 core 1 classes; n = nd_rnr_n since the last window per step, mark it in pf_lg_mark),
   `prof f%u w%u c0r key %u.%02u mtx %u.%02u hit %u.%02u` (cm of NDT_MKEY, NDT_MMTX, NDT_MHIT). Use the CS macro of :618.
## Checklist (hw_guidelines.md rule 9)
- Timers only when pm_ph_on (sampled steps): no cost in other steps. One counter store per nd_rec_nr call (as nd_altu_n).
## Tests (output to build3ds/out/, read the last 20 lines; 1 build, at most 2 Azahar starts)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/prof2_host.txt` (timeout 600000): PASS (stunit, nat5a link nd_rec.c: keep their sed strings intact). Commit `PROF2 1: timer classes`.
2. `PM_LANE=PROF2 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py build3ds/pm_3ds.elf`: 0 / 0 / 1.
3. Base, run and cmp (`bench.py base <bench> --commit $(git merge-base HEAD main)`, `bench.py run spot_mac01_drec,entcape_kmr03_drec --row PROF2`, `bench.py cmp`): every shot byte-equal.
   Report any `c1d` or `c0r` line of the run logs (spot_mac01_drec has prof=4; a missing line is not a failure).
Fix rights: the edited lines, compile errors. A DIFF: stop, send the output path to oracle NATIVE15.
Stop: last commit `PROF2 2: gate` deletes this page. One SubagentHandback: commit hash, host, c0gate, cmp, the c1d and c0r lines if present.
