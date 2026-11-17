# ws_NTGT (Sonnet, user approved 2026-11-16): native target records and a pure role table replace img_role (C11; stage 2 precondition)
Designer: the oracle (SendMessage). Worktree build3ds/wt/NTGT, branch NTGT. PM_ROW=NTGT PM_LANE=NTGT.
Start: group 1 now (3ds/ files only). Before group 2, run `git merge main` once after Merge GPOSREST (GPOSREST edits model.c:3962 and underwater.c).
Why not Haiku: one record form replaces a walker decision with 12 callers and 32 game sites, and the runner must copy the walker's op prologue exactly.
## Objective (line numbers: main 7eb679a916)
- img_role (3ds/src/nd_target.c:32-43) reads nd_ws.cimg, nd_ws.cimg_siz, cimg_zero, nd_ws.omode_l, nd_ws.omode_h and zcol per draw. After NTGT:
  the target state is `NdTgt nd_tgt` (not NdWs). Its address class is decided once, at the target change. The role is a pure function.
- FACT: nuGfxZBuffer is allocated once and never freed (3ds/src/pm_main.c:281, :298; no free in 3ds/src or src). DepthCopyBuffer and the frame buffers are static.
  So each address test of img_role gives the same value at the target change and at the draw.
- Counters: C11 func -1 (img_role leaves the ELF). C1, C2, C5 do not change. C8 reader: write the real value.
## Design rules
1. New 3ds/include/nd_tgt.h: `typedef struct NdTgt { void* img; uint16_t stride; uint8_t siz, kind; } NdTgt;` kind = bits, one per address test of img_role
   and nd_rect.c:234: NULL image, address word 0 (cimg_zero), DepthCopyBuffer, pm_is_cfb, inside nuGfxZBuffer + 320*240, pm_fb_native. Keep each test expression as today.
2. `int nd_tgt_role(unsigned kind, unsigned siz, int for_fill, uint32_t oml, uint32_t omh, int zcol)`: pure C, no global, no call. The same branches in the same order as img_role.
   pm_zbuf_need stays a side effect of the caller at ROLE_CPUFILL (nd_target.c:36).
3. set_cimg (nd_target.c:102-109) writes nd_tgt and computes kind. Remove cimg, cimg_siz from NdWs (3ds/include/nd.h:145-146) and cimg_stride, cimg_zero (nd_int.h:22).
   Readers to edit: nd_target.c (16 lines), nd_rect.c:88 :93 :98 :118 :230 :234 :295 :298, nd_direct.c:86-87, nd_state.c:10 :37, 3ds/tools/nd_direct_test.c (3 lines). Grep again first.
4. Delete img_role. Its 12 callers call one inline helper in nd_tgt.h that passes nd_tgt and nd_ws.omode_l, nd_ws.omode_h (S9DEL2 replaces these 2 inputs with the draw's pso):
   nd_comb.c:214 :262, nd_direct.c:86, nd_geom.c:197 :219, nd_rect.c:236 :313 :314, nd_set.c:211 :239, nd_shape.c:718 :771. Test stub: nd_direct_test.c:52.
5. Record side (3ds/include/nd_geo.h, 3ds/src/nd_geo.c): `nd_target_begin(img, siz, width)` and `nd_target_end()` (the main colour buffer: (uint32_t)(uintptr_t)nuGfxCfb_ptr;
   osVirtualToPhysical is the identity for host addresses, 3ds/src/pm_os.c:59-62). Each records one NDC_NAPI record (nd_rec_napi, nd_api.h:11) with runner nd_tgt_run (nd_target.c).
   nd_tgt_run does what nd_walk_state does for one NDO_MISC_CIMG unit: the same skip test and op prologue (read nd_walk_state in 3ds/src/nd_walk.c; gv_epoch must change as today), then set_cimg.
6. Game sites: every nd_tgt_cimg call (FACT grep: 31 sites in 17 src files, plus model.c:3962 after GPOSREST = 32). An image other than nuGfxCfb_ptr: nd_target_begin. nuGfxCfb_ptr (any cast): nd_target_end.
   Then delete nd_tgt_cimg, nd_tgt_cimgw (nd_geo.h:18-19, nd_geo.c:14-17). The walker arm NDO_MISC_CIMG (nd_walk.c:346) stays for the static lists (src/effects/gfx/flame.c:38, :50).
7. No division, no double, no toggle, no stub. Keep the debug output unchanged.
## Edit shapes
Before (src/screen_render_util.c:151): `nd_tgt_cimg(G_IM_FMT_RGBA, G_IM_SIZ_32b, 64, osVirtualToPhysical(nuGfxZBuffer));` After: `nd_target_begin(osVirtualToPhysical(nuGfxZBuffer), G_IM_SIZ_32b, 64);`
Before (src/screen_render_util.c:204): `nd_tgt_cimg(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, osVirtualToPhysical(nuGfxCfb_ptr));` After: `nd_target_end();`
## Tests that decide
1. New 3ds/tools/nd_tgt_test.c and 3ds/tests/host.d/ntgt.sh: a copy of today's img_role body with its tests as inputs against nd_tgt_role, every combination of
   the kind bits, siz 0-3, for_fill 0-1, cycle type 0-3, oml render mode VISCVG or not, oml bits 0x30, zcol 0-1. Print `ntgt ok <n>`. Seeded fault (one branch changed in a copy): `ntgt FAIL`.
2. geounit (3ds/tests/host.d/geounit.sh): replace the nd_tgt_cimg case with nd_target_begin and nd_target_end record words.
3. `sh 3ds/tools/nd_host_test.sh` after the change (one time): `ND HOST TESTS PASS`, `lkey census ok`, `ntgt ok`.
4. One build: rc 0, memcheck margin above 0. listcen and `python 3ds/tools/elfgate.py` into build3ds/out/ntgt_lc.txt (func 1 lower than main).
5. Gate (at most 6 Azahar starts, base runs not charged): spot_mac01_drec, wscmb_pause_drec, wscmb_fsel_drec, fxr_drec. `bench.py base <b>`,
   `bench.py run <b> --row NTGT --why "NTGT gate"`, `bench.py cmp <b> --row NTGT --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
## Loop and stop
Group 1: rules 1-4 and test 1 (commit `NTGT 1`). Group 2: rules 5-6 and test 2 (commit `NTGT 2`). Then tests 3-5 (commit `NTGT 3: gate`, deletes this page).
Fix scope: the files named on this page. Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle.
Hand-back: commit, listcen totals, elfgate line, gate values, memcheck margin.
