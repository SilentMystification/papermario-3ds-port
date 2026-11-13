# Row NPSO4: psochk, the compare of a stated NdPsoDesc with the walker's state at the draw (the proof that a description is complete)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main of NPSO2 ac0e68cd5)
- comb_in_walker(NdCombIn*) (3ds/src/nd_comb.c:476-482) fills w0, w1, omode_h, key_c, key_s, conv_k4, tex_on, use_t1, t1_valid,
  dual_on, box_prod, box_folded, comb_set, comb_alpha_shade, leg_mode from the walker (nd_ws and the nd_comb.c globals).
  NdPsoDesc (nd_api.h, NPSO1) also holds oml (nd_ws.omode_l), geom (nd_ws.geom), vprim (pm_comb_vprim, pm_comb.c:312), zimg
  (img_role(0) == ROLE_DEPTHQ, nd_int.h:16) and plod ((uint8_t)nd_ws.prim_lod).
- No draw states a pipeline id yet (NPSO5 and CONV2D will). So this row builds the tool and proves it on the host; the device
  hook gets its first caller in NPSO5.
Start: `sh 3ds/tools/wt_new.sh NPSO4`. PM_ROW=NPSO4 PM_LANE=NPSO4 on every call. Blocker NPSO2: merged.
## Touch only (places)
3ds/src/nd_comb.c: one new function right after comb_in_walker (after :482). New 3ds/src/nd_psochk.c, 3ds/include/nd_psochk.h,
3ds/tools/nd_psochk_test.c, 3ds/tests/host.d/psochk.sh. Nothing else.
## Exact edits
1. nd_comb.c after comb_in_walker: `void nd_walker_desc(NdPsoDesc* d)`: `NdCombIn in; comb_in_walker(&in); memset(d, 0, sizeof *d);`
   then d->cw0 = in.w0, cw1 = in.w1, omh = in.omode_h, oml = nd_ws.omode_l, geom = nd_ws.geom, key_c = in.key_c, key_s = in.key_s,
   conv_k4 = in.conv_k4, tex_on = in.tex_on, use_t1 = in.use_t1, t1_valid = in.t1_valid, dual_on = in.dual_on, box_prod =
   in.box_prod, box_folded = in.box_folded, comb_set = in.comb_set, comb_alpha_shade = in.comb_alpha_shade, leg_prev =
   in.leg_mode, vprim = pm_comb_vprim, zimg = img_role(0) == ROLE_DEPTHQ, plod = (uint8_t)nd_ws.prim_lod.
2. nd_psochk.h / nd_psochk.c (pure C, host and device): `int nd_psochk_cmp(const NdPsoDesc* stated, const NdPsoDesc* walker,
   char* out, unsigned n)`: 0 when every field is equal, else the count of differing fields and in out the first one as
   `<field> stated <v> walker <v>` (hex for the words). On the device: `void nd_psochk_draw(const NdPsoDesc* stated)`: when the
   switch psochk is on (pm_debug_has, read once), calls nd_walker_desc and nd_psochk_cmp, counts draws and bad, logs the first 8
   bad ones `psochk bad <field> ...` and every 100 frames `psochk f<n> draws <d> bad <b>` (pm_log_evt). Off: one test.
   Split (no #ifdef): nd_psochk.c holds only nd_psochk_cmp (host-testable). nd_psochk_draw goes into nd_comb.c right after
   nd_walker_desc (edit 1's place) and calls both.
3. nd_psochk_test.c: two descs that differ in oml (0x00552048 vs 0x00552078) and one equal pair: cmp gives 1 with out starting
   `oml stated 00552048 walker 00552078`, and 0. A third case differs in tex_on and plod: cmp gives 2 and names tex_on first
   (field order of NdPsoDesc). Print `psochk ok 3/3` or `psochk FAIL <case>`, exit 1.
4. host.d/psochk.sh: `gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/psochk 3ds/tools/nd_psochk_test.c 3ds/src/nd_psochk.c && /tmp/psochk`.
5. Commit: `NPSO4: psochk compare (nd_psochk.c), walker description and the device hook in nd_comb.c, host test`.
## Gate (expected values)
1. Seeded fault (a value change): in nd_psochk_cmp skip the oml field (compare `d->omh` twice). nd_host: `psochk FAIL` for the
   oml case. Restore: `psochk ok 3/3`, `ND HOST TESTS PASS`.
2. Build, memcheck OK. No Azahar: no draw calls nd_psochk_draw yet (NPSO5 adds the first call and runs the scene with psochk).
## Stop and end
A field of NdPsoDesc that comb_in_walker and nd_ws cannot give (hand back its name), a failed gate, or a tool fault: stop, commit,
hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
