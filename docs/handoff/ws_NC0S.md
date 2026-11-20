# ws_NC0S (Haiku): the scissor and prim_z leave nd_c0
Designer: oracle NATIVE17 (plan_perf.md section 7). Start now from main. Parallel with T1FIX (other files). NC0V waits for this merge.
Worktree build3ds/wt/NC0S, branch NC0S. PM_ROW=NC0S PM_LANE=NC0S. Lines: main d0483da1ba. Grep the quoted text if a line moved.
Why Haiku: a closed edit list. The result is byte-equal by construction (proof below). No census.
Objective: delete nd_c0.sci_x0, sci_y0, sci_x1, sci_y1 and nd_c0.prim_z. Counter: nd_c0 896 to 876 B (GUESS: 5 words, no padding). c0gate 0 / 0 / 1.
## Proof (FACT code read)
- Scissor writers: only nd_sci_run (nd_set.c:84) and nd_sv_sci (nd_set.c:108). Both then call pm_gpu_set_scissor with the same 4 values.
  pm_gpu_set_scissor has no other caller (FACT grep 3ds/src, 3ds/tools). So a raw copy at the top of pm_gpu_set_scissor is equal to nd_c0.sci_* at
  every time. Start values: nd_state.c:11 `.sci_x1 = 320, .sci_y1 = 240` = the new copy `{ 0, 0, 320, 240 }`.
- gsci_* (pm_gpu.c:65) are not equal: the setter clamps them and pm_gpu.c:2776, :2885 reset them. Do not read gsci_* for these readers.
- Scissor readers: nd_rect.c:63-66 (rect_to_image_draw), nd_rect.c:166 (nd_texrect_depth), nd_target.c:84 (nd_img_check band).
- prim_z has no reader (FACT grep `nd_c0.prim_z`): writes at nd_msnap.c:33, nd_set.c:107 (nd_sv_primz), pm_gpu.c:4571.
## Edits
1. pm_gpu.c:65 after the gsci line: `static int gsci_r[4] = { 0, 0, 320, 240 }; /* NC0S: the raw words of the last scissor unit (before the clamp) */`
   pm_gpu.c:4672 first line of pm_gpu_set_scissor: `gsci_r[0] = x0; gsci_r[1] = y0; gsci_r[2] = x1; gsci_r[3] = y1; /* NC0S */`
   After the function: `const int* pm_gpu_sci_raw(void) { return gsci_r; } /* NC0S */`. pm_gpu.h:103 next line: its prototype.
2. nd_set.c:84 before: `... gv_epoch++; nd_c0.sci_x0 = (int16_t)r[0]; ... pm_gpu_set_scissor(nd_c0.sci_x0, nd_c0.sci_y0, nd_c0.sci_x1, nd_c0.sci_y1); }`
   after: `... gv_epoch++; pm_gpu_set_scissor((int16_t)r[0], (int16_t)(r[0] >> 16), (int16_t)r[1], (int16_t)(r[1] >> 16)); } /* NC0S */`
   nd_set.c:108 nd_sv_sci: the same body without the nd_c0 stores. Keep nd_sv_tex on that line unchanged.
3. nd_rect.c:63-66: first `const int* const sc = pm_gpu_sci_raw(); /* NC0S */`, then `nd_c0.sci_x0` to `sc[0]`, sci_y0 `sc[1]`, sci_x1 `sc[2]`, sci_y1 `sc[3]`.
   nd_rect.c:166: `nd_c0.sci_x0` to `pm_gpu_sci_raw()[0]`. nd_target.c:84: the same map with `const int* const sc = pm_gpu_sci_raw();` before `int band`.
4. prim_z: nd_msnap.c:33 delete ` nd_c0.prim_z = s->d_primz;` (keep the comment tags). nd_set.c:107 nd_sv_primz body:
   `{ extern void pm_gpu_dbg_primz(unsigned z); pm_gpu_dbg_primz(r[1]); }`. pm_gpu.c:4571 delete ` nd_c0.prim_z = nd_snap_primz();` and the
   `extern unsigned nd_snap_primz(void);` there. nd_snap.c:255 nd_snap_primz then has no caller: delete it.
5. nd.h: delete line 169 `unsigned prim_z;` (NdC0, not NdWs line 148) and line 183 `int sci_x0, ...`. nd_state.c:11 delete `, .sci_x1 = 320, .sci_y1 = 240`.
## Checklist (hw_guidelines.md 2, 6): 16 B of static data in pm_gpu.c, one 4-word store per scissor unit. PICA: the same scissor calls and values.
## Tests (build3ds/out/, last 20 lines). Gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=NC0S sh 3ds/build.sh`; c0gate 0 / 0 / 1; note `sizeof(NdC0)` (map or a static assert).
2. `bench.py run hudfb_drec,wscmb_pause_drec --row NC0S --why "NC0S gate"`; `bench.py cmp <b> --row NC0S` byte-equal for both.
Fix rights: compile errors, a missed `nd_c0.sci_` or `nd_c0.prim_z` text (grep 3ds/src 3ds/tools 3ds/tests first). A DIFF or a crash: stop, send the path to oracle NATIVE17.
Stop: last commit `NC0S 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hash, host, c0gate, nd_c0 size base and tip, the 2 cmp lines.
