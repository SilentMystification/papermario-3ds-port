# ws_GRABSEQ (Haiku): the grab hold and the splash grab copy once per drawn frame, not once per frame number
Objective (FACT, oracle code read, main 89725d888c). Tidal Wave = fx_underwater (sushie.c:336). It draws the screen copy of this frame (underwater.c:222-258, grab hold :230).
With 60 FPS, a walked key frame (pm_nusys.c:194-207) and the next L frame have the same frame number (pm_main.c:499 sets pm_frame_n after the L post).
The second walk skips the copy (nd_target.c:70) but pm_gpu_begin_wait dropped the slot (pm_gpu.c:2888, :2897). The load reads the CPU memory of
the colour buffer. The combiner PM_CC_48 (macros.h:501) is texel + prim * prim alpha: prim (10,110,255) only = the flashing blue rectangle.
water_splash (sushie.c:369) has the same defect (nd_target.c:77). Counters: no C1..C12 counter moves. Model: Haiku.
Worktree build3ds/wt/GRABSEQ, branch GRABSEQ. PM_ROW=GRABSEQ PM_LANE=GRABSEQ on every bench call.
Files: 3ds/src/pm_gpu.c, 3ds/src/nd_target.c, 3ds/include/nd_grabonce.h (new), 3ds/tools/nd_grabonce_test.c (new),
3ds/tests/host.d/grabonce.sh (new). Edit no other file. The blocks are in ws_GRABSEQ_code.md.
## Edit 1 (pm_gpu.c:2880-2884). Before:
`static int fh_on(void); ...` / `void pm_gpu_begin_wait(void) {` / `    PM_WALK_OWN();` / `    if (!begin_pend) return;` / `    begin_pend = 0;`
After: add the line `unsigned pm_gpu_seq; /* GRABSEQ: ... */` above `void pm_gpu_begin_wait(void) {`.
Add the line `    pm_gpu_seq++; /* GRABSEQ: ... */` after `    begin_pend = 0;`. Exact text: block A.
## Edit 2 (nd_target.c:69-81). Before:
`        if (hold_grab_f == pm_frame_count()) return;` / `        hold_grab_f = pm_frame_count();`
After: `        if (!nd_grab_once(&hold_grab_f, pm_gpu_seq)) return;` (one line replaces two). Do the same for splash_grab_f at :77-78.
Add `#include "nd_grabonce.h"` after `#include "variables.h"` (:14). Add `extern unsigned pm_gpu_seq;` after `unsigned hold_grab_f = ~0u; ...` (:58).
## Edit 3: block B as 3ds/include/nd_grabonce.h. Block C as 3ds/tools/nd_grabonce_test.c. Block D as 3ds/tests/host.d/grabonce.sh (LF endings).
## Loop (1 build, at most 4 Azahar starts; base runs not charged)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/grabseq_host.txt 2>&1`. Pass: `ND HOST TESTS PASS` and `grabonce ok`.
   Commit `GRABSEQ 1: grab hold and splash grab key on the drawn frame (pm_gpu_seq)`.
2. One build. memcheck margin above 0 (GUESS: change under 1 KB).
3. Gate (fxr_drec draws fxtest 87 = underwater at 30 FPS): `python 3ds/tools/bench.py base fxr_drec`, then
   `python 3ds/tools/bench.py run fxr_drec --row GRABSEQ --why "GRABSEQ gate"`, then
   `python 3ds/tools/bench.py cmp fxr_drec --row GRABSEQ --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal (one differing bit fails).
   A cold first fxr_drec cmp: one warm rerun (bench.py:2023 rule). Then the same 3 steps for spot_mac01_drec.
## Tests that decide
grabonce.sh: the seed (no once test) must print `grabonce FAIL walk 0 copies 2`. The lint seed must match. The real header prints `grabonce ok`.
FACT: no Azahar bench walks a key frame with the wave (fps60 det is not stable, memory note). The user checks Tidal Wave with 60 FPS on the console.
## Fix scope
A host test failure: fix only the new files. A byte gate that differs: do not change the code. Write the cmp output to build3ds/out/ and send the path to the oracle.
## Stop and hand-back
Stop: host PASS and both gates byte-equal. Final commit `GRABSEQ 2: gate` with the values. It deletes ws_GRABSEQ.md and ws_GRABSEQ_code.md.
Hand-back: build3ds/out/handback_GRABSEQ.txt (line 1 commit hash, then the 2 gate values, memcheck margin, counters: none).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
