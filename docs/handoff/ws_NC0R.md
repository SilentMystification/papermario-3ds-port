# ws_NC0R (Sonnet): the target role reads the draw's recorded other mode words; nd_c0.omode_l goes
Designer: oracle NATIVE16 (plan_perf.md section 7). Start after NC0B is merged (it adds NdGs and nd_gs). Branch from main. Worktree build3ds/wt/NC0R, branch NC0R.
PM_ROW=NC0R PM_LANE=NC0R. Parallel with NC0C (other lines). Lines: main 01e0780881; NC0B shifts some: grep the quoted text.
Why Sonnet: 14 readers take 3 sources (the snap, the static entry, the model snap), 2 functions change their signature across 4 files, and a census decides.
Objective: nd_img_role (nd_tgt.h:38-42) reads no nd_c0 word; NdC0 loses omode_l (nd.h:167 keeps omode_h). nd_c0 size 900 to 896 (GUESS: base after NC0B). c0gate 0 / 0 / 1.
Proxy (Azahar): drec byte-equal, 0 `rolediff` lines. Console number: none by design (no frame time change).
## Design (FACT code read)
- The only reader of nd_c0.omode_l is nd_tgt.h:39. Its writers: nd_direct.c:58, nd_recs.c:148 (second half), nd_set.c:105 (nd_sv_oml), init nd_state.c:11.
- Stream OML units never write nd_c0.omode_l (nd_rec.c:480 records nd_oml_run, nd_walk.c:258 does nothing): today the role reads the last copy, hit or direct value.
  The snap carries the exact value at each draw (nd_snap.c:55 computes r.role from the same words on core 1).
- nd_tgt.h: replace nd_img_role by `static inline int nd_img_role_m(int ff, uint32_t oml, uint32_t omh)`: same body with oml, omh for nd_c0.omode_l, nd_c0.omode_h.
  nd_snap.h after `#include "nd_apw.h"` (:10; not near NdGs, NC0C edits that line): `#include "nd_tgt.h"` and `#define ND_ROLE(ff) nd_img_role_m((ff), nd_gs ? nd_gs->oml : nd_ap->oml, nd_gs ? nd_gs->omh : nd_ap->in.omode_h)`.
## Step 1 (switch, with a census; nd_c0.omode_l is still written)
1. To ND_ROLE(0): nd_comb.c:139, :200; nd_direct.c:38; nd_geom.c:200, :224; nd_rect.c:240; nd_snap.c:197, :198; nd_target.c:78. To ND_ROLE(1): nd_rect.c:321, :322.
   Example nd_geom.c:224 before `if (nd_img_role(0) == ROLE_DEPTH) {` after `if (ND_ROLE(0) == ROLE_DEPTH) {`.
2. Model path (the record's own snap words): nd.h:416 `pm_dlc_native_ranges(..., unsigned max, uint32_t oml, uint32_t omh)`, nd.h:418 `pm_dlc_vis_mask(const void* list,
   uint64_t* out, uint32_t oml, uint32_t omh)`; nd_shape.c:714, :767 use `nd_img_role_m(0, oml, omh)`.
   Hit path, pm_gpu.c:3991 and :4125 (before hit_apply): `const uint32_t* kw = nd_msnap_key((const uint32_t*)(uintptr_t)k->rp);` pass `kw[1], kw[0]` (k_oml, k_omh).
   Miss path, nd_model_set.c:31 (after rec_apply): `const uint32_t* mw = nd_msnap_w();` pass `mw[MSW_OML], mw[MSW_OMH]` (include nd_msnapw.h).
3. A slice inside a static call keeps the static words (as NC0B did for geom): nd_set.c:105 nd_sv_oml adds `if (nd_gs) nd_gs->oml = (nd_gs->oml & ~r[0]) | r[1];`,
   :103 nd_sv_omh adds `if (nd_gs) nd_gs->omh = (nd_gs->omh & ~m) | r[1];`.
4. Census: in step 1 ND_ROLE(ff) and the 2 model calls go through `int nd_role_c(int ff, uint32_t oml, uint32_t omh, const char* f, int l)` (nd_target.c): it returns
   nd_img_role_m(ff, oml, omh) and, when `nd_tgt_role(nd_tgt.kind, nd_tgt.siz, ff, nd_c0.omode_l, nd_c0.omode_h, zcol)` differs, a cold log
   `rolediff f<n> <file>:<line> new <r> old <o>` (16 lines at most). Pass `__FILE__, __LINE__` from the macro.
## Step 2 (delete): the census (ND_ROLE calls nd_img_role_m); nd.h:167 omode_l; nd_state.c:11 its init; nd_direct.c:58; nd_recs.c:148 the omode_l half;
   nd_set.c:105 nd_sv_oml keeps only the nd_gs line (the symbol stays: nd_ds.c:525, :528 return it); nd_snap_in.c:46 comment.
## Checklist (hw_guidelines.md 5, 6): ND_ROLE is 2 loads, 2 branches and the inline role (no call on the clean path: pm_zbuf_need only for ROLE_CPUFILL).
   PICA: no new command; pm_gpu_off_begin (nd_target.c:84) runs at the same epochs.
## Tests (build3ds/out/, last 20 lines). Per step: 1 build, 2 Azahar runs (`bench.py base <b> --commit $(git merge-base HEAD main)`, `bench.py run`, `bench.py cmp`)
1. Step 1: host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS; build; c0gate 0 / 0 / 1; spot_mac01_drec (depth role, crystal ball) and hudfb_drec byte-equal;
   `grep -c rolediff` 0 in both logs. Commit `NC0R 1: target role from the recorded other mode words`.
2. Step 2: host PASS; c0gate 0 / 0 / 1; nd_c0 size (`arm-none-eabi-nm -S`); wscmb_pause_drec and entcapd_kgr01_drec (darkness stencil, ROLE_CPUFILL) byte-equal.
   entcapd_kgr01_drec may need its base first (2 runs): the task budget is 8 Azahar starts.
Fix rights: compile errors; host test calls of the 2 changed signatures. A DIFF, a rolediff line or a crash: stop, send the output path to oracle NATIVE16.
Stop: last commit `NC0R 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hashes, host, c0gate, nd_c0 size base and tip, the 4 cmp lines, rolediff count.
