# Row FILLA0R: revert FILLA0 (the alpha test on blended draws made the effects slower on the console)

(Model: Haiku. Base: main 5fa808909 or later. Edits 3ds/src/nd_tmpl.c (:313-315) and 3ds/tools/nd_tmpl_test.c (:72-73) only. EDGEAA stays
(nd_tmpl.c:302, cutout alpha test 0x30). Disjoint from PMUPHFIX2 (pm_phase.c). Reads this file and CLAUDE.md.)

## User fact (XL fxtest, build 4340 with FILLA0 vs build 4324 without; draw counts equal)
fire_breath 0.08 -> 17.1 ms, snowfall 0.73 -> 12.6, water_splash 0 -> 10.2, water_block 1.4 -> 10.3, energy_shockwave 1.1 -> 6.1,
pink_sparkles 0.08 -> 6.0, embers 1.2 -> 3.6, effect_3D 0.46 -> 3.1. TILEWIN2 holds (whirlwind 0.68, effect_63 0.65).

## Cause (oracle code read)
- FILLA0 (commits 0be852ce9, c1fc8ac95) is the only change between the runs: nd_render_state (nd_tmpl.c:313-315) sets atest 0 (alpha
  test GREATER 0) for blended draws without a depth write; nd_tmpl.c:377 writes it as R_ALPHA_TEST on, and the template key holds atest
  (k->atest_on, k->aref), so these draws use other templates than before. The picture stayed byte-equal on Azahar, but Azahar does not
  model the PICA200 fill cost. On the console these draws became fill-bound: the alpha-tested path is slower per fragment than the plain
  blend it was meant to save (the GPU does not skip the blend work for a discarded fragment the way FILLA0 assumed). The code read cannot
  show the hardware rule; the console numbers decide: revert.
- EDGEAA (cutouts, atest 0x30) is the long-standing pre-KMR11W rule and was in the fast run (build 4324 included it): not affected.

## Start
`sh 3ds/tools/wt_new.sh FILLA0R`. PM_ROW=FILLA0R PM_LANE=FILLA0R on every build and bench call.

## Exact edits
1. nd_tmpl.c: delete the FILLA0 comment and line (:313-315: `/* FILLA0: ... */` and `if (o->blend && o->atest < 0 && !o->zupd && !o->nocol && !o->memb) o->atest = 0;`).
2. nd_tmpl_test.c: delete the FILLA0 comment and its exception line (:72-73: `/* FILLA0: ... */` and `if (rs.blend && !rs.zupd ... ) rs.atest = -1;`).

## Gate (CLAUDE.md 2; Azahar budget 4 of 8)
- Build, memcheck OK; `bench.py run nd_host --row FILLA0R --why "FILLA0 removed"`: PASS.
- `bench.py base spot_mac01_det --commit c16fd02a0` and `bench.py base mdl5_iwa10_det --commit c16fd02a0` (the pre-FILLA0 base);
  one list `bench.py run spot_mac01_det,mdl5_iwa10_det --row FILLA0R --why "same picture as before FILLA0"`; `bench.py cmp spot_mac01_det
  --row FILLA0R --commit c16fd02a0` and the same for mdl5_iwa10_det: write the byte-equal counts (other merges since c16fd02a0 may differ:
  name them if a shot differs). The console fxtest run is the user's check.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line.
