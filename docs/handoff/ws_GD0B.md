# ws_GD0B (Haiku): census 2: the frame push out of ui, ui split (hud, mode, win), xf records timed, alt units per class (GAMEDRAW)
Designer: oracle NATIVE18 (plan_gamedraw.md 1.3, 1.4). Worktree build3ds/wt/GD0B, branch GD0B. PM_ROW=GD0B PM_LANE=GD0B. Start: main b686c3ec5d or later.
Why Haiku: timer wraps, one counter and print lines, all given below. No draw change. Counter: none moves; the lines size GD7, GD4, GD6.
FACT: in a sampled step the logic runs inline, so nuGfxTaskStart (src/main_loop.c:291, inside the ui region) walks the frame on the same thread
(3ds/src/nd_wq.c:155-156). The walk's untimed rest (cls rest 5.87 ms, live_cap1c_170.txt) lands in c1g ui (7.98). GD0 (merged ff6e9cc1ea) is the base.
## Edits (CRLF files: use the Edit tool; grep the quoted text, lines move)
1. 3ds/include/nd.h:452: after `NDT_GUI,` add `NDT_GSUB, NDT_GHUD, NDT_GMODE, NDT_GWIN, NDT_RXF, /* GD0B */`. nd.h:459 prototype becomes
   `void pm_gpu_ndt_take_g(uint64_t o[13], unsigned u[13], unsigned a[13]);`.
2. src/main_loop.c: move `NDT_E(tu_b, NDT_GUI); }` (:294) to the line before `nuGfxTaskStart(NU_GFX_UCODE_F3DEX2, NU_SC_TASK_LODABLE` (:291);
   wrap :291 as `{ NDT_B(tu_c); nuGfxTaskStart(...); NDT_E(tu_c, NDT_GSUB); }`. Wrap each call below in its own block, K per call:
   render_hud_elements_backUI (:221), render_hud_elements_frontUI (:250): NDT_GHUD; render_game_mode_backUI (:223), render_game_mode_frontUI (:279):
   NDT_GMODE; render_window_root (:228, :274), render_messages (:244, :258, :268): NDT_GWIN.
   Before: `    render_hud_elements_backUI();`  After: `    { NDT_B(t_hb); render_hud_elements_backUI(); NDT_E(t_hb, NDT_GHUD); } /* GD0B */`
3. 3ds/src/nd_xf.c: add `#include "nd.h"` and `extern int pm_ph_on;`. xf_rec body: `NDT_B(tx); nd_ds_unit(&nd_ds, w0, w1); nd_rec_xf(nd_xf_run, r); NDT_E(tx, NDT_RXF);`.
4. 3ds/src/pm_gpu.c:1229. Before: `static uint64_t ndt_gi[NDT_GUI - NDT_GENT + 1]; static unsigned ndt_gu[NDT_GUI - NDT_GENT + 1], ndt_u[4]; extern unsigned nd_rnr_n; /* GD0 */`
   After: `#define NDT_GC (NDT_GWIN - NDT_GENT + 1)` and `static uint64_t ndt_gi[NDT_GC]; static unsigned ndt_gu[NDT_GC], ndt_ga[NDT_GC], ndt_u[4], ndt_a[4]; extern unsigned nd_rnr_n, nd_altu_n; /* GD0, GD0B */`.
   Push (:1234): add `ndt_a[ndt_sp] = nd_altu_n;` after `ndt_u[ndt_sp] = nd_rnr_n;`.
   Pop (:1243): `k <= NDT_GUI` becomes `k <= NDT_GWIN`; add `ndt_ga[k - NDT_GENT] += nd_altu_n - ndt_a[i];` in the same block.
   take_g (:1255-1257): 13 classes (NDT_GC), also copies ndt_ga to a and zeroes it.
5. 3ds/src/pm_phase.c:492: `{ uint64_t g_[13]; unsigned u_[13], a_[13]; pm_gpu_ndt_take_g(g_, u_, a_); } /* GD0 */`.
6. pm_phase.c:640-645: arrays of 13 (gi, gu, ga, gm); ga[k] divided by pf_s as gu[k]. The c1G line gets 4 more names after ui: `gsub`, `hud`, `mode`, `win`
   (CS(gm[9]) .. CS(gm[12])); the c1u line the same 4 (gu[9] .. gu[12]). New lines after them:
   `prof f%u w%u c1v ent %u mdl %u ply %u npc %u wrk %u fx %u task %u thud %u ui %u gsub %u hud %u mode %u win %u` (ga[0..12]) and
   `prof f%u w%u c1x gsub %u.%02u hud %u.%02u mode %u.%02u win %u.%02u rxf %u.%02u` (CS(cm[NDT_GSUB]) .. CS(cm[NDT_RXF]), exclusive).
   The divides run once per window in a cold print (hw_guidelines.md rule 4 is for hot code).
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS. 2. `PM_LANE=GD0B sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
3. `python 3ds/tools/bench.py base hudfb_drec --commit $(git merge-base HEAD main)`; `bench.py run hudfb_drec --row GD0B --why "GD0B census only"`;
   `bench.py cmp hudfb_drec --row GD0B`: byte-equal.
4. `python 3ds/tools/capture_check.py --row GD0B --proof "c1x gsub,c1v ent,c1G ent" --profile prof warp=mac_01:0 pos=0:0:200 profdisk=1`:
   CAPTURE CHECK OK. Never add phase=on. Azahar timers read 0.00: the proof is the line.
Fix rights: compile errors (nd.h include order: before nd_st.h, PROF3 note). A missing line: read where the c1g line prints. A DIFF, a base fault:
stop, send the output path to oracle NATIVE18.
Stop: commit `GD0B 1: gate. <host> <c0gate> <cmp line> <CAPTURE CHECK line>` deletes this page. Hand-back: one SubagentHandback: commit hash, 4 results.
