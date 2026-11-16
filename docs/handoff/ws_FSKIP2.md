# ws_FSKIP2 (Haiku): the 3 modes Off, On, Dynamic go on the Options row "Frame skip"; the row "60 FPS" goes away
Designer to ask: the bug oracle (oracle 39, SendMessage; the orchestrator gives the id). Worktree build3ds/wt/FSKIP2, branch FSKIP2.
PM_ROW=FSKIP2 PM_LANE=FSKIP2 on every build and bench call. Edit only the files of ws_FSKIP2_code.md. No counter (C1..C12) moves.
Do not edit tools/gset.py, gset.d, 3ds/tools/gfxin*, 3ds/build.sh (GSETR and STALEOBJ own them).
## Design (decided by code read, main a0115f743c; do not change it)
- One setting: pm_opt.fps60 (0 Off, 1 On, 2 Dynamic). options.ini key stays `fps60=0|1|2` (old files load, pm_options.c:209).
- The General tab shows 2 rows: "Frame skip" (RW_FPS, value Off/On/Dynamic) and "Resolution Mode". The old boolean row (RW_DBG + 7,
  pm_opt.fskip) and the "60 FPS" label go away. RW_FPS stays the first row, so pad_menu_part2.txt:5 (`SELECT, ddown, A`) still hits Resolution.
- FS1 (pm_main.c:689, drops the draws of a late step) runs in Off and Dynamic, never in On. Reason: the user says only On "runs slower";
  Off keeps the default of today (fps60=0 with fskip=1, pm_options.c:16-18), so Off on the console does not change.
- The key frame skip (LGF_ISKIP, pm_main.c:720) does not change: On never skips, Dynamic skips under load or when late.
- options.ini key `fskip`: read and ignored (no warning line), not written. The launch switch `fps60` with fps60=0 acts as Dynamic (as today).
  The switch `fs1off` turns FS1 off in every mode. The switch `fs1force=<n>` forces FS1 on in every mode (BTRANS bench, azt_scenarios.json:10805).
- Byte-equal: det turns FS1 and ISKIP off (pm_main.c:689, :720), so a det bench gives the same bytes in every mode.
## Loop (1 build, 2 Azahar runs)
1. Make all edits of ws_FSKIP2_code.md (5 files and the host test pair). Run `sh 3ds/tools/nd_host_test.sh > build3ds/out/fskip2_host.txt 2>&1`.
   Pass: the line `fskip ok 14/14` and the last line `ND HOST TESTS PASS`. The 5 seeds of 3ds/tests/host.d/fskip.sh run inside it.
2. `PM_LANE=FSKIP2 PM_ROW=FSKIP2 sh 3ds/build.sh > build3ds/out/fskip2_b1.txt 2>&1`. Pass: rc 0, 0 compile errors.
3. Commit (changed files only): `FSKIP2 1: Frame skip row holds the 3 modes (fps60 0/1/2); the 60 FPS row and the fskip boolean go; FS1 runs in Off and Dynamic`.
4. Byte gate (Off): `python 3ds/tools/bench.py base spot_mac01_det`, then
   `python 3ds/tools/bench.py run spot_mac01_det --row FSKIP2 --why "FSKIP2 Off byte gate"`, then
   `python 3ds/tools/bench.py cmp spot_mac01_det --row FSKIP2 --commit $(git merge-base HEAD main)`. Pass: every shot byte-equal (1 differing bit fails).
## Fix scope
- A failing host test: fix only your edit of the named file. Never change a seed or an expected line of the test.
- A compile error: fix only the edited line. A tool fault in step 4: stop and send the output path to the oracle.
- A menu-shot bench (shotmenu: pad_menu_part2, pad_menu_part3) shows 1 row less on the General tab. This is expected. Do not run them.
## Stop and hand-back
Stop: steps 1-4 pass. Final commit `FSKIP2 2: gate` with the cmp result. It deletes ws_FSKIP2.md, ws_FSKIP2_code.md and their INDEX.md line.
Hand-back: build3ds/out/handback_FSKIP2.txt. Line 1 commit hash. Line 2 `host fskip ok 14/14, ND HOST TESTS PASS`. Line 3 build rc and errors.
Line 4 the cmp line. Line 5 `counters: none`. Line 6 the files changed. Last line: `FSKIP2 done`.
Two tries per named cause, then write the output to build3ds/out/ and SendMessage the path to the oracle.
