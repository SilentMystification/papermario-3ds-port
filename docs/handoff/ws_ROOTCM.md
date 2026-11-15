# ws_ROOTCM: codemod pass on the root state calls and the list calls of v2 tables (moves C1 build, C2 gfxpos)
Model: Haiku. PM_ROW, PM_LANE = ROOTCM. Branch ROOTCM, worktree build3ds/wt/ROOTCM. Budget: 2 builds (1 head, 1 base lane), 4 Azahar runs. Start now (main 40518d695).
Owns: 3ds/tools/codemod.py, 3ds/tests/codemod/**, and the 18 src files of steps 5 and 6. Edit no other file. Never run codemod on src/effects.
FACT (listcen, main 40518d695): C1 build 805, C2 gfxpos 288. GUESS (oracle dry run of codemod on file copies): both fall by 59 (39 state and value calls, 20 list calls).
## Group 1: tool and host test (0 builds, 0 runs, commit `ROOTCM 1: ...`)
1. New case files in 3ds/tests/codemod/cases. nt_dl.in.c, 5 lines: `void f(void) {` / `    gSPDisplayList(gMainGfxPos++, NDN(A));` / `    gSPDisplayList(gMainGfxPos++, B);` /
   `    gSPDisplayList(gMainGfxPos++, C);` / `}`. nt_dl.ntab, 2 lines: `A` / `B`. nt_dl.out.c: nt_dl.in.c with line 2 `    nd_draw_list(NDN(A), ND_SELF);` and line 3
   `    nd_draw_list(B, ND_SELF);` (line 4 stays). nt_dl.log, 1 line: `codemod keep nt_dl.c:4 gSPDisplayList no-entry`. All files LF.
2. run.sh:21. Before `fl=; case "$n" in st_*) fl=--state ;;` After `fl=; case "$n" in nt_*) fl="--ntab $CASES/$n.ntab" ;; st_*) fl=--state ;;` (rest of the line stays).
3. Seeded check: `sh 3ds/tests/codemod/run.sh > build3ds/out/rootcm_t0.txt 2>&1`. It must print `FAIL nt_dl` (the old tool keeps line 2 and line 3). Else stop and send the file to the oracle.
4. codemod.py. Before :192 `def rewrite(path, s):` add 4 lines: `def dl_key(a):` / `    a = re.sub(r'\s+', '', a)` /
   `    return a[4:-1] if a.startswith('NDN(') and a.endswith(')') else a` / an empty line (2 empty lines before `def rewrite`).
   :287 Before `        self_ok = nm == 'nd_rec_dl' and bool(args) and re.sub(r'\s+', '', args[0]) in NTAB`
   After `        self_ok = nm in DL_NAMES and len(args) == nargs and dl_key(args[-1]) in NTAB`.
   :286 comment: append `; ROOTCM: also gSPDisplayList(P, X) and gSPDisplayList(P, NDN(X))`. :27 docstring: Before `  nd_rec_dl(list) of a v2 table (--ntab)      -> nd_draw_list(list, ND_SELF)`
   After `  nd_rec_dl(list), gSPDisplayList(P, list) of a v2 table (--ntab; list X or NDN(X)) -> nd_draw_list(list, ND_SELF)`.
   Gate: `sh 3ds/tests/codemod/run.sh` prints `codemod: N/N PASS` (all cases). `sh 3ds/tools/nd_host_test.sh > build3ds/out/rootcm_h1.txt 2>&1`: `ND HOST TESTS PASS`. Commit.
## Group 2: the pass (1 build, 4 runs, commit `ROOTCM 2: ...`)
5. `python 3ds/tools/listcen.py > build3ds/out/rootcm_lc0.txt`. Then
   `python 3ds/tools/codemod.py --state src/state_title_screen.c src/entity.c src/status_star_shimmer.c src/speech_bubble.c src/pulse_stone.c src/inspect_icon.c src/i_spy.c`.
   Example (state_title_screen.c:474): Before `    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);` After `    nd_pso_cyc(G_CYC_1CYCLE);`.
   Example (entity.c:309): Before `        gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 0, alpha);` After `        nd_val_prim(0, 0, 0, 0, 0, alpha);`.
   Want (GUESS, oracle dry run): state_title_screen.c 20 calls, entity.c 6, status_star_shimmer.c 5, the other 4 files 2 each (nd_xf_push and nd_xf_pop).
6. Write build3ds/out/rootcm_nt.txt, one name per line: D_80074580 D_80293970 D_8024B6F0 filemenu_dl_copyarrow filemenu_dl_draw_char_init DefaultImgFXSetupGfx D_8014C620
   D_8014C2D8 gMsgDlistInitDrawNumber PauseGfxOrbs PauseGfxWorldMap PauseGfxPathPoints PauseGfxArrows PauseGfxSpiritsBg D_80077908 (FACT: all 15 are in build3ds/out/ntab_tables.txt of main).
   `python 3ds/tools/codemod.py --ntab build3ds/out/rootcm_nt.txt src/43F0.c src/battle/190B20.c src/filemenu/filemenu_common.c src/filemenu/filemenu_msg.c src/imgfx.c src/item_entity.c src/msg.c src/pause/pause_badges.c src/pause/pause_map.c src/pause/pause_spirits.c src/state_logos.c`.
   Example (pause_map.c:177): Before `    gSPDisplayList(gMainGfxPos++, NDN(PauseGfxWorldMap));` After `    nd_draw_list(NDN(PauseGfxWorldMap), ND_SELF);`.
   Want: 20 changed lines (43F0.c:774, 190B20.c:3012, filemenu_common.c:975 987 1009, filemenu_msg.c:705 751, imgfx.c:847, item_entity.c:1148, msg.c:2103 2300, pause_badges.c:569 695,
   pause_map.c:159 177 255, pause_spirits.c:165, state_logos.c:265 281 292). Check with `git diff --stat`. Another line changed: `git checkout -- <file>`, send the diff to the oracle.
7. Includes. After line 1 `#include "common.h"` add each missing line: `#include "nd_st.h"` when the file now calls nd_pso_* or nd_tex_on, `#include "nd_val.h"` for nd_val_*,
   `#include "nd_xf.h"` for nd_xf_*. FACT: none of the 7 files of step 5 includes one of the 3 now. The 11 files of step 6 include nd_gtab.h already.
8. `sh 3ds/tests/codemod/run.sh` PASS, nd_host PASS. One build rc 0, `memcheck ... margin` above 0 KB. For B = spot_mac01_drec, then sbk14_drec:
   `python 3ds/tools/bench.py base B --row ROOTCM --commit $(git merge-base HEAD main)`, `python 3ds/tools/bench.py run B --row ROOTCM --why "ROOTCM 2"`,
   `python 3ds/tools/bench.py cmp B --row ROOTCM --commit $(git merge-base HEAD main)`: byte-equal. `python 3ds/tools/listcen.py > build3ds/out/rootcm_lc1.txt`. Delete this page. Commit.
Fix rights: a build error in a step 5 or 6 file is a missing include (step 7): fix it. A codemod keep line is not a fault (the site stays). Any other build error, a cmp difference,
or a host FAIL: 2 tries per named cause, then the stop rule (CLAUDE.md 5) with the output. A cmp difference after step 6 only: `git checkout` the step 6 files, gate step 5 alone, commit it, send the cmp to the oracle.
Stop: group 2 committed and gated. Hand-back build3ds/out/handback_ROOTCM.txt, first 8 lines: commit hashes, C1 and C2 before and after (lc0, lc1), changed lines per step,
the 2 cmp results, memcheck margin, the keep line count of step 5.
