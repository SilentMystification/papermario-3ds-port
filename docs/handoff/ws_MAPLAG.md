# ws_MAPLAG (Haiku): map change lag, group 1 (b) core 1 file load, group 2 (a) fade-in hold
Designer: the oracle (SendMessage to oracle). Worktree build3ds/wt/MAPLAG, branch MAPLAG from main. PM_ROW=MAPLAG PM_LANE=MAPLAG.
No counter (C1..C12) moves. Model: Haiku (closed blocks).
Cause, FACT (code read, console log build3ds/hw_tidal1_live.txt #178704, #179775, #180731):
- The first drawn step of a new map runs the first dlc_build (nd_shape.c:383) on the walker (core 0). It loads the T-point table
  (seam_get, nd_shape.c:183) and the whole .dlc file (dlcp_open, nd_shape.c:215): seam 29.4/35.6/36.7 + dlcp 80.2/54.1/62.1 ms
  of dlc 117.3/107.9/119.7 ms. The pool fill (dlcp_load, 7.7/18.2/20.9 ms) stays on the walker: the pools are walker-owned.
- Model misses 211/189/231 (nd_recs.c:227): no cache entry at the first draw of a new map's model. pm_xfer_skip (pm_xfer.c:160) builds
  64 a covered frame. Only 1-2 frames are covered (warm 1/0, 2/58, 1/85), so 58-148 misses build in visible frames.
- pm_tex.c:1252 bumps pm_dlc_gen on the walker when the queued shape invalidation runs. A table loaded before that bump is lost.
Design:
- Group 1 (b): decode_yay0 (pm_map.c:151) drains the walker, then calls pm_dlc_preload: seam_get + dlcp_open run in the load step on
  the logic thread (core 1 on the O3DS, nd_wq.c:3; inline in det). dlc_reset keeps the tables of the current pm_dlc_gen. Thread rule:
  the drain makes the walker idle; the next walker entry is queued after the writes (nd_wq.c:126 barrier); no lock. Same files, same
  bytes: only the thread and the time change. It covers map change, enter world, file select and battle stage loads.
- Group 2 (a): pm_maplag.h PM_MAPLAG_HOLD 3: in GAME_MODE_CHANGE_MAP the fade-in stays at progress 255 for 3 more steps. Covered
  frames 4-5 (GUESS), miss budget 256-320 >= 231 (FACT #180731). ENTER_WORLD does not hold: every drec bench boots through it.
- Memory: +4 B static. Heap: the new tables (max 13536 + 47841 B, FACT sam_11 files) live with the old map's list entries
  (GUESS peak +62 KB, memcheck margin 438 KB). ARM11: cold code, once per load, no division.
Files: 3ds/src/nd_shape.c, 3ds/src/pm_map.c, 3ds/CMakeLists.txt, new: 3ds/include/pm_maplag.h, 3ds/tools/pm_maplag_test.c,
3ds/tests/host.d/dlcpre.sh, 3ds/tests/host.d/maplag.sh. Do not edit any other file. Do not edit src/.
## Edit shape (nd_shape.c:648) Before: `    seam_reset();`   After: `    if (seam_gen != pm_dlc_gen || !seam_ok) seam_reset(); /* MAPLAG: ... */`
## Edits: blocks A to G of ws_MAPLAG_code.md, exact text (scratch check: dlcpre ok, maplag ok 5/5, seeds). Loop: 1 build, 2 head runs.
1. Blocks A, B, C. Run `sh 3ds/tools/nd_host_test.sh > build3ds/out/maplag_host1.txt 2>&1`. Pass: `ND HOST TESTS PASS`, `dlcpre ok`.
   Commit `MAPLAG 1: seam table and .dlc file load in the load step (pm_dlc_preload)`.
2. Blocks D to G. Host tests again (maplag_host2.txt). Pass: `ND HOST TESTS PASS`, `dlcpre ok`, `maplag ok 5/5`.
   Commit `MAPLAG 2: map change fade-in held 3 steps at full cover (pm_maplag.h)`.
3. Run `sh 3ds/build.sh > build3ds/out/maplag_build.txt 2>&1`. Pass: rc 0, memcheck margin above 0.
4. Gate, B = `$(git merge-base HEAD main)`. For spot_mac01_drec and sbk14_drec: `python 3ds/tools/bench.py base <bench> --commit B`,
   `python 3ds/tools/bench.py run <bench> --row MAPLAG --why "MAPLAG gate, byte-equal"`,
   `python 3ds/tools/bench.py cmp <bench> --row MAPLAG --commit B`. Pass: each 1/1 byte-equal.
## Fix scope
A failing host test or seed: compare your files with the blocks byte for byte (LF). Do not change an expected text or a seed.
A compile error: check E1 and the extern lines. A link error on __real_update_enter_map_screen_overlay: check G1. A byte difference: stop, send the cmp output path to the oracle.
## Stop and hand-back
Stop: host PASS, build rc 0, both gates 1/1 byte-equal. Final commit `MAPLAG 3: gate` with the values deletes both page files.
Hand-back: build3ds/out/handback_MAPLAG.txt (line 1 commit hash, then host lines, build number, memcheck margin, 2 gate values).
Any other failure after 2 tries: write the output to build3ds/out/, send the path to the oracle (SendMessage), stop.
User check on the O3DS (xfer map lines): init drops by about seam + dlcp (old 85-117 ms), load grows by the same, warm f is 3 higher.
