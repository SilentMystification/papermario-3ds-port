# ws_FXSELF: effect list calls whose every list is self-contained become nd_draw_list(arg, ND_SELF) (C10 recdl)
Model: Haiku. PM_ROW, PM_LANE = FXSELF. Budget: 2 Azahar runs. Start after `Merge CONV2C2` (gen_ntab_entries.py names) and `Merge TEX2` (TEX T2 owns src/effects)
show in `git log --oneline main`. Branch FXSELF (`sh 3ds/tools/wt_new.sh FXSELF`). Files: new 3ds/tools/selfapply.py, new 3ds/tests/selfapply/ (fixture), new
3ds/tests/host.d/selfapply.sh, src/effects/*.c (only the lines the tool writes).
FACT (listcen, main e0e468ff6): recdl 167, src/effects 127. GUESS (oracle scratch scan with the ntab files of 2026-11-13): 84 of the effects sites resolve to
self-contained lists only, 33 reach an inheriting list, 6 an unsupported list, 3 do not resolve. Only the 84 move here.
## Why it is draw-equal (code read)
nd_draw_list(l, ND_SELF) is nd_rec_dl_e(l, NULL) (nd_gtab.h:76-78): dl_ent stays NULL (nd_gtab.c:114), so nd_rec_dl records the same nd_gtab_run record as
before (nd_gtab.c:110-112). The device stream does not change. The tool decides only the source form.
## Group 1: the tool (host test first, commit `FXSELF 1: ...`)
`python 3ds/tools/selfapply.py [--dry] <ntab_tables.txt> <file or dir>...` (a dir: its *.c). tab = the set of lines of the tables file. A line
`^(\s*)nd_rec_dl\((.+)\);(.*)$` (the whole call on one line): `ns = names(text, arg)` imported from gen_ntab_entries.py (sys.path of the tool dir), then the exact line
`ok = ns is not None and len(ns) > 0 and all(n in tab for n in ns)`. ok: the line becomes `<indent>nd_draw_list(<arg>, ND_SELF);<rest>` and the tool prints
`selfapply site <file>:<line>`. Else it prints `selfapply keep <file>:<line> <the names not in tab, or skip>`. A call over more lines: keep. Last line:
`selfapply total site <n> keep <k>`. --dry: print only.
Fixture 3ds/tests/selfapply/a.c, tab.txt, a.out.c, a.log (write a.out.c and a.log by hand from the rule): balloon.c:13 array and :106 site (all names in tab.txt); a
plain name in tab.txt; a plain name not in tab.txt (keep); a local with two assignments, one not in tab.txt (keep); `nd_rec_dl(it->displayList);` (keep).
selfapply.sh: a seeded fault first (`sed 's/all(n in tab/any(n in tab/'` copy of the tool) must give a different a.c or log; then the real tool must give a.out.c and
a.log byte for byte. Prints `selfapply ok` or `selfapply FAIL ...` and exit 1.
## Group 2: the effect sites (1 build, 1 Azahar run, commit `FXSELF 2: ...`)
1. `sh 3ds/tools/nd_host_test.sh` (it writes build3ds/out/ntab_tables.txt of the worktree). 2. `python 3ds/tools/listcen.py > build3ds/out/lc_before.txt`.
3. `python 3ds/tools/selfapply.py build3ds/out/ntab_tables.txt src/effects > build3ds/out/selfapply.txt`.
Example (balloon.c:106) Before `    nd_rec_dl(D_E00963E0[idx]);` After `    nd_draw_list(D_E00963E0[idx], ND_SELF);`.
Example (attack_result_text.c:209) Before `            nd_rec_dl(D_E0090A40[data->unk_04]);` After `            nd_draw_list(D_E0090A40[data->unk_04], ND_SELF);`.
4. nd_host PASS, one build, memcheck margin 310 KB or more. listcen after: recdl falls by the site count of step 3 exactly, build, gfxpos, data unchanged.
5. `bench.py run fxpic_91_drec --row FXSELF --why "FXSELF 2"`, then `bench.py cmp fxpic_91_drec --row FXSELF --commit d29deacc3`: byte-equal.
Commit with the selfapply total line and the listcen recdl values. The commit deletes this page.
Fix rights: the new files and the tool output lines. Do not edit a keep line by hand. A cmp difference, a listcen difference other than recdl or a host FAIL:
no commit, build3ds/out/fxself.txt, send the path to the oracle (CLAUDE.md rule 5). Hand back: 8 lines (hashes, site and keep counts, recdl before and after, cmp;
details build3ds/out/handback_FXSELF.txt).
