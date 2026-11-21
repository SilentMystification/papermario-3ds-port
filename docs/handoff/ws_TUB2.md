# ws_TUB2: 5 more sub effects take the state of their call (plan_tubba.md section 3, shapes A, B, C)
Row TUB2. Model Haiku. Start main after the TUB0 merge (parallel with TUB1: other files). Worktree build3ds/wt/TUB2, branch TUB2, PM_ROW=TUB2, PM_LANE=TUB2. Oracle TUBBA.
Objective: no sub range draws with the state of nd_sub_open. C1..C12: none. Row metric: subst lines with n 532 to 751 (FACT before: the TUB0 gate lines; after: 0).
Files, in this order: src/effects/hieroglyphs.c, stop_watch.c, thunderbolt_ring.c, firework.c, whirlwind.c. CRLF: use the Edit tool. Edit no other file.
## Shape B (other state per call): hieroglyphs.c, stop_watch.c. Example hieroglyphs.c:141-178.
- Before: `nd_sub_open(&sub);` (:141), body (:142-166), `nd_sub_close(&sub);` (:167), and in the loop `nd_sub_call(&sub);` (:178).
- After: the body moves without a change into a static function directly above `void hieroglyphs_appendGfx(void* effect) {`:
```c
/* TUB2: one pass. Each call records it after its own prim and env: a sub range keeps the state of nd_sub_open (plan_tubba.md). */
static void hieroglyphs_body(s32 lifeTime) {
    Matrix4f sp20, sp60;
    s32 i;
    /* the lines of hieroglyphs.c:142-166, unchanged */
}
```
  Delete :141 and :167. `nd_sub_call(&sub);` (:178) becomes `hieroglyphs_body(lifeTime);`. Delete `NdSub sub;` (:124).
- stop_watch.c: `static void stop_watch_body(Vtx_t* vtxBuffer)` with locals `Matrix4f sp20; s32 i;` and the lines :234-246. Delete :233 and :247.
  `nd_sub_call(&sub);` (:255) becomes `stop_watch_body(vtxBuffer);`. Delete `NdSub sub;` (:190).
## Shape A (one state for all calls): move the state lines directly above nd_sub_open. Example thunderbolt_ring.c:
- Before: :121 `nd_sub_open(&sub);`, ..., :136 `nd_sub_close(&sub);`, :137 `nd_val_prim(0, 0, data->unk_18, data->unk_1C, data->unk_20, unk_24);`, :138 `nd_val_env(255, 255, 255, 0);`.
- After: the lines :137-138 stand directly above :121 (after the nd_draw_list of :119). :136 `nd_sub_close(&sub);` is followed by the blank line and the loop.
- firework.c: move :303 `nd_val_prim(0, 0, primR, primG, primB, primA);` and :304 `nd_val_env(envR, envG, engB, envA);` directly above :291
  `nd_sub_open(&sub);` (indent 12 spaces). The three nd_sub_call lines stay.
## Shapes C + A: whirlwind.c
- Delete :207-215 (the triSub comment, `nd_sub_open(&triSub);`, the loop of 15 `nd_mesh_tri2`, `nd_sub_close(&triSub);`).
- :261 `nd_sub_call(&triSub);` becomes `for (j = 0; j < 15; j++) nd_mesh_tri2(j + 0x10, j + 1, j, 0, j + 0x10, j + 0x11, j + 1, 0); /* TUB2: after the tile sizes of this segment */`.
- :221-222 (the comment and `nd_sub_open(&mainSub);`) become the 3 lines of :265-267 (`nd_pso_geom_clr(G_CULL_BOTH);`, the nd_val_prim, the nd_val_env).
- Delete :264-270 (`nd_sub_close(&mainSub);`, the 3 old state lines, the blank line, the comment, `nd_sub_call(&mainSub);`). The 2 nd_xf_pop lines stay.
- :163 `NdSub triSub, mainSub;` becomes `s32 j;`.
- The build gives no new warning. Do not change any other line.
## Gate (1 build, 2 Azahar row runs, the bases from the tool)
1. `PM_LANE=TUB2 sh 3ds/build.sh`: memcheck OK. `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS. c0gate: calls 0 interp 0 state 0.
2. `python 3ds/tools/bench.py base <b> --commit $(git merge-base HEAD main)` for subcol_drec and fxparts_drec.
3. `bench.py run subcol_drec,fxparts_drec --row TUB2 --why "TUB2 gate"`.
4. `bench.py cmp subcol_drec --row TUB2 --expect fxsp`: `expected-diff PASS` (exit 2), or byte-equal when no slot 3-7 effect drew. `cmp fxparts_drec`: byte-equal.
5. build3ds/azt/logs/subcol_drec.txt: 0 `subst f<n>` lines with n 532 to 795. The seeded fault of this check is main (the TUB0 gate lines).
6. Record `python 3ds/tools/drec_state.py tdiff --bench subcol_drec --row TUB2` (the tdiff line, no pass rule).
Fix scope: the 5 effect files. Stop: step 4 FAIL, fxparts DIFF, or a subst line in 532 to 795: SendMessage TUBBA with the output path.
Hand-back: commit hash, the gate values, the tdiff line. The last commit deletes this page.
