# Row S6VORD: S6T value order: no value unit between two static units; MW none is a no-op; s6chk compares fog, scissor, lights

(Model: Haiku. Every edit is below. No blocker. S6KEY may run at the same time: it edits sc_exec, ND_SC_KEYW and nd_scomp_test.c:66.)

## Cause (ORACLE6 code read, main 8301c42fb)
- sc_exec (3ds/src/nd_scomp.c:202-221) runs the `pre` value units, then the folded static block (all static units at once), the
  decision, then the other value units. The op path runs each unit in place. nd_scomp_analyse refuses only class-4 units between
  two static units (:44, :64); a class-2 unit there (SCISSOR, CIMG, PRIMZ, FOG, BLEND, FILL, MTX, MOVEMEM, MW) moves behind the
  later static units. Setters with side effects that depend on that order: gbi_movemem and nd_light_count run lit_freeze
  (nd_mtx_set.c:181, :226), set_om flushes on a filter change (nd_set.c:88-91), pm_gpu_set_mode and pm_gpu_set_scissor flush.
  S6E proved the effect (spot_mac00_det 0/6 until ent_gap took nv == pre, nd_ent.c:393).
- MW none (clip ratio, persp normalize) is class 1 (:23) but set_mw has no arm for it (nd_set.c:107-117): a no-op that counts as a
  static unit, so a value after it is "between statics".
- Defs on main with a value between two statics: cam_range (src/cam_main.c:119-142: Scissor), cb_r1 (crystal_ball_gfx.c:29-35:
  ColorImage), cb_r2 (:36-47: PrimDepth). The new rule refuses them (op path), so this row splits each at its value (exact order).

## Start
`sh 3ds/tools/wt_new.sh S6VORD`. PM_ROW=S6VORD PM_LANE=S6VORD on every call.

## Touch only
3ds/src/nd_scomp.c: unit_class, nd_scomp_analyse, the s6chk block (S6_FIELDS to s6_run). 3ds/tools/nd_scomp_test.c: cam_u, two new
arrays after mid_u, the checks after line 90. src/cam_main.c: cam_range_u/cam_range and the record line :258 (and the :185 comment).
src/world/area_mac/mac_01/crystal_ball_gfx.c: cb_r1, cb_r2 and their record lines :162, :176.

## Exact edits
1. nd_scomp.c:23 -> `case NDO_MW: return (a >> 16) == NDO_MW_NONE ? 3u : 2u; /* S6VORD: clip ratio, persp normalize: no field, a no-op */`.
   :16 comment: add "class 2 too must not lie between two static units (S6VORD: sc_exec runs it after the block)".
2. nd_scomp.c:44 `if (c == 4u) om |= 1u << k;` -> `if (c == 2u || c == 4u) om |= 1u << k; /* S6VORD: no value unit between statics */`.
3. s6chk: in S6_FIELDS, before `X(sc_ok, sc_ok)`, add `X(fogc, fogc) X(blendc, blendc) X(fog_fm, fog_fm) X(fog_fo, fog_fo) X(prim_z,
   nd_ws.prim_z) X(fill, nd_ws.fill) X(sci_x0, sci_x0) X(sci_y0, sci_y0) X(sci_x1, sci_x1) X(sci_y1, sci_y1) X(lt_n, lt_n)` (declared in
   nd_int.h:173-177). Lights: enum `S6F_plan, S6F_lt, S6F_N`; s6_name ends `"cur_plan", "lights"`; S6Snap gets `NdLight lt[9];`;
   s6_take `memcpy(s->lt, lt, sizeof s->lt);`; s6_put `memcpy(lt, s->lt, sizeof lt);`; s6_run after the plan compare
   `if (memcmp(a.lt, b.lt, sizeof a.lt)) { s6_fbad[S6F_lt]++; bad = 1; }`.
4. nd_scomp_test.c: in cam_u move the SCISSOR unit (:23) to right after the viewport (:19). Check :82 becomes `cam.nv == 3u &&
   cam.pre == 2u && cam.vmask == (1u | 1u << 1 | 1u << 9)` (text "... 2 before the first static"). After mid_u add `mid2_u` = {OMH
   cycle 1CYCLE, `NDO_W0(NDO_SCISSOR, 0), 0`, OML `OM_A(0, 2)` 0} and `mwn_u` = {`NDO_W0(NDO_MW, NDO_MW_NONE << 16), 0`,
   `NDO_W0(NDO_MTX, 0x07), 0`, OMH cycle 1CYCLE}; NdSDef mid2 and mwn in main written like `pre` (all zero fields: -Wextra);
   after :90: `check("scissor between static units is refused", nd_scomp_analyse(&mid2) == 0);` and `check("MW none is a no-op:
   the matrix is a pre value", nd_scomp_analyse(&mwn) == 1 && mwn.nv == 1u && mwn.pre == 1u);`.
5. cam_main.c: cam_range_u splits after `gsDPSetScissor(...)`: `cam_a_u[]` = Viewport .. Scissor (6 units), `cam_b_u[]` =
   TextureLOD .. Matrix (the rest, unchanged order); `static NdSDef cam_a = { cam_a_u, sizeof cam_a_u / sizeof cam_a_u[0] }, cam_b
   = { cam_b_u, sizeof cam_b_u / sizeof cam_b_u[0] };`. :258 -> `nd_scomp_rec(&cam_a, scv, 2); nd_scomp_rec(&cam_b, scv + 2, 2);`
   (scv: viewport, scissor | colour image, matrix). Comment :185 "the value units of cam_a and cam_b".
6. crystal_ball_gfx.c: cb_r1_u -> `cb_r1a_u` {CycleType, RenderMode, ColorImage} and `cb_r1b_u` {CombineMode, PrimColor}; cb_r2_u
   -> `cb_r2a_u` {ColorImage, DepthSource, PrimDepth} and `cb_r2b_u` {RenderMode .. TextureLOD}; NdSDef as the file's form.
   :162 -> `nd_scomp_rec(&cb_r1a, v1, 1); nd_scomp_rec(&cb_r1b, v1 + 1, 1);`; :176 -> `nd_scomp_rec(&cb_r2a, v2, 2);
   nd_scomp_rec(&cb_r2b, NULL, 0);` (the p++ value lines stay as they are).

## Gate (expected values)
- Build, memcheck OK. `bench.py run nd_host --row S6VORD --why "S6VORD value order"`: PASS (SCOMP TEST PASS; gtab `bad 0`: the
  tool re-splits its defs with the new rule, so hand back the `gtab lists .. defs .. ops ..` line, counts may change).
- `bench.py base spot_mac01_det` (merge-base); one list `bench.py run spot_mac01_det,s6chk_mac01 --row S6VORD --why "S6VORD: cam
  and crystal ball split defs"` (2 starts, both normal). cmp 6/6 byte-equal. s6chk_mac01: `bad 0`, no `s6chk field` line; hand back
  `ranges` and `made` (ranges goes up: two records where one was). No `scomp: def` line. Budget 4.

## Stop
A byte difference, `bad` above 0, a test FAIL or a scomp line: stop, commit, hand back the lines (2 tries per named cause).
Delete this row file in the completing commit.
