# ws_HOLD1 (Haiku): 3 hold lines lose their `&`; gset moves 4 ok lists of 4 sources (C3 data, C4 data_arrays, C5 gfxtype)
Designer to ask: oracle DL3 (SendMessage; the orchestrator gives the id). Worktree build3ds/wt/HOLD1, branch HOLD1. PM_ROW=HOLD1 PM_LANE=HOLD1.
Start now. Parallel with GSETR, CGFX, DLSITE1, MODELRM, GBIHDR, PRA31 (no shared file).
Objective (FACT, oracle gset dry run on a copy of main a0115f743c with the 3 edits below): `gset moved 4 files 4 keep 0`, `gset ok`;
listcen data -61, data_arrays -4, gfxtype -2. build and gfxpos do not change. The dry run changed only the 4 sources and wrote 4 new twins.
Lists (FACT: all 4 are in build3ds/out/gset_ok.txt on main): inspect_icon_gfx, speech_bubble_gfx, gPausePartnersDL, kmr_21_Gfx_TexSetup_TitleImage.
Cause (FACT, gset.py:160): a line with `&X` is a hold line, so the list stays. `&X` and `X` give the same address for an array.
kmr_21/main.c has no hold line: it only needs a gset.d entry (ws_GSETR kept it out while ws_DEADSRC ran; DEADSRC is merged, e91cff3feb).
Files: src/inspect_icon.c, src/speech_bubble.c, src/pause/pause_partners.c, src/world/area_kmr/kmr_21/main.c, new 3ds/tools/gset.d/HOLD1.txt,
and the 4 twins gset.py writes under 3ds/tools/gfxin/src/. Do not edit any other file. Do not edit gset.py.
## Edit 1 (by hand, 3 lines)
Before (src/inspect_icon.c:75): `gSPDisplayList(gMainGfxPos++, &inspect_icon_gfx);`. After: `gSPDisplayList(gMainGfxPos++, inspect_icon_gfx);`.
Same shape: src/speech_bubble.c:62 `&speech_bubble_gfx` -> `speech_bubble_gfx`; src/pause/pause_partners.c:426 `&gPausePartnersDL` -> `gPausePartnersDL`.
## Edit 2: new 3ds/tools/gset.d/HOLD1.txt (4 lines, LF)
`src/inspect_icon.c` `src/speech_bubble.c` `src/pause/pause_partners.c` `src/world/area_kmr/kmr_21/main.c` (one path per line).
## The result of gset (dry run FACT, for the check only)
src/inspect_icon.c: `#include "inspect_icon.gfx.inc.c"` becomes `NDN_DECL(N(gfx));` (NAMESPACE inspect_icon, inspect_icon.c:5), line 75 becomes
`gSPDisplayList(gMainGfxPos++, NDN(inspect_icon_gfx));`. pause_partners.c:188-203 (the block) becomes `NDN_DECL(gPausePartnersDL);`.
## Loop (1 build, at most 4 Azahar starts; base runs not charged)
1. Edit 1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/hold1_host0.txt 2>&1`. Pass: `ND HOST TESTS PASS`. Commit `HOLD1 1: 3 hold lines lose the &`.
2. Edit 2. `python 3ds/tools/listcen.py > build3ds/out/hold1_lc0.txt`. Then
   `python 3ds/tools/gset.py 3ds/tools/gset.d/HOLD1.txt build3ds/out/gset_ok.txt > build3ds/out/hold1_gset.txt 2>&1`.
   Pass: `gset moved 4 files 4 keep 0` and `gset ok`. Another number: stop and send the path to oracle DL3.
3. Host test again (> build3ds/out/hold1_host1.txt). Pass: `ND HOST TESTS PASS`, `lkey census ok`, `gconv gset ok` with no `not moved` line,
   conv2c line `bad 0`. Commit `HOLD1 2: gset moves 4 lists` (the set file, the 4 sources, the 4 twins).
4. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0. `python 3ds/tools/listcen.py > build3ds/out/hold1_lc1.txt`.
   Pass vs hold1_lc0.txt: data -61, data_arrays -4, gfxtype -2, the other classes the same.
5. Gate. For spot_mac01_drec and sbk14_drec: `bench.py base <b>`, `bench.py run <b> --row HOLD1 --why "HOLD1 gate"`,
   `bench.py cmp <b> --row HOLD1 --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
   NOT proven by bench (GUESS: no bench draws them): the inspect icon, the speech bubble, the pause partners tab, the kmr_21 title image.
   Their proof: the host compare of step 3 (conv2c bad 0, gconv gset ok).
## Fix scope
Edits 1-2 only. A compile error or a host fault names one source: remove its line from HOLD1.txt, `git checkout HEAD -- src 3ds/tools/gfxin`,
run step 2 again, and write the new moved count in the commit.
## Stop and hand-back
Stop: both gates pass. Final commit `HOLD1 3: gate` with the values deletes this page. Hand-back: build3ds/out/handback_HOLD1.txt
(line 1 commit hash, then gate values, gset moved, listcen data data_arrays gfxtype, memcheck).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to oracle DL3 (SendMessage). Then stop.
