# ws_PROF4B (Haiku): PROF4 lane overhead under 2 ms per step per core: a counts census in Azahar picks the exclude list and the depth
Oracle NATIVE18. Worktree build3ds/wt/PROF4B, branch PROF4B, PM_ROW=PROF4B PM_LANE=PROF4B. Start: main after the PROF4 merge (c265e0f80b in it).
Why Haiku: a counts mode in the hook, 2 print lines and a Python filter, all given below; the numbers decide, not judgement.
FACT (PROF4 Azahar capture): core 0 calls 41715 and pairs 1615 per step, hook 53 ticks per untimed call, 838 per timed pair: ovh 13.03 ms. Budget: 2 ms =
536K ticks per step. Target: calls x 53 + pairs x 838 at most 480K per core (for example 5000 calls and 260 pairs). Calls and pairs are counts: Azahar
gives the same counts as the console for the same words, so the whole tuning runs in Azahar. Only the final profile needs the console (1 push).
FACT (GCC manual): -finstrument-functions-exclude-function-list matches a substring of a function name, so a listed name also drops every longer name
that contains it. The filter reports those.
## Edits (lane code only: 3ds/src/pm_fprof.c and 3ds/tools; nothing in the shipped build)
1. pm_fprof.c: launch word `fprofn` (read in pm_fprof_thread next to fprofd) sets fp_cnt = 1. FpHalf gets `unsigned dn[9]` (calls per depth, 8 = 8 and up).
   In __cyg_profile_func_enter, before `if (s->d++ >= fp_dmax) return;` add:
       if (fp_cnt) { FpEnt* e = fp_slot(s, (unsigned)fn); e->n++; e->cyc += s->d; s->h[s->cur].dn[s->d < 8u ? s->d : 8u]++; s->d++; return; } /* PROF4B: counts only */
   In __cyg_profile_func_exit, after `if (!s || !s->d) return;` add `if (fp_cnt) { s->d--; return; }`. (cyc holds the depth sum: mean depth = cyc / n.)
2. fp_print: when fp_cnt, print every entry with n >= steps (1 per step and up), not the top 30:
   `fprofn f%u c%u %08x n %u dsum %u` (n and the depth sum per window), then `fprofn f%u c%u steps %u calls %u d %u %u %u %u %u %u %u %u %u` (dn[0..8]).
3. New 3ds/tools/fprof_excl.py <log> [--lane PROF4B] [--max 60] [--budget 480000]: names from the map of build3ds/lane-<L>_fprof (hotset.py lane_dir and its
   map reader). Per core: per-step n of each function. Exclude a name when n per step > max on either core. A kept name that contains an excluded name
   as a substring is excluded too: list it as `collateral <name>`. Predict per core: calls = sum of kept n; pairs(D) = sum of kept n with mean depth < D.
   Pick the largest D (1 to 6) with calls x 53 + pairs(D) x 838 <= budget. Write 3ds/tools/fprof_exclude.txt (one name per line, sorted) and print
   `fprof_excl names <k> collateral <c> core0 calls <x> pairs <y> core1 calls <x> pairs <y> D <d> ticks0 <t> ticks1 <t>`. No D fits: lower --max by 20 and
   repeat (down to 20); still none: stop and send the line to oracle NATIVE18.
## Steps (output to build3ds/out/, read the last 20 lines)
1. `PM_FPROF=1 PM_LANE=PROF4B sh 3ds/build.sh`. Census: `python 3ds/tools/capture_check.py --row PROF4B --lane PROF4B_fprof --proof "fprofn f"
   --profile prof fps60 fprofn warp=mac_01:0 pos=0:0:200 profdisk=1` (both threads, whole steps, one window). `python 3ds/tools/fprof_excl.py <log>`.
   Commit `PROF4B 1` with fprof_exclude.txt and the fprof_excl line.
2. Rebuild the lane. Check: the same capture_check without fprofn, with `fprofd=<D>`, proof "fprof f". Pass: each `fprof f<F> c<k> calls .. pairs ..` line
   gives calls x 53 + pairs x 838 <= 536K (2 ms). Over: step 1 again with --max lowered by 20 (3 rounds), then stop and send the lines to oracle NATIVE18.
3. Shipped build: `PM_LANE=PROF4B sh 3ds/build.sh`, host test PASS, c0gate 0 / 0 / 0, spot_mac01_drec base (merge-base) and cmp: byte-equal.
   Azahar runs: 2 lane runs + 1 bench run (+ 2 per extra round, budget 8).
## What the user pushes, in order (after the merge; the orchestrator writes it into push_runbook form)
1. Build the lane on main: `PM_FPROF=1 PM_LANE=PROF4B sh 3ds/build.sh`. 2. Push it: `$env:PM_LANE='PROF4B_fprof'` then
   `powershell -NoProfile -File 3ds/go.ps1 push o -ip <addr> -NoBuild -Seconds 75` with the words `--profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1
   fprofd=<D>` (D from the commit). One push. Read: `fprof cal` (the console hook and pair ticks) and per core the ovh line: under 2 ms per step is usable.
   Over 2 ms on the console: no second push; send the lines to oracle NATIVE18 (the console cal line rescales the budget, step 1 reruns in Azahar).
Fix rights: compile errors, the filter script, the map reader. A DIFF, a base fault, a crash: stop, send the output path to oracle NATIVE18.
Stop: commit `PROF4B 2: gate. <fprof_excl line> <check lines> <cmp line>` deletes this page. Hand-back: one SubagentHandback: commits, D, the lines.
