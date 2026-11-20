# ws_DO3 (Sonnet): native submit cost per draw: split, then cut the citro3d context update per draw (DRAWOBJ row 3)
Designer: oracle NATIVE17 (plan_drawobj.md sections 4 and 6). User rule: plan_drawobj.md is read before DRAWOBJ coders. DO0 is merged (gst.txt gate).
Parallel with DO1 (other files: DO3 edits pm_gpu.c:1260-1340 and nd_emit.c only). Worktree build3ds/wt/DO3, branch DO3. PM_ROW=DO3 PM_LANE=DO3.
Why Sonnet: step 1 data picks the step 2 branch; step 2 changes the emitted words, with GPU state equality (gst.txt) as the gate.
Objective: core 0 cls sub 5.97 ms per step for 113 draws (FACT build 7680: 53 us per draw) to about 2 ms (GUESS -3 to -4 ms core 0).
## Facts (FACT code read and capture build 7680)
- nd_native_submit (pm_gpu.c:1288-1337): prog_use, BufInfo, scissor, user clip, shadow resets, then C3Di_UpdateContext when c3d_upd_needed (:1263-1280),
  then nd_emit_draw in place (:1312-1314), ndunit_after, dc_kick.
- `cnt upd 113.0` per frame = draws 113: C3Di_UpdateContext runs at every native draw (FACT prof cnt line, live_prof_170.txt #1390).
- gst.txt (DO0) hashes the GPU register file, float uniforms and LUTs at each draw, addresses left out. Note of the DO0 coder: gst_scan skips the words
  of an old buffer after its last native draw and before a split; the uniform config 0x2C0 goes into gst_r, the data 0x2C1 into gst_f only.
## Step 1 (commit `DO3 1`, no behaviour change): split and reasons
1. Sampled timers (NDT_B/NDT_E, nd.h:457-459; new classes NDT_SUPD, NDT_SEMIT at the end of the enum) around C3Di_UpdateContext and nd_emit_draw.
2. Counters per reason of c3d_upd_needed (one per `return 1` line, :1268-1280): the dirty flag word, the pointer at +268, +0x208, fog mode 7,
   float uniform dirty, int and bool uniform dirty.
3. A prof line `prof f%u w%u c0s upd %u.%02u emit %u.%02u r ...` (the reason counts per frame) after the c0r line (pm_phase.c).
4. Gate: spot_mac01_drec, entcape_kmr03_drec byte-equal; capture_check with the plan_60fps.md words and `--proof "c0s upd"`. Send the Azahar c0s line
   (reasons are valid on Azahar; the ms come from the console) to oracle NATIVE17, then wait for the step 2 branch.
## Step 2 (branch by the step 1 data; the oracle names it)
- A (float uniform dirty is the reason): emit the dirty vertex uniform rows inside the native words (nd_emit.c, as the uvso block) and clear their
  C3D_FVUnifDirty bits before c3d_upd_needed. Then C3Di_UpdateContext runs only for other reasons.
- B (another flag): clear the cause where it is set (the reason counter names the flag), or write that state in the native words.
- Gate of step 2: gst.txt equal to the base (bench.py cmp compares it), drec byte-equal: spot_mac01_drec, hudfb_drec; then fxparts_drec, wscmb_pause_drec.
## Tests (build3ds/out/, last 20 lines). Each gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. Build; c0gate 0 / 0 / 0. `python 3ds/tools/bench.py selftest-gst` PASS.
2. The cmp lines name drec.txt and gst.txt; both equal.
Fix rights: compile errors, the citro3d names of registers. A gst DIFF, a drec DIFF, a crash: stop, send the path to oracle NATIVE17.
Stop: `DO3 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hashes, host, the cmp lines, the c0s line before and after.
