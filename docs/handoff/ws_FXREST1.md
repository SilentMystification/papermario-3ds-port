# ws_FXREST1: capture and class file of the 21 effect nd_rec_dl rest sites (moves C10 recdl when a site applies)
Model: Haiku. PM_ROW, PM_LANE = FXREST1. Branch FXREST1, worktree build3ds/wt/FXREST1. Budget: 2 builds, 6 Azahar runs. Start now (main 75d442afb).
FACT (grep src/effects, 75d442afb): 22 `nd_rec_dl(` lines; listcen recdl 22. chapter_change.c:163 stays `nd_rec_dl` (user). Never edit chapter_change.c.
Files: the 21 site files of src/effects below, new 3ds/tools/fxrest.d/ (class files). Tools as they are on main: entcap.py, entapply.py, benches fxr_cap, fxr_chk, fxr_drec (bench.d/FXENT3.json).
## Group 1: keyed capture and class file (1 build, 1 run, commit `FXREST1 1: ...`)
1. Put the FXENT3 keys back (the line numbers are main 75d442afb). fxk = `s32 fxk = 0;` on a new line before the named `for (` line, and `fxk = 1;` on a new line after the call.
   cold_breath:234 `nd_rec_dl_k(D_E00DE810[unk00][temp_s32], unk00 * 2 + cond, 6);` | gather_energy_pink:224 `nd_rec_dl_k(dlist2, unk_00 * 2 + fxk, 4);` fxk, for :205
   green_impact:202 `nd_rec_dl_k(dlist2, fxk, 2);` fxk, for :186 | misc_particles:330 `nd_rec_dl_k(D_E00E4D90[variation], variation * 2 + fxk, 12);` fxk, for :317
   red_impact:209 `nd_rec_dl_k(dlist2, fxk, 2);` fxk, for :193 | shockwave:341 `nd_rec_dl_k(dlist, (temp_v0 >= 2) * 2 + fxk, 4);` fxk, for :326
   recover:240 `nd_rec_dl_k(D_090033F0_3851F0, (type == 0 || type == 2) ? (unk_44 == 0 ? 0 : 1) : 2, 3);` | ring_blast:141 `nd_rec_dl_k(dlist, unk_00 != 0, 2);`
   stat_change:274 `nd_rec_dl_k(D_E00AC7BC[arrowType], (data->unk_24 != 255) * 3 + arrowType, 6);`
   Example. Before (green_impact.c:186, :202) `    for (i = 1; i < ...` .. `            nd_rec_dl(dlist2);`
   After `    s32 fxk = 0;` / `    for (i = 1; i < ...` .. `            nd_rec_dl_k(dlist2, fxk, 2);` / `            fxk = 1;` (the indent of the call line).
   The 12 plain sites stay plain: chomp_drop:164, :167, damage_indicator:286, :288, disable_x:308, :310, explosion:198, recover:223, landing_dust:331,
   shattering_stones:172, something_rotating:316, star_spirits_energy:400.
2. nd_host PASS. One build rc 0. `bench.py run fxr_cap --row FXREST1 --why "FXREST1 capture"`.
   `python 3ds/tools/entcap.py <log> > build3ds/out/fxrest_cap.txt`. B0 = the `bad` value of the last psochk line of the log.
3. `python 3ds/tools/entapply.py --dry build3ds/out/fxrest_cap.txt > build3ds/out/fxrest_dry.txt`.
4. Class file 3ds/tools/fxrest.d/classes.txt, one line per site of step 1 (21 lines): `<file>:<line> <class> <detail>`. class = the entapply word of that site in fxrest_dry.txt
   (site, keyed, so, multi, kmiss, mixed, skip). No entapply line and no fxrest_cap.txt line for the site: class `unseen`. detail = the rest of the entapply line.
   3ds/tools/fxrest.d/cap.txt = the lines of fxrest_cap.txt whose `src/effects/<f>.c:<line>` (with or without `#<k>`) is a site of step 1. Write B0 in the commit text.
5. Put every site of step 1 back to its main text (`nd_rec_dl(<arg>);`, delete the fxk lines). `git diff main -- src` is empty. Commit only 3ds/tools/fxrest.d/.
## Group 2: apply (only when classes.txt has a site, keyed or so line; else skip to the hand-back) (1 build, 4 runs, commit `FXREST1 2: ...`)
6. Do step 1 again only for the keyed sites whose class is keyed. `python 3ds/tools/entapply.py build3ds/out/fxrest_cap.txt > build3ds/out/fxrest_apply.txt`.
   entapply edits only the sites with class site, keyed or so. Each `nd_rec_dl_k(` line left: back to main text, delete its fxk lines. `grep -rn "nd_rec_dl_k(" src` is empty.
7. nd_host PASS, one build rc 0. `bench.py run fxr_chk --row FXREST1 --why "FXREST1 2"`: the `bad` value of its last psochk line equals B0.
8. `bench.py base fxr_drec --row FXREST1 --commit $(git merge-base HEAD main)`, `bench.py run fxr_drec --row FXREST1 --why "FXREST1 2"`,
   `bench.py cmp fxr_drec --row FXREST1 --commit $(git merge-base HEAD main)`: byte-equal. listcen recdl before and after. Commit.
Fix rights: the files above. A failed gate in step 7 or 8: put that site back to main text, 2 tries, then the stop rule (CLAUDE.md 5).
Stop: group 1 committed and group 2 committed or skipped. Delete this page in the last commit. Send the path of classes.txt to the oracle (the next page designs the keys).
Hand-back file, first 8 lines: commits, class counts (site keyed so multi kmiss mixed skip unseen), B0, fxr_chk bad, fxr_drec cmp, recdl before and after, Azahar runs.
