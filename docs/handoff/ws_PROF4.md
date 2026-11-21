# ws_PROF4 (Sonnet): function profile lane: exclusive cycles per function per core over normal steps, top 30 per core as log lines
Oracle NATIVE18. Worktree build3ds/wt/PROF4, branch PROF4, PM_ROW=PROF4 PM_LANE=PROF4, start main cd9c1d7ff3 or later. Why Sonnet: a new lane in CMake and build.sh, a hook with thread identity and swapped tables, and an exclude list made from a census (a judgement loop).
Objective: name the untimed time (about 20 ms on core 0, 7 ms on core 1). Lane build only: the shipped build has no hook, no flag, no file (as the HC0 lane).
FACT: the census lane exists: 3ds/build.sh:24-29 PM_FNCOV, 3ds/CMakeLists.txt:383-400 (-finstrument-functions on nd_*.c, pm_gpu*.c, pm_tex.c, pm_comb.c and the
src/ files of tools/cov.d/*.txt), 3ds/src/pm_fncov.c (the hook), 3ds/tools/hotset.py --fncov (names). A clock read costs 0.99 us (svcGetSystemTick, cal line), a PMU read
1.16 us (svcControlPerformanceCounter, pm_phase.c:271): both are SVCs. The tick runs at the CPU clock (PF_TPM 268112 per ms): ticks = cycles.
## Edits
1. 3ds/tools/cov.d/PROF4.txt: src/main_loop.c, src/cam_main.c, src/model.c, src/entity.c, src/entity_model.c, src/animator.c, src/npc.c, src/77480.c,
   src/sprite.c, src/imgfx.c, src/effects.c, src/worker.c, src/hud_element.c, src/draw_box.c, src/inventory.c, src/msg.c, src/state_world.c (one per line).
2. 3ds/build.sh: PM_FPROF=1 as PM_FNCOV (:24-29: needs PM_LANE, OUT=build3ds/lane-${PM_LANE}_fprof, FPV=ON); pass -DPM_FPROF=$FPV and add it to the
   CMakeCache check (:373, :376). Both 1: exit 1 with a message.
3. 3ds/CMakeLists.txt after the PM_FNCOV block: option(PM_FPROF OFF). The same TU list (move the list into a block both options use). Flags on those TUs:
   -finstrument-functions -finstrument-functions-exclude-file-list=include/,3ds/include/,devkitPro and, when tools/fprof_exclude.txt has names,
   -finstrument-functions-exclude-function-list=<its names joined by commas>. Add src/pm_fprof.c (removed from PORT_SRC as pm_fncov.c), define PM_FPROF=1.
4. 3ds/src/pm_fprof.c (all functions no_instrument_function). State per thread: tls (getThreadLocalStorage of the main and the logic thread, set by
   pm_fprof_thread(k) from pm_main.c start and pm_logic.c lg_main start, under #ifdef PM_FPROF), d (instrumented depth), sp, last tick, stk[8], 2 table
   halves of 512 FpEnt {addr, n, u64 cyc} (16 B), calls, pairs. 32 KB bss in the lane only (no heap). Hook core:
       void __cyg_profile_func_enter(void* fn, void* cs) { FpSt* s = fp_me(); (void)cs; if (!s) return; /* another thread: no count */
           s->calls++; if (s->d == 0u && s->gen != fp_gen) fp_swap(s); /* window change at depth 0: the other half */
           if (s->d++ >= fp_dmax) return; /* deeper calls: their time stays with the timed caller */
           const unsigned t = (unsigned)svcGetSystemTick(); s->pairs++;
           if (s->sp) fp_slot(s, s->stk[s->sp - 1u])->cyc += t - s->last;
           s->stk[s->sp++] = (unsigned)fn; s->last = t; }
       void __cyg_profile_func_exit(void* fn, void* cs) { FpSt* s = fp_me(); (void)fn; (void)cs; if (!s) return;
           if (--s->d >= fp_dmax) return;
           const unsigned t = (unsigned)svcGetSystemTick(); FpEnt* e = fp_slot(s, s->stk[--s->sp]); e->cyc += t - s->last; e->n++; s->last = t; }
   fp_dmax: 4, launch word fprofd=<1..8> (lane only). fp_slot: open addressing on addr (as pm_fncov.c fn_slot); a full half counts a drop.
5. Report: pm_phase.c at the window print, `#ifdef PM_FPROF pm_fprof_window(pf_total, steps); #endif`. It sets fp_gen++. fp_swap marks the half it leaves as done; the window prints each
   done half (a thread that has not swapped yet prints at the next window): per core the top 30 by cyc, one line each (under 256 B): `fprof f<F> c<0|1> r<rank> <addr> ex <ms per step x100> n <calls per step x10>`,
   then `fprof f<F> c<k> calls <per step> pairs <per step> drop <n> ovh <ms per step x100>`, then clears the half and its mark. Boot line `fprof cal hook <ticks> pair <ticks>`:
   1000 direct untimed and timed hook pairs. ovh = (calls x hook + pairs x pair) / steps. Core 0 = main thread, core 1 = logic thread. 6. 3ds/tools/hotset.py: `--fprof <log>` joins the addresses with the map of build3ds/lane-<L>_fprof (as --fncov, lane_dir with the suffix).
## Steps. 1. Exclude census: `PM_FNCOV=1 PM_LANE=PROF4 sh 3ds/build.sh`; one run of scenario fncov_mac01_det on that lane; `hotset.py --fncov <log>`. Names with
   more than 300 calls per frame go to tools/fprof_exclude.txt (they stay inside their caller's time). Commit `PROF4 1`.
2. Gate. Shipped build: `PM_LANE=PROF4 sh 3ds/build.sh`, host test PASS, c0gate 0 / 0 / 0, `bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`,
   run and cmp `--row PROF4`: byte-equal. Lane: `PM_FPROF=1 PM_LANE=PROF4 sh 3ds/build.sh`; `python 3ds/tools/capture_check.py --row PROF4 --lane PROF4_fprof
   --proof "fprof cal,fprof f" --profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`: CAPTURE CHECK OK. Never add phase=on. 2 builds (2 binaries).
Overhead check (console, the user's push of the lane): the ovh line per core under 2 ms per step, and ft of the lane within 5 percent of the main
capture of the same words (live_cap2_170.txt ft 41.21). Over: lower fprofd or add names to fprof_exclude.txt from the n column; no rebuild for fprofd. Bias: the tick is wall time (audio preemption on core 1 counts to the function it interrupts).
Fix rights: build and link errors, the hook placement, hotset.py. A DIFF, a base fault, a crash: stop, send the path to oracle NATIVE18.
Stop: commit `PROF4 2: gate. <cmp line> <CAPTURE CHECK line> <cal line>` deletes this page. Hand-back: one SubagentHandback: commits, the results.
