# ws_FXREST2: entries for the 10 multi and kmiss effect sites (moves C10 recdl 22 -> 12 when all apply, GUESS)
Model: Haiku. PM_ROW, PM_LANE = FXREST2. Branch FXREST2, worktree build3ds/wt/FXREST2. Budget: 3 builds, 5 Azahar runs. Start now (main 49f252400).
FACT (git grep, 49f252400): 22 `nd_rec_dl(` in src/effects. chapter_change.c:163 stays. Never edit chapter_change.c or the 11 other rest sites.
Design (oracle). geom, plod, key_c, key_s are draw values: nd_pso_build reads no geom (nd_decide.c:346-365); nd_pso_colour takes plod and keys per draw (nd_decide.c:380).
The compare drops them, entcap writes them as 0. Real pipelines get a branch key. B0 666 no longer applies: the compare changes. B1 (step 4) is the new base.
Files: 3ds/src/nd_psochk.c nd_walk.c nd_comb.c pm_fxtest.c, 3ds/include/nd_psochk.h, 3ds/tools/nd_psochk_test.c entapply.py, azt.d/FXENT3.json (step 1f only), fxrest.d/cap2.txt (new), the 9 site files of step 2.
## Group 1: tools, keys, capture (1 build, 1 run, commit `FXREST2 1: ...`)
1. a. nd_psochk.c:12-16: delete the 4 items `PSOCHK_F(geom, "%08x"); ` `PSOCHK_F(key_c, "%08x"); ` `PSOCHK_F(key_s, "%08x"); ` `PSOCHK_F(plod, "%02x");`. Keep every other item.
   Before (:13) `    PSOCHK_F(key_c, "%08x"); PSOCHK_F(key_s, "%08x"); PSOCHK_F(conv_k4, "%08x");`  After `    PSOCHK_F(conv_k4, "%08x");`
   b. nd_psochk_test.c:29: `if (r == 2 &&` -> `if (r == 1 &&`. Add case 4 before the printf: base_desc both, `w.geom = 1; w.key_c = 2; w.key_s = 3; w.plod = 4;`, pass when r == 0
      (else print `psochk FAIL draw`, return 1). Last line prints `psochk ok %d/4`; comment line 1 says four cases.
   c. nd_walk.c:511: after `    nd_walker_desc(&d);` add the line `    d.geom = 0; d.key_c = 0; d.key_s = 0; d.plod = 0; /* FXREST2: draw values, not entry state */`.
   d. nd_comb.c: after line 233 (the end of nd_psochk_draw) add
      `void nd_psochk_end(void) { if (psochk_on()) pm_log_evt("psochk end f%u draws %u bad %u lk %u miss %u lkbad %u", pm_frame_n, psochk_draws, psochk_bad, pk_hit, pk_miss, pk_bad); }`
      nd_psochk.h: after the nd_psochk_draw line, and pm_fxtest.c after line 16, add `void nd_psochk_end(void);`. pm_fxtest.c:271 before `if (slot == nsel && k == 0) pm_log_evt("fxtest: done");`
      after `if (slot == nsel && k == 0) { nd_psochk_end(); pm_log_evt("fxtest: done"); }`.
   e. entapply.py:30: `SITE = re.compile(r"(src/effects/\w+\.c):(\d+)(?:#(\d+))?")` -> `SITE = re.compile(r"(src/effects/\w+\.c):(\d+)(?: \(discriminator \d+\))?(?:#(\d+))?")` (FACT cap.txt:132-143).
   f. pm_fxtest.c: after line 171 `E(rising_bubble_a30, ...))` put ` \` on it and add 13 lines (indices 140-152, FACT 140 entries now: lines 32-171):
      cold_breath_t1 `fx_cold_breath(1, X, Y, Z, 2.0f, 45)`, cold_breath_t2 (type 2), ring_blast_t1 `fx_ring_blast(1, X, Y, Z, 4.0f, 20)`, shockwave_t2 `fx_shockwave(2, X, Y, Z)`,
      recover_n0 `fx_recover(0, X, Y, Z, -15)`, recover_n2 `fx_recover(2, X, Y, Z, -15)`, stat_change_a3 `fx_stat_change(3, X, Y, Z, 1.5f, 60)`, stat_change_a6 (arg 6),
      misc_particles_v0, _v2, _v3, _v4, _v5 `fx_misc_particles(<v>, X, Y, Z, 24.0f, 24.0f, 1.0f, 5, 0)`. Form: `E(cold_breath_t1, fx_cold_breath(1, X, Y, Z, 2.0f, 45)) \`; no ` \` on the last.
      azt.d/FXENT3.json:8 and :40: `123,127,128,` ... `139 loglevel` -> `123,127-152 loglevel` (170 characters, run_azahar.ps1:89 stops above 255). Do not edit :68 (fxr_drec).
   g. pm_fxtest.c:271-274 (range form a-b). Before `n = atoi(p);` / `if (n >= 0 && n < FXT_COUNT) sel[nsel++] = n;` After
      `char* d = strchr(p, '-'); int m;` / `n = atoi(p); m = d ? atoi(d + 1) : n;` / `for (; n <= m; n++) if (n >= 0 && n < FXT_COUNT && nsel < FXT_COUNT) sel[nsel++] = n;`
      Check: the fxr_cap log has `fxtest: 56 effects selected of 153`.
2. Keys (main line numbers). Before (ring_blast.c:141) `    nd_rec_dl(dlist);` After `    nd_rec_dl_k(dlist, unk_00 != 0, 2);`. Keep the indent.
   cold_breath:234 `nd_rec_dl_k(D_E00DE810[unk00][temp_s32], unk00, 3);` | gather_energy_pink:224 `nd_rec_dl_k(dlist2, unk_00, 2);` | shockwave:341 `nd_rec_dl_k(dlist, temp_v0 >= 2, 2);`
   misc_particles:330 `nd_rec_dl_k(D_E00E4D90[variation], variation, 6);` | stat_change:274 `nd_rec_dl_k(D_E00AC7BC[arrowType], (data->unk_24 != 255) * 3 + arrowType, 6);`
   recover:223 `nd_rec_dl_k(D_090033D0_3851D0, (part->alpha != 255) + 2 * unk_44, 4);` | recover:240 the same key with D_090033F0_3851F0. green_impact:202, red_impact:209 stay plain.
3. `sh 3ds/tools/nd_host_test.sh` PASS (psochk ok 4/4). One build rc 0. `bench.py run fxr_cap --row FXREST2 --why "FXREST2 capture"`.
4. `python 3ds/tools/entcap.py <log> > build3ds/out/fxrest2_cap.txt`. B1 = the `bad` value of the `psochk end` line of the log. cap2.txt = the fxrest2_cap.txt lines of the 10 sites.
5. `python 3ds/tools/entapply.py --dry build3ds/out/fxrest2_cap.txt > build3ds/out/fxrest2_dry.txt`. Commit the files of step 1, the step 2 sites and cap2.txt. Write B1 in the commit text.
## Group 2: apply and gate (1 build, 4 runs, commit `FXREST2 2: ...`)
6. `python 3ds/tools/listcen.py` recdl before. `python 3ds/tools/entapply.py build3ds/out/fxrest2_cap.txt > build3ds/out/fxrest2_apply.txt`.
   `git diff --name-only -- src/effects`: a file outside the 9 site files: `git checkout -- <file>`. A site of step 2 with no `entapply site` or `entapply keyed` line
   (kmiss, multi, skip): put its line back to the main text `nd_rec_dl(<arg>);`. `git grep -n "nd_rec_dl_k(" -- src` is empty.
7. nd_host PASS. One build rc 0. A build stop `gen_pso_main: one key with two combs (<name>)`: put the site of <name> back to main text, delete its ND_PSO_DESC lines, build again (2 tries).
   `bench.py run fxr_chk --row FXREST2 --why "FXREST2 2"`: the `bad` value of its `psochk end` line equals B1.
8. `bench.py base fxr_drec --row FXREST2 --commit $(git merge-base HEAD main)`, `bench.py run fxr_drec --row FXREST2 --why "FXREST2 2"`,
   `bench.py cmp fxr_drec --row FXREST2 --commit $(git merge-base HEAD main)`: byte-equal. listcen recdl after. Delete this page. Commit.
Fix rights: the files above. A failed gate in step 7 or 8: put the sites of that gate back to main text, 2 tries, then the stop rule (CLAUDE.md 5).
Stop: group 2 committed. Hand-back file, first 8 lines: commits, entapply counts (sites keyed multi kmiss skip), B1, fxr_chk end bad, fxr_drec cmp, recdl before and after,
sites put back (file:line and entapply word), Azahar runs.
