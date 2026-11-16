# ws_STARTLR2 (Haiku): start menu L gives N64 Z, R gives N64 R
Designer to ask: the oracle (SendMessage to oracle). Branch from main. Worktree build3ds/wt/STARTLR2, branch STARTLR2.
PM_ROW=STARTLR2 PM_LANE=STARTLR2 on every bench call. No counter (C1..C12) moves.
Cause, FACT (code read): pm_shoulder.h:7 gives L = N64 L (0x0020) and R = N64 Z (0x2000) in the start menu.
No pause code reads N64 L (grep BUTTON_L in src/pause: no hit). So L does nothing and R is Z. The user saw this (USER FACT, build 5852).
The pause menu reads Z as previous and R as next: src/pause/pause_tabs.c:406 and 417, pause_items.c:582 and 601,
pause_partners.c:700 and 704, pause_badges.c:1041 and 1062. The user words "L -> Z, R -> R" are the wanted map.
Fix: in the start menu L gives N64 Z (0x2000) and R gives N64 R (0x0010). ZL, ZR, the O3DS combo and the world map do not change.
Files: 3ds/include/pm_shoulder.h, 3ds/src/pm_input.c (comment only), 3ds/tools/pm_shoulder_test.c, 3ds/tests/host.d/startlr.sh.
Do not edit any other file.
## Edit shape (example from pm_shoulder.h:7)
Before: `    if (start_menu) return (unsigned short)((l ? 0x0020 : 0) | (r ? 0x2000 : 0) | (zl ? sh4[2] : 0) | (zr ? sh4[3] : 0));`
After:  `    if (start_menu) return (unsigned short)((l ? 0x2000 : 0) | (r ? 0x0010 : 0) | (zl ? sh4[2] : 0) | (zr ? sh4[3] : 0));`
## Edits: blocks A to D of docs/handoff/ws_STARTLR2_code.md, exact text.
## Loop (1 build, at most 2 Azahar starts)
1. Do blocks A to D. Run `sh 3ds/tools/nd_host_test.sh > build3ds/out/startlr2_host.txt 2>&1`.
   Pass: `ND HOST TESTS PASS` and `startlr ok` in the file. Commit `STARTLR2 1: start menu L gives N64 Z, R gives N64 R`.
2. Run `sh 3ds/build.sh > build3ds/out/startlr2_build.txt 2>&1`. Pass: rc 0 and memcheck margin above 0.
3. Gate (no start menu input in this bench): `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`,
   then `python 3ds/tools/bench.py run spot_mac01_drec --row STARTLR2 --why "STARTLR2 gate"`,
   then `python 3ds/tools/bench.py cmp spot_mac01_drec --row STARTLR2 --commit $(git merge-base HEAD main)`.
   Pass: 1/1 byte-equal. One differing bit fails the gate.
## Fix scope
A failing host test: compare your files with the blocks byte for byte (LF line ends). Do not change an expected text or the seed.
A tool fault (base run fails, stale build, two runs disagree): stop and hand back the exact output.
## Stop and hand-back
Stop: host PASS, build rc 0, gate 1/1 byte-equal. Final commit `STARTLR2 2: gate` with the values.
The final commit deletes ws_STARTLR2.md and ws_STARTLR2_code.md.
Hand-back: build3ds/out/handback_STARTLR2.txt. Line 1 commit hash, then the host line, build number, memcheck margin, gate value.
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
User check on the console (not an agent step): in the start menu, L goes one tab left and R goes one tab right.
