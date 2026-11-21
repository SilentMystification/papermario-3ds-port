# ws_PIPE1 (Haiku): the L frame command lists start PIPE_H words into the citro3d command buffer; the K tail list keeps word 0
Designer: oracle NATIVE19 (plan_pipe.md 2, 4 row 2). Worktree build3ds/wt/PIPE1, branch PIPE1. PM_ROW=PIPE1 PM_LANE=PIPE1. Start: main 4a89ac5db7 or later.
Why Haiku: one block, given below. A move of list addresses only: no word of a list changes.
FACT: C3D_FrameBegin sets the buffer to (ctx->cmdBuf, size, 0) (citro3d renderqueue.c:175). The key show writes the K tail list at word 0 (3ds/src/pm_gpu.c:3181).
kc_build copies [kc_base, end) (:3121-3134) and maps list addresses by kc_cb (set to kc_base at :3147, used by the key show at :3173). kc_base is read at :3017.
With the L lists at word PIPE_H, a later early walk (PIPE3) does not write over the K tail. Size: 1.5 MB buffer (:1855), worst fill 369 KB (FACT comment there),
PIPE_H = 32768 words = 128 KB. Memory: 0 new. Counter: none (needed by PIPE3).
## Edit (pm_gpu.c; CRLF: use the Edit tool)
1. Above `static u64 s60_next;` (:2938) add:
   `#define PIPE_H 32768u /* PIPE1: the L frame lists start here (words); the K tail list of the key show stays at word 0 */`
   `static unsigned pipe_h_skip; /* PIPE1: frame begins that found words in the buffer (no offset then) */`
2. :3017. Before: `    { u32 *b_, sz_, of_; GPUCMD_GetBuffer(&b_, &sz_, &of_); kc_base = b_; }`
   After:
   `    { u32 *b_, sz_, of_; GPUCMD_GetBuffer(&b_, &sz_, &of_);`
   `      if (!of_ && sz_ > 2u * PIPE_H) { GPUCMD_SetBuffer(b_ + PIPE_H, sz_ - PIPE_H, 0); b_ += PIPE_H; } /* PIPE1 */`
   `      else if (!pipe_h_skip++) pm_log_w("pipe1: %u words at the frame begin, no offset (f%u)", (unsigned)of_, pm_frame_count());`
   `      kc_base = b_; }`
3. Do not change the key show (:3156-3185), kc_build or C3D_FrameBegin calls elsewhere (the menu present :1985 keeps word 0).
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=PIPE1 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`; run and cmp with `--row PIPE1`: byte-equal (K copy path).
   Then the same 3 commands for spot_mac01_drec: byte-equal (drec and gst; gst leaves the command buffer addresses out, pm_gpu.c:612).
3. grep the two run logs for `pipe1:`: no line (a line means a word before :3017: stop and send the log path).
Fix rights: compile errors. A DIFF, a `pipe1:` line or a base fault: stop, send the output path to oracle NATIVE19.
Stop: commit `PIPE1 1: gate. <host> <c0gate> <2 cmp lines> pipe1 0` deletes this page. Hand-back: one SubagentHandback: commit hash, the results.
Fallback: revert (the lists start at word 0, as now).
