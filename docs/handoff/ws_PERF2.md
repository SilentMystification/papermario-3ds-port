# ws_PERF2 (Haiku: 3 lines, byte-equal): the audio control and mixer threads run on core 0 by default; switch audiocore1 keeps core 1
Designer: oracle NATIVE15 (plan_perf.md F4, section 8). Branch from main. Worktree build3ds/wt/PERF2, branch PERF2. PM_ROW=PERF2 PM_LANE=PERF2.
WAIT: console capture 1 (plan_perf.md section 9). Start only when run B (`audiocore0`) shows lgms at least 2 ms lower than run A
and wk - wwait higher by less than the lgms fall. Else the orchestrator deletes this page.
Objective: balance. FACT the audio control thread (prio of the game thread, pm_audio.c:116-119) and the mixer (that prio + 1, pm_au2.c:191-195)
run on core 1 and preempt the logic thread (game prio + 2, pm_logic.c:93-95). Core 0 waits 15 to 17 ms per step for core 1 (FACT capture).
Counters: none of C1..C14. Prof: lgms falls, wk - wwait rises (console only).
Edit only: 3ds/src/pm_audio.c:119, :121, 3ds/src/pm_au2.c:194. Sources are CRLF.
## Edits (line numbers main 1472130686)
1. pm_audio.c:119 before: `if (!pm_debug_has("audiocore0")) au_thread = pm_thread_create(au_main, NULL, 64 * 1024, prio, 1, "audio");`
   after: `if (pm_debug_has("audiocore1")) au_thread = pm_thread_create(au_main, NULL, 64 * 1024, prio, 1, "audio"); /* PERF2: core 0 by default (the fallback below) */`
2. pm_au2.c:194 before: `int core = pm_debug_has("audiocore0") ? -2 : 1;` after: `int core = pm_debug_has("audiocore1") ? 1 : -2; /* PERF2 */`.
3. pm_audio.c:121 `pm_log_w("audio thread on core 1 not created, using core 0");` becomes `pm_log_w("audio: control thread on core 0 (no audiocore1, or core 1 refused)"); /* PERF2 */`.
## Tests (output to build3ds/out/, read the last 20 lines; 1 build, at most 2 Azahar starts)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/perf2_host.txt` (timeout 600000): PASS. Commit `PERF2 1: audio threads on core 0`.
2. `PM_LANE=PERF2 sh 3ds/build.sh`. Base, run and cmp as ws_PERF0 step 3 for spot_mac01_drec: every shot byte-equal.
   Pass also: the run log (loglevel debug) has `threads: audio on core 0` and `threads: audio render on core 0`.
Fix rights: the 3 edited lines. A DIFF, a hang or another core in the thread lines: stop, send the output path to oracle NATIVE15.
Stop: last commit `PERF2 2: gate` deletes this page. One SubagentHandback: commit hash, host, cmp result, the 2 thread lines verbatim.
The orchestrator pushes the merged build once with the PERF capture words (section 9, capture 2) to confirm on the console.
