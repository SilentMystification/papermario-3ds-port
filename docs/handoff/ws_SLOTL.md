# ws_SLOTL (Haiku): the logic thread waits only for the L frame walk, not for the key show (slot and depth 1)
Designer: oracle NATIVE19 (plan_walk.md 5). Worktree build3ds/wt/SLOTL, branch SLOTL. PM_ROW=SLOTL PM_LANE=SLOTL. Start: main 0eb73a88e7 or later; grep the quoted text.
Why Haiku: 2 counters, 1 condition function, 4 one-line changes, 1 race census, given below. Indentation: as in the file.
FACT: wq_fr_walked counts L frames (3ds/src/nd_wq.c:149) and key frames (:167). The slot (:165) and the depth 1 wait (:152) wait for both, so core 1 waits for
the key show, and the key show waits for the GPU tail of the L frame (pm_gpu.c key show, c0k kgx). The data they protect: rtab and repl (pm_interp.c:127-131),
read only while pm_interp_repl is 1 (nd_mtx.c:48, nd_mtx_set.c:115), set only for the main task of an L frame (pm_nusys.c:133-135); the display context of
the frame. A key frame is a copy from kc_buf (kc_build in the step end, before the key entry) and reads neither. The walker runs the key frame before the
next draw pass hands a step (pm_main.c:645-651), so the context chain of step N is free when step N+2 builds into it.
Objective: core 1 leaves the slot at the end of the L walk. Counter: console c0i idle (FACT 1.70 ms at idle, capture 3) falls; ft GUESS -1 to -1.7 when not pace bound.
## Edits (3ds/src/nd_wq.c, 3ds/src/pm_nusys.c, 3ds/src/pm_phase.c; CRLF: use the Edit tool)
1. nd_wq.c:36: after `static volatile unsigned wq_fr_push, wq_fr_walked; /* frames pushed and walked */` add
   `static volatile unsigned wq_l_push, wq_l_walked; /* SLOTL: L and plain frames only: the walker reads rtab and the display context only in these */`
2. :111: after `static int c_walked(void* c) { (void)c; return wq_fr_walked == wq_fr_push; }` add
   `static int c_lwalked(void* c) { (void)c; return wq_l_walked == wq_l_push; } /* SLOTL */`. Add `static int c_lwalked(void* c);` after :92.
3. :105: `        if (cond == c_walked) wq_bw[1] += d;` -> `        if (cond == c_walked || cond == c_lwalked) wq_bw[1] += d; /* SLOTL: fw keeps its meaning */`
4. :82: in the WQB_WALKED line replace `wq_fr_walked != wq_fr_push` with `wq_l_walked != wq_l_push`.
5. :149: `... __sync_synchronize(); wq_fr_walked++; }` -> `... __sync_synchronize(); wq_fr_walked++; wq_l_walked++; }`
6. :152: `        if (wq_fr_walked != wq_fr_push) wq_block(c_walked, NULL, WQB_ALL); /* depth 1: the frame before is walked first */` ->
   `        if (wq_l_walked != wq_l_push) wq_block(c_lwalked, NULL, WQB_WALKED); /* depth 1, SLOTL: the frame before is walked; its key frame is a copy */`
7. :153 and :157: `    wq_fr_push++;` (inside pm_wq_frame only, not :169 in pm_wq_key) -> `    wq_fr_push++; wq_l_push++;`
8. :165: replace `wq_fr_walked != wq_fr_push) wq_block(c_walked,` with `wq_l_walked != wq_l_push) wq_block(c_lwalked,`.
   If c_walked has no user left, delete it and its declaration (:92, :111).
9. Race census. pm_nusys.c:169: `pm_interp_lerp(0.5f);` -> `pm_lerp_busy = 1; __sync_synchronize(); pm_interp_lerp(0.5f); __sync_synchronize(); pm_lerp_busy = 0;`
   and above `void nuGfxTaskStart(` add `volatile int pm_lerp_busy; unsigned pm_slot_race; /* SLOTL: a lerp that overlaps an L walk */`.
   pm_nusys.c:133-135: after `        pm_interp_repl = (f->flags & PMF_REPL) != 0;` add `        if (pm_lerp_busy) pm_slot_race++;` and before `        pm_interp_repl = 0;` add the same line.
   pm_phase.c:730: after the `core2 drain` print statement add
   `            { extern unsigned pm_slot_race; pm_log_prof_line("prof f%u w%u slotl race %u", pf_total, pf_w, pm_slot_race); } /* SLOTL: since boot, must stay 0 */`
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=SLOTL sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`; run and cmp with `--row SLOTL`: byte-equal.
   Then the same 3 commands for spot_mac01_fps60_stereo_det: byte-equal.
3. `python 3ds/tools/capture_check.py --row SLOTL --proof "slotl race 0" --profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`: CAPTURE CHECK OK.
Race note: det runs are serial, so the benches show no race. On the console a race shows as `slotl race` above 0 (a lerp while an L walk reads rtab:
a jump of a moving object in the L picture). The console capture must print 0.
Fix rights: compile errors, an unused function. A DIFF, a race above 0 or a base fault: stop, send the output path to oracle NATIVE19.
Stop: commit `SLOTL 1: gate. <host> <c0gate> <2 cmp lines> <CAPTURE CHECK line>` deletes this page. Fallback: revert (wait for the key show, as now).
