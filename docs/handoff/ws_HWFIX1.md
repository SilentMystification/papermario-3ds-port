# ws_HWFIX1 (Haiku): a pso table miss decides with nd_comb_core again (console: white and black rectangles; no counter move)
Designer: oracle DIAG1. Worktree build3ds/wt/HWFIX1, branch HWFIX1 from main 9ef4f1fd8d or later. PM_ROW=HWFIX1 PM_LANE=HWFIX1.
Edit only 3ds/src/nd_comb.c, 3ds/src/nd_psokey.c (comment), the 8 path sites below; git mv 3ds/tools/nd_decide.c 3ds/src/nd_decide.c;
add 3ds/tests/host.d/hwfix1.sh, 3ds/tools/azt.d/HWFIX1.json, 3ds/tools/bench.d/HWFIX1.json.
## Cause (code read, FACT)
- nd_comb.c:264: a key that misses the pso table calls pso_fallback (:257-260). It zeroes the decision: use -1, plan_on 0, cc_on 0, leg 0.
  sync_combine_work (:320-330) then sets no plan and no constant: the draw shows vertex colour only, no texture. That is the
  white or black rectangle (shadow, particle, sprite, sign, menu grid, lit Mario on kkj_12).
- Before PSOFB (7733aa14d8 nd_comb.c:256) a miss ran nd_comb_core, the full decision. f6970ab99b moved nd_comb_core
  (nd_decide.c:230) to 3ds/tools, so the device build no longer has it. Misses are common: fxslot_cap lk 5185, miss 26091 (FACT).
- Azahar benches stay byte-equal only when every key is in the table (PSOFB gate: psoseen 0/0). NST1, NST3: core 1 or PMDL("dschk") only
  (nd_rec.c:191, :402). They do not change core 0 output.
## Edits
1. `git mv 3ds/tools/nd_decide.c 3ds/src/nd_decide.c` (its device part is in nd_psokey.c, so no symbol is defined twice).
2. Replace `3ds/tools/nd_decide.c` with `3ds/src/nd_decide.c` at: build.sh:211, host.d/coin.sh:6, :8, conv2c.sh:9, decide.sh:5,
   psolk.sh:12, psotab.sh:10, gen_pso_tab.py:129, and the comment nd_psokey.c:2 ("host only" -> "nd_decide.c, device and host").
3. nd_comb.c: delete :256-260 (the PSOFB comment and pso_fallback). Before :264, after the PSOLK comment (:262):
```c
static int psooff_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("psooff"); return v; } /* HWFIX1 gate: every key misses */
```
   :264 before: `    if (!nd_pso_find(in, (unsigned)pm_comb_vprim, o)) { pk_miss++; pso_fallback(in, o); return; }`
   :264 after:  `    if (psooff_on() || !nd_pso_find(in, (unsigned)pm_comb_vprim, o)) { pso_seen_note(in); pk_miss++; nd_comb_core(in, (unsigned)pm_comb_vprim, comb_mode5, o); return; }`
4. hwfix1.sh (form of host.d/barricade.sh): check A: 3ds/src/nd_comb.c holds the :264 after line once. B: no `pso_fallback` in 3ds/src.
   C: 3ds/src/nd_decide.c has `void nd_comb_core(`. Seeded fault: a copy of nd_comb.c with `nd_comb_core(in, (unsigned)pm_comb_vprim,
   comb_mode5, o)` replaced by `pso_fallback(in, o)` must fail check A first. Pass prints `hwfix1 ok 3`.
5. HWFIX1.json (azt.d and bench.d): scenario and bench hwfix1_psooff, a copy of fxslot_drec (azt.d/FXSLOT.json:32) with ` psooff`
   added at the end of its debug line.
## Gates (1 build, 2 Azahar starts)
1. `PM_HOSTSEL=all sh 3ds/tools/nd_host_test.sh > build3ds/out/hwfix1_host.txt 2>&1`: ND HOST TESTS PASS, `hwfix1 ok 3`, psolk and decide pass.
2. `sh 3ds/build.sh > build3ds/out/hwfix1_build.txt 2>&1`: rc 0, memcheck line present.
3. `python 3ds/tools/bench.py run fxslot_drec --row HWFIX1 --why "HWFIX1 table"` and `python 3ds/tools/bench.py run hwfix1_psooff --row
   HWFIX1 --why "HWFIX1 all miss"`. `cmp` the two drec.txt files of their SHOTS lines: equal. With the fix a miss decides like a hit
   (psolk: hit = nd_comb_core), so all-miss equals table. Before the fix the all-miss run gives rectangles and a DIFF.
- No cmp against the merge-base: a miss changes the draw (expected, the fix). Commit 1 `HWFIX1 1: pso miss runs nd_comb_core again`.
## Fix rights: only the files above. A duplicate symbol, a host FAIL, a drec DIFF or a missing SHOTS line: stop, send the path to DIAG1.
## Console check (user, after merge): intro stage, shadows, walk dust, heart box, signs, Z spin, start menu grid and the menu switch,
  kkj_12 Mario. Each one shows its texture.
## Stop: last commit `HWFIX1 2: gate. hwfix1 ok 3, fxslot_drec = hwfix1_psooff drec byte-equal` deletes this page. One SubagentHandback.
