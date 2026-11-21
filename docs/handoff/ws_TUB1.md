# ws_TUB1: shockwave, green_impact, red_impact record each pass after its colours (plan_tubba.md section 3, shape B)
Row TUB1. Model Haiku. Start main after the TUB0 merge. Worktree build3ds/wt/TUB1, branch TUB1, PM_ROW=TUB1, PM_LANE=TUB1. Oracle TUBBA.
Objective: each pass of these effects draws with the prim, env and cull set just before it (the Tubba Blubba cloud fades). C1..C12: none.
Row metric: subst lines with n 400 to 531 in subcol_drec (FACT before: at least 1, the TUB0 gate; after: 0).
Files, in this order: src/effects/shockwave.c, src/effects/green_impact.c, src/effects/red_impact.c. CRLF: use the Edit tool. Edit no other file.
## Edit shape (shockwave.c:273-354). Do the same in green_impact.c:182-218 and red_impact.c:189-225.
1. Add a static function directly above `void <fx>_appendGfx(void* effect) {`. Its body is the part loop, moved without a change:
```c
/* TUB1: the ring parts. Each pass records them after its own prim, env and cull: a sub range keeps the state of nd_sub_open (plan_tubba.md). */
static void shockwave_parts(ShockwaveFXData* data, s32 numParts, NdLst dlist) {
    Matrix4f sp20, sp60;
    s32 i;

    data++;
    for (i = 1; i < numParts; i++, data++) {
        /* the lines of shockwave.c:326-342 (if (data->unk_4C >= 0) { ... }), unchanged */
    }
}
```
2. In appendGfx delete `nd_sub_open(&sub); ...` (:322), the moved loop (:324-343) and `nd_sub_close(&sub);` (:344).
3. Replace each `nd_sub_call(&sub);` (:349, :354) with `shockwave_parts(data, ((EffectInstance*) effect)->numParts, dlist);`.
   After: prim2, env2, geom_clr, geom_set(G_CULL_FRONT), shockwave_parts(...), prim, env, geom_clr, geom_set(G_CULL_BACK), shockwave_parts(...).
4. Delete the declarations that appendGfx no longer reads: shockwave `NdSub sub;` (:283), `s32 i;` (:285), sp60 of `Matrix4f sp20, sp60;` (:275).
- green_impact: `static void green_impact_parts(GreenImpactFXData* part, s32 numParts)`, locals `Matrix4f sp20, sp60; NdLst dlist2; s32 i;`,
  first statement `part++;`. Delete in appendGfx `NdLst dlist2;` (:140), `NdSub sub;` (:154), `Matrix4f sp60;` (:156), `s32 i;` (:157).
- red_impact: `static void red_impact_parts(RedImpactFXData* part, s32 numParts, f32 temp_1C, f32 temp_20, f32 temp_24)`, the same locals,
  first statement `part++;`. Delete `NdLst dlist2;` (:151), `NdSub sub;` (:168), `Matrix4f sp60;` (:170), `s32 i;` (:171).
- Call: `green_impact_parts(part, ((EffectInstance*)effect)->numParts);`, `red_impact_parts(part, ((EffectInstance*)effect)->numParts, temp_1C, temp_20, temp_24);`.
- The build gives no new warning. Do not change any other line.
## Gate (1 build, 2 Azahar row runs, the bases from the tool)
1. `PM_LANE=TUB1 sh 3ds/build.sh`: memcheck OK. `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS. c0gate: calls 0 interp 0 state 0.
2. `python 3ds/tools/bench.py base <b> --commit $(git merge-base HEAD main)` for subcol_drec and fxparts_drec.
3. `bench.py run subcol_drec,fxparts_drec --row TUB1 --why "TUB1 gate"`.
4. `bench.py cmp subcol_drec --row TUB1 --expect fxsp`: `expected-diff PASS` (exit 2). `bench.py cmp fxparts_drec --row TUB1`: byte-equal.
5. build3ds/azt/logs/subcol_drec.txt: 0 `subst f<n>` lines with n 400 to 531. The seeded fault of this check is main (the TUB0 gate: at least 1).
6. Record `python 3ds/tools/drec_state.py tdiff --bench subcol_drec --row TUB1` (the tdiff line, no pass rule) and the subst lines of n 532 to 795.
Fix scope: the 3 effect files. Stop: step 4 FAIL, fxparts DIFF, or a subst line in 400 to 531: SendMessage TUBBA with the output path.
Hand-back: commit hash, the gate values, the tdiff line. The last commit deletes this page.
