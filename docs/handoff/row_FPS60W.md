# Row FPS60W: the 60 fps key frame copy takes only patches that land on command starts (fix of the copy path)

(Model: Haiku. One function's validation pass and one checked return; exact edits. Try 3 (ORACLE7): the timeout and reset edits of
tries 1-2 (branch FPS60W 37ed263e4) are not this row any more: start a new branch from main; the orchestrator closes the old one.)

## User fact (2026-11-12)
With 60 fps turned on, the O3DS .170 froze at the toggle or some frames after it and needed a power cycle.

## Code read (ORACLE7, main at writing)
What 60 fps adds on the GPU side, read to values:
- Frame pacing (pm_gpu.c:2736-2747 sync60_pace): two frames per step, one per 59.83 Hz period; each C3D_FrameBegin (:2789 L frame,
  :2941 key frame) first waits for the whole GX queue of the frame before (citro3d waits with no timeout), so at most one frame is
  in flight; the vertex ring is reset only after that wait (:2790). No buffer is written while the GPU reads it: kc_build writes
  kc_buf at the L frame end (:3017), the key frame of the same step queues it after the L queue finished, and the next kc_build
  comes after the next frame begin waited for the key frame. The GX queue: pm_gx_room drains at 24 of 32 entries (:2455-2464), and
  pm_gpu_key_show calls it before every add (:2953). Memory: tables of about 290 KB and 393 KB on the heap, kc_buf 192 KB linear,
  each with a failure path (pm_interp.c:94-97, pm_gpu.c:183-191, :2909-2910). Core 2 is not used. The logic/walker hand-off waits on
  events (nd_wq.c:94-107, pm_logic.c:172-177): every step end with a key posts its key message (pm_main.c:496-502), so the key
  wait is always signalled.
- The one value the copy trusts without a check: kc_build applies each K patch at `off = p->base - cb` and reads the parameter count
  from `cb[off + 1] >> 20` (pm_gpu.c:2919-2926). It checks only that base lies inside the copied range. If a recorded base is not the
  start of a command (an odd word offset, or a parameter word inside a consecutive write), word off + 1 is not a header: the count
  is any value up to 255, and the patch writes K words over words of another command, including a header (w = off for j = 0, off + 1
  + j for j >= 1, :2924-2925). A header word overwritten with a uniform value is an invalid PICA command: the real GPU stops on it
  (a hang with a frozen picture), Azahar's command processor reads on. The patch bases are recorded at the emit sites (nd_emit.c,
  NdKPatch, INTERPB2) and dropped for arena and scratch words (pm_gpu.c:1167-1178); nothing proves every kept base is a command
  start. This is the only path found whose bad value hangs the console and not Azahar.
- C3D_FrameBegin's result at :2941 is not checked: on false (a frame already begun) the key frame queues entries into a frame
  citro3d did not open.

## Start
`sh 3ds/tools/wt_new.sh FPS60W2` (new branch from main). PM_ROW=FPS60W2 PM_LANE=FPS60W2.

## Touch only
3ds/src/pm_gpu.c: kc_build (:2894-2933) and the C3D_FrameBegin line of pm_gpu_key_show (:2941).

## Exact edits
1. kc_build, after the list check loop and before `memcpy(kc_buf, cb, coff * 4u)`: decode the command words [0, coff) once from the
   start: at word i, `h = cb[i + 1]`, `n = (h >> 20) & 0xFF` extra words, next command at `i + 2 + n + (n & 1)` (8-byte padding);
   mark i in a static bitmap `kc_cmd[KC_WORDS / 64]` (u32 words, 3 KB) and store n in no table (recompute below). A decode that
   steps past coff: `pm_kc_why[2]++; return;` (the copy is refused, the key frame is walked).
2. In the patch loop: `off` must be marked in kc_cmd (k is then read from a real header; the `j <= k` rule stays as today); a base
   whose off is not marked: `pm_kc_why[2]++; kc_out("patch", ...);
   return;` (refuse the whole copy, as for a list outside the range).
3. pm_gpu_key_show :2941: `if (!C3D_FrameBegin(0)) { pm_kc_why[4]++; return; }` (nothing queued; pm_key_show still calls
   pm_lg_key_walked, so the step pairing holds).
No timeout and no reset in this row.

## Gate (exact; no Azahar-only proof of the hardware rule)
- Build, memcheck OK; nd_host PASS.
- Host self-test in nd_host (new, 3ds/tools/kccmd_test.c, links only the decode function moved to a static inline in a header
  3ds/include/nd_kccmd.h): a hand-made command list (3 commands, one consecutive write of 4 params) gives the expected start set;
  a patch base at a parameter word is refused; the decode of a list with a count past the end is refused. Prints `kccmd ok 3/3`.
- One run of spot_mac01_fps60_det (copy runs as before): the interp census line keeps its pm_kc_why[2] value at 0 on Azahar
  (hand back pm_kc_why and the copies shown pm_kc_n[0]); a value above 0 names frames whose patch bases were not command starts:
  that is the fault proven by value; hand it back. Budget 2 Azahar starts.

## Try 4 (ORACLE7, after 0a371ba29 / 8ed5fa0fa: row spec fault, budget reset once)
- Why the interp census never printed (code read): pm_main.c:710 calls interp_heavy() only inside `!pm_det_on() && (...)`, so under
  det (spot_mac01_fps60_det has det) interp_heavy, which holds the 300-step print (:516-520), never runs. Also the print is
  pm_log_hw, a debug-level line, and the scene's prof=4 sets log=off, so it would be dropped anyway.
- (a) The copy path runs in this scene: under det LGF_ISKIP is never set (:710), so every drawn step takes its keys
  (pm_main.c:496), wq_step_end sets kc_want (:473), and kc_build runs at each L frame end (pm_gpu.c:3017); pm_kc_n[0] counts the
  copies shown. In det the counters are never reset (only interp_heavy resets them), so they are counts since boot.
- (b) The stale-build refusal after the commit is a normal coder step (rebuild, then run), not a row fault.
- Edit (pm_main.c only, next to interp_heavy): `static void kc_census(void)`, called once per drawn step when pm_interp_on > 0, at
  the line before :710 (outside the det condition). It counts steps; at step 300 it prints once with pm_log_prof_line (the line that
  prints in prof=4 mode): `kcen f<frame> steps 300 kcopy <pm_kc_n[0]> kwalk <pm_kc_n[1]> why <pm_kc_why[0..4]>`. It resets nothing.
- Scenario in the row's own file 3ds/tools/azt.d/FPS60W2.json and bench in bench.d/FPS60W2.json: `kccen_mac01` = spot_mac01_fps60_det's
  debug without `shots shotwin=200:6`, until `kcen f\d+ `, cap 45, deadline 40, needs_picture false, expected_events only the
  warp line.
- Gate: the kcen entry shows kcopy above 0 (the copy path ran) and why[2] = 0 (no patch base off a command start), exact. Hand back
  the whole entry. Budget 2 Azahar starts.

## Stop
A failed self-test or pm_kc_why[2] above 0: stop, commit, hand back the values (the oracle then fixes the emit site that records
the base). Delete this row file in the completing commit.
