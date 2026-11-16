# ws_DEADSRC (Haiku): delete the sources the device never builds; resolve the version branches of 8 files (C1 C2 C5 C6 C7)
Designer to ask: the display-list oracle (oracle DL1, SendMessage; the orchestrator gives the id). Worktree build3ds/wt/DEADSRC, branch DEADSRC.
PM_ROW=DEADSRC PM_LANE=DEADSRC on every bench call. Parallel with ws_GSETR (no shared file).
Objective (FACT, listcen and dry run on main 6a987519c3): build 738 -> 722, gfxpos 224 -> 201, data 3577 -> 3016, data_arrays 352 -> 277,
gfxtype 977 -> 885. FACT: no file below is in the link map build3ds/pm_3ds.map, and no device source includes it.
## Edit 1: delete (3 git rm commands)
`git rm -r -q src/world/dead src/bss src/level_up_letters_en_de.c src/level_up_letters_es.c src/level_up_letters_fr.c src/starpoint_de.c src/starpoint_en.c`
`git rm -q src/starpoint_es.c src/starpoint_fr.c src/titlemenu.c src/filemenu/filemenu_selectlanguage.c src/os/nusys/nugfxinit.c src/os/nusys/nugfxtaskmgr.c`
`git rm -q src/effects/gfx/chapter_change_de.c src/effects/gfx/chapter_change_es.c src/effects/gfx/chapter_change_fr.c`
Then delete the 3 lines `src/effects/gfx/chapter_change_de.c`, `..._es.c`, `..._fr.c` (lines 14-16) in each of 3ds/tools/gset.d/FXVROM.txt, PMOVE.txt and VSLOT2.txt.
Do not edit ver/*/splat.yaml (N64 build files).
## Edit 2: unver.py (new tool) and its gate script
Write block A of docs/handoff/ws_DEADSRC_code.md to 3ds/tools/unver.py. Write block B to build3ds/out/deadsrc_unver.sh (not committed).
`sh build3ds/out/deadsrc_unver.sh > build3ds/out/deadsrc_unver.txt 2>&1`. The script copies the 8 files, runs unver.py on them, and compares
the device preprocessor output (gcc -E -P in docker pm3ds-host:1) of each file before and after. Pass (last 2 lines): `unver seed ok` and `unver check ok`.
Expected (FACT, oracle dry run): `unver total chains 57 lines 966`, 8 lines `unver same <file>`.
Example before (src/entity/SaveBlock.c:23-26):
`#if VERSION_PAL` / `extern Gfx Entity_SaveBlock_RenderBlock_es[];` / `extern s32 gCurrentLanguage;` / `#endif`. After: the 4 lines are gone.
Example before (src/starpoint.c:6 and :10): `#if !VERSION_PAL` then `#if VERSION_JP`. After: the JP branch (lines 10-69) and the 4 directive lines are gone; the `#else` branch lines stay.
## Loop (1 build, at most 4 Azahar starts; base runs not charged)
1. Edit 1. Commit `DEADSRC 1: delete the sources the device never builds`.
2. Edit 2. Pass: `unver check ok`. `git diff --stat` shows only the 8 files. Commit `DEADSRC 2: unver.py, version branches of 8 files` (unver.py and the 8 files).
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/deadsrc_host.txt 2>&1`. Pass: `ND HOST TESTS PASS`.
4. One build (3ds/build.sh). Pass: rc 0, 0 compile errors, memcheck margin above 0. `python 3ds/tools/listcen.py > build3ds/out/deadsrc_lc.txt`.
   Pass: the 5 totals of the objective, exact. Another number: write it in the commit text and go on (the deleted lines decide, not the number).
5. Gate. For each of spot_mac01_drec and sbk14_drec: `python 3ds/tools/bench.py base <b>`, then
   `python 3ds/tools/bench.py run <b> --row DEADSRC --why "DEADSRC gate"`, then `python 3ds/tools/bench.py cmp <b> --row DEADSRC --commit $(git merge-base HEAD main)`.
   Pass: 1/1 byte-equal each (one differing bit fails).
## Fix scope
unver.py and the gate script only. Never edit one of the 8 files by hand. A compile error after Edit 1 names a deleted file: restore that file
(`git checkout HEAD~1 -- <file>`), note it in the commit, and go on. A `unver DIFF <file>` line: restore that file (`git checkout HEAD -- <file>`) and note it.
## Stop and hand-back
Stop: both gates pass. Final commit `DEADSRC 3: gate` with the values. That commit deletes this page and ws_DEADSRC_code.md.
Hand-back: build3ds/out/handback_DEADSRC.txt (line 1 commit hash, then gate values, the 5 listcen totals, memcheck margin).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
