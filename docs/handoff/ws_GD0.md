# ws_GD0 (Haiku): core 1 draw census: the frame push out of ui, ui split, xf records timed, whole time and units per class (GAMEDRAW)
Designer: oracle NATIVE18 (plan_gamedraw.md 1.6, 1.7). Worktree build3ds/wt/GD0, branch GD0. PM_ROW=GD0 PM_LANE=GD0. Start: main 01a1569b7d or later; grep the quoted text.
Why Haiku: timer wraps, 2 counters and print lines, all given below. No draw change. Counter: none moves; the lines pick GD2 to GD7.
FACT: in a sampled step the logic runs inline, so nuGfxTaskStart (src/main_loop.c:291, inside the ui region) walks the frame on the same thread
(3ds/src/nd_wq.c:155-156): the walk's untimed rest (cls rest 5.87 ms, live_cap1c_170.txt) lands in c1g ui (7.98).
## Edits (CRLF files: use the Edit tool)
1. 3ds/include/nd.h:452: after `NDT_GUI,` add `NDT_GSUB, NDT_GHUD, NDT_GMODE, NDT_GWIN, NDT_RXF, /* GD0 */`. After `void pm_gpu_ndt_pop(int k);` add
   `void pm_gpu_ndt_take_g(uint64_t o[13], unsigned u[13], unsigned a[13]); /* GD0: classes NDT_GENT..NDT_GWIN */`.
2. src/main_loop.c: move `NDT_E(tu_b, NDT_GUI); }` (:294) to the line before `nuGfxTaskStart(NU_GFX_UCODE_F3DEX2, NU_SC_TASK_LODABLE` (:291);
   wrap :291 as `{ NDT_B(tu_c); nuGfxTaskStart(...); NDT_E(tu_c, NDT_GSUB); }`. Wrap each call with its own `{ NDT_B(t_); <call>; NDT_E(t_, K); }`:
   render_hud_elements_backUI (:221), render_hud_elements_frontUI (:250): K = NDT_GHUD; render_game_mode_backUI (:223), render_game_mode_frontUI (:279):
   NDT_GMODE; render_window_root (:228, :274), render_messages (:244, :258, :268): NDT_GWIN. Example, before: `    render_hud_elements_backUI();`
   after: `    { NDT_B(t_hb); render_hud_elements_backUI(); NDT_E(t_hb, NDT_GHUD); } /* GD0 */`.
3. 3ds/src/nd_xf.c: add `#include "nd.h"` and `extern int pm_ph_on;`. xf_rec body: `NDT_B(tx); nd_ds_unit(&nd_ds, w0, w1); nd_rec_xf(nd_xf_run, r); NDT_E(tx, NDT_RXF);`.
4. 3ds/src/pm_gpu.c after `static unsigned ndt_sp;` (:1177):
   `#define NDT_GC (NDT_GWIN - NDT_GENT + 1)` and `static uint64_t ndt_gi[NDT_GC]; static unsigned ndt_gu[NDT_GC], ndt_ga[NDT_GC], ndt_u[4], ndt_a[4]; extern unsigned nd_rnr_n, nd_altu_n; /* GD0 */`.
   Push (:1182), in the `if (ndt_sp < 4u)` block: `ndt_u[ndt_sp] = nd_rnr_n; ndt_a[ndt_sp] = nd_altu_n;`.
   Pop, after `ndt_sum[k] += tot > in ? tot - in : 0u;` (:1190):
   `if (k >= NDT_GENT && k <= NDT_GWIN) { ndt_gi[k - NDT_GENT] += tot; ndt_gu[k - NDT_GENT] += nd_rnr_n - ndt_u[i]; ndt_ga[k - NDT_GENT] += nd_altu_n - ndt_a[i]; }`
   After pm_gpu_ndt_take (:1201): pm_gpu_ndt_take_g copies ndt_gi, ndt_gu, ndt_ga to o, u, a and zeroes them.
5. 3ds/src/pm_phase.c:491, after `{ uint64_t ct[NDT_N]; pm_gpu_ndt_take(ct); }` add `{ uint64_t g_[13]; unsigned u_[13], a_[13]; pm_gpu_ndt_take_g(g_, u_, a_); } /* GD0 */`.
6. pm_phase.c after the c1g print (:636-638), same block. gm[k] = pf_cs(gi[k], pf_s); gu[k] and ga[k] divided by pf_s (0 when pf_s is 0). Lines:
   `prof f%u w%u c1x gsub %u.%02u hud %u.%02u mode %u.%02u win %u.%02u rxf %u.%02u` (exclusive: CS(cm[NDT_GSUB]) .. CS(cm[NDT_RXF]));
   `prof f%u w%u c1G ent .. mdl .. ply .. npc .. wrk .. fx .. task .. thud .. ui .. gsub .. hud .. mode .. win ..` (CS(gm[0..12]), %u.%02u each);
   `prof f%u w%u c1u ent %u ... win %u` (gu[0..12]) and `prof f%u w%u c1v ent %u ... win %u` (ga[0..12]). Same 13 names in that order.
   The divides run once per window in a cold print (hw_guidelines.md rule 4 is for hot code).
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS. 2. `PM_LANE=GD0 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
3. `python 3ds/tools/bench.py base hudfb_drec --commit $(git merge-base HEAD main)`; `bench.py run hudfb_drec --row GD0 --why "GD0 census only"`;
   `bench.py cmp hudfb_drec --row GD0`: byte-equal.
4. `python 3ds/tools/capture_check.py --row GD0 --proof "c1x gsub,c1G ent,c1u ent,c1v ent" --profile prof warp=mac_01:0 pos=0:0:200 profdisk=1`:
   CAPTURE CHECK OK. Never add phase=on. Azahar timers read 0.00: the proof is the line.
Fix rights: compile errors (nd.h include order: before nd_st.h, PROF3 note). A missing line: read where the c1g line prints. A DIFF, a base fault:
stop, send the output path to oracle NATIVE18.
Stop: commit `GD0 1: gate. <host> <c0gate> <cmp line> <CAPTURE CHECK line>` deletes this page. Hand-back: one SubagentHandback: commit hash, 4 results.
