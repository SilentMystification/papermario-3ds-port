# ws_NC0M (Haiku): the GPU call memos leave nd_c0 (omode_h, prim, prim_lod, envc, comb_w0, comb_w1, conv_k4)
Designer: oracle NATIVE17 (plan_perf.md section 7). WAIT NC0V: branch from main after the NC0V merge (shared lines nd_snap.c:189, nd_recs.c:151-154, nd.h).
Parallel with NC0T1 (other lines). NC0F waits for this merge. Worktree build3ds/wt/NC0M, branch NC0M. PM_ROW=NC0M PM_LANE=NC0M.
Lines: main 8bcf0252ee; NC0V moves some lines: grep the quoted text.
Why Haiku: a pure move with one sed line. Every read and every write of the 7 fields moves to one new object with the same start values,
so the result is byte-equal by construction. No census.
Objective: delete the 7 fields from NdC0. Counter: nd_c0 -28 B (GUESS, 7 words). c0gate 0 / 0 / 1.
## Facts (FACT code read, main 8bcf0252ee)
- The 7 fields are memos: the last value the walker sent to pm_gpu_flush, pm_gpu_set_prim, pm_gpu_set_env, pm_comb_set_k4, pm_tex_set_filter.
  Readers and writers: nd_snap.c:181-182 (ls_gpu), :189 (nd_snapchk_tri), :217 (nd_lsr_end); nd_set.c:103-104 (nd_sv_omh), :106 (nd_sv_prim, nd_sv_env),
  :110 (nd_sv_conv), :111 (nd_sv_comb); nd_direct.c:57-66; nd_recs.c:147 (hd_comb), :151-152 (hd_rare), :162 (hit_apply); nd_walk.c:263, :266 (prim_lod).
- Host: 3ds/tools/nd_direct_test.c includes nd_direct.c and defines `NdC0 nd_c0;` (:42). It reads the fields at :28-29, :62, :158-161, :178-183, :212.
- Start values: nd_state.c:11 `{ .omode_h = 0, .prim = 0xffffffffu, .envc = 0xffffffffu }`; the others 0.
## Edits
1. nd.h after `extern NdC0 nd_c0;` add:
   `typedef struct NdGm { unsigned omh, prim, env, plod, cw0, cw1, k4; } NdGm; /* NC0M: the last values the walker sent to the GPU setters (class C memo) */`
   `extern NdGm nd_gm;`
2. nd_state.c:11 before: `ND_HOTD NdC0 nd_c0 = { .omode_h = 0, .prim = 0xffffffffu, .envc = 0xffffffffu };`
   after: `ND_HOTD NdC0 nd_c0;` and next line `ND_HOTD NdGm nd_gm = { .omh = 0, .prim = 0xffffffffu, .env = 0xffffffffu }; /* NC0M */`
3. One sed (CRLF safe), run from the repo root:
   `sed -b -i -e 's/nd_c0\.omode_h/nd_gm.omh/g' -e 's/nd_c0\.prim_lod/nd_gm.plod/g' -e 's/nd_c0\.prim\b/nd_gm.prim/g' -e 's/nd_c0\.envc/nd_gm.env/g' -e 's/nd_c0\.comb_w0/nd_gm.cw0/g' -e 's/nd_c0\.comb_w1/nd_gm.cw1/g' -e 's/nd_c0\.conv_k4/nd_gm.k4/g' 3ds/src/nd_snap.c 3ds/src/nd_set.c 3ds/src/nd_direct.c 3ds/src/nd_recs.c 3ds/src/nd_walk.c 3ds/tools/nd_direct_test.c`
   Example, nd_set.c:111 before `GP_SET(0, nd_c0.comb_w0 == r[0] && nd_c0.comb_w1 == r[1]);` after `GP_SET(0, nd_gm.cw0 == r[0] && nd_gm.cw1 == r[1]);`.
4. nd_direct_test.c:42 next line: `NdGm nd_gm; /* NC0M */`.
5. nd.h NdC0: delete `unsigned omode_h;`, `unsigned prim;`, `unsigned envc;`, `uint32_t prim_lod;`, `unsigned comb_w0, comb_w1;`, `unsigned conv_k4;`.
   Keep NdWs (the same names at nd.h:144-160) unchanged.
6. Check: `grep -rnE "nd_c0\.(omode_h|prim|prim_lod|envc|comb_w0|comb_w1|conv_k4)\b" 3ds/src 3ds/tools 3ds/tests` prints only nd_decide.c comment lines.
## Checklist (hw_guidelines.md 2): nd_gm is 28 B in .data.pmhot (ND_HOTD), one or two cache lines. PICA: the same setter calls in the same order.
## Tests (build3ds/out/, last 20 lines). Gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=NC0M sh 3ds/build.sh`; c0gate 0 / 0 / 1; note sizeof(NdC0) base and tip.
2. `bench.py run wscmb_pause_drec,fxparts_drec --row NC0M --why "NC0M gate"`; `bench.py cmp <b> --row NC0M` byte-equal for both.
Fix rights: compile errors (an include of nd.h is in every file above), a missed site that step 6 shows. A DIFF or a crash: stop, send the path to oracle NATIVE17.
Stop: last commit `NC0M 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hash, host, c0gate, nd_c0 size base and tip, the 2 cmp lines.
