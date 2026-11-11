# Row STACKANY: stack walk for a crash on any pm_thread thread

(Model: Haiku. Waits: BTLDBG merged (it edits the recs loop of crash_handler) and LIBTHR merged (pm_thread.c pm_thread_adopt).
Branch from main after both. Edits pm_thread.c and pm_main.c crash_handler only. Reads only this file and CLAUDE.md.)

## Cause (oracle code read, main 4db2dfadf)
- The stack walk runs only when sp is inside a StackRec (pm_main.c:33-36 recs[4]: game, audio, render, logic; filled by
  pm_stack_register). Every other thread (flash, savecrash, log writer, log step, log sink, c1, c2, prefetch, rs_loader, wdog, main,
  and the adopted libctru threads) prints `sp ... is outside the known thread stacks, no stack walk`.
- pm_thread_create knows each thread's stack size (its `stack` argument, pm_thread.c:47) and th_entry (:36-45) runs at the stack top:
  it can record the bounds itself, as pm_stack_register does (pm_main.c:38-45: hi = a local's address, lo = hi - size + 512).
  An adopted thread has no known size: it keeps no bounds.

## Start
`sh 3ds/tools/wt_new.sh STACKANY`. PM_ROW=STACKANY PM_LANE=STACKANY on every build and bench call.

## Exact edits
1. pm_thread.c PmTh: add `u32 ssz, slo, shi; /* STACKANY: stack size and bounds (0: unknown) */`. pm_thread_create: `s->ssz = (u32)stack;`
   before threadCreate. th_entry, first lines: `volatile u32 here; s->shi = (u32)&here + 512u; s->slo = ((u32)&here - s->ssz + 512u) & ~3u;`.
   pm_thread_adopt: `s->ssz = s->slo = s->shi = 0;`. slot_free: the same three set to 0.
   New: `int pm_thread_stack_current(u32* lo, u32* hi)`: the used slot whose th == threadGetCurrent() and shi != 0: set lo, hi, return 1;
   else 0. Declare it in 3ds/include/pm_thread.h.
2. pm_main.c crash_handler: after the recs loop add
   `StackRec tst_; if (!st) { u32 lo_, hi_; if (pm_thread_stack_current(&lo_, &hi_) && regs->sp >= lo_ && regs->sp < hi_) { tst_.lo = lo_; tst_.hi = hi_; tst_.name = "thread"; st = &tst_; } } /* STACKANY */`.
   The walk below already stops at st->hi and after 2048 words.

## Gate (CLAUDE.md 2; Azahar budget 3 of 8)
- Build, memcheck OK; `bench.py run nd_host --row STACKANY --why "thread stack bounds"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py run spot_mac01_det --row STACKANY --why "no draw change"`, `bench.py cmp`: 6/6 byte-equal.
- One crash run on Azahar is not possible (Azahar does not call the guest handler, pm_main.c:206). Console check for the user: a push
  with `crashtest=prefetch` (a pm_thread thread without a StackRec): crash.txt has `[CRASH] stack used` and `[CRASH] stack` lines.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file
in the completing commit. Hand back the hash, the gates, the memcheck line.
