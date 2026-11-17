# ws_FXLIT: PRIMDEPTH becomes a unit only; 6 src lists move (screen_overlays D_8014E8F0, D_8014E9A8; curtains, kmr_20, mgm_00, hos_05)
Model: Haiku (design closed, oracle dry run done). PM_ROW, PM_LANE = FXLIT. Branch FXLIT, worktree build3ds/wt/FXLIT. Budget: 1 build, 8 Azahar starts.
Start: after `Merge FXOPS` (same file 3ds/tools/nd_gfx_conv.c; FXOPS makes FILL, SCISSOR and RECT units only). FACT: both patches apply on main 67ec996aed
plus ws_FXOPS.patch, before and after ws_FXLT.patch (git apply --check).
Owns: 3ds/tools/nd_gfx_conv.c (the NDO_PRIMZ case only), 3ds/tests/host.d/fxlit.sh, 3ds/tools/gset.d/FXLIT.txt, src/screen_overlays.c, src/curtains.c,
src/world/area_kmr/kmr_20/records.c, src/world/area_mgm/mgm_00/scoreboard.c, src/world/area_hos/hos_05/intro.c, their twins under 3ds/tools/gfxin (gset.py writes them).
Edit no other file. Never edit by hand a file that gset.py wrote.
Design (oracle, code read). A table runs its units on the walker: nd_ntab_run = nd_nr_exec = nd_walk_state (nd_walk.c:384, :600). The PRIMZ unit runs
set_primdepth there (nd_set.c:64, :170). The depth source bit is other mode L, which the desc holds. So PRIMZ needs no v2 op, as FXOPS did for FILL, SCISSOR, RECT.
Counters. FACT (oracle dry run, main 023db9ca81 with the same unit-only rule, listcen before and after the 5 files): build -12, gfxpos -12, data -91,
data_arrays -6, gfxtype -5. recdl, reader, model do not change. FACT dry: `gset moved 6 files 5 keep 0`, `gset ok`, `ND HOST TESTS PASS`, `fxlit ok`.
GUESS: elfgate data falls (6 ndg_ tables), gfxnames falls by 6 or fewer.
## Group 1: converter (0 builds, 0 runs, commit `FXLIT 1: ...`)
1. `git apply docs/handoff/ws_FXLIT.patch` (nd_gfx_conv.c and the new host.d/fxlit.sh). The edit, nd_gfx_conv.c (after FXOPS, the line after the FXOPS unit-only case).
   Before `        case NDO_PRIMZ: case NDO_MISC:` After `        case NDO_PRIMZ: /* FXLIT: a unit only, ... */` / `            break;` / `        case NDO_MISC:`.
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/fxlit_h1.txt 2>&1`. Pass: `ND HOST TESTS PASS`, `fxlit ok`, `ntab tables ... bad 0`, no `gconv: moved` line.
   Seed (oracle): without the edit D_8014E8F0 is refused (`PRIMDEPTH (unit 4 of D_8014E8F0)` in ntab_unsup.txt), so fxlit.sh fails on its first name.
   Write the `gconv ntab tables` and `gconv gset ok` lines to the commit text. Commit.
## Group 2: call sites, move, gate (1 build, 2 Azahar runs, commit `FXLIT 2: ...`)
3. `python 3ds/tools/listcen.py > build3ds/out/fxlit_lc0.txt`. `git apply docs/handoff/ws_FXLIT2.patch` (12 call sites in 5 files, gset.d/FXLIT.txt).
   Example, src/screen_overlays.c:295. Before `            gSPDisplayList(gMainGfxPos++, D_8014E8F0);` After `            nd_draw_list(D_8014E8F0, ND_SELF);`.
   The 3 world files also get `#include "nd_gtab.h"` (example src/world/area_kmr/kmr_20/records.c:2).
4. `python 3ds/tools/gset.py 3ds/tools/gset.d/FXLIT.txt build3ds/out/gset_ok.txt > build3ds/out/fxlit_set.txt`. Pass: `gset moved 6 files 5 keep 0`, `gset ok`.
   Example, src/world/area_hos/hos_05/intro.c (main :1366). Before `nd_draw_list(N(gfx_setup_story_viewport), ND_SELF);` After `nd_draw_list(NDN(N(gfx_setup_story_viewport)), ND_SELF);`.
   The list block becomes `NDN_DECL(N(gfx_setup_story_viewport));` and moves to 3ds/tools/gfxin/src/world/area_hos/hos_05/intro.c.
5. `B=spot_mac01_drec,curt_title_drec`. `python 3ds/tools/bench.py base $B --row FXLIT --commit $(git merge-base HEAD main)`.
   `sh 3ds/tools/rowloop.sh --row FXLIT --bench $B --why "FXLIT 2"`. Pass: `LOOP PASS`, 2 times `byte-equal`, `ND HOST TESTS PASS` with `fxlit ok`.
   OPEN: no bench draws the 6 lists (GUESS: curt_title_drec draws NoControllerGfx, ws_FXOPS.md:34). The proof is the host unit-stream compare (nd_gtab_test.c v2_units).
6. `python 3ds/tools/listcen.py > build3ds/out/fxlit_lc1.txt`. `python 3ds/tools/elfgate.py > build3ds/out/fxlit_elf.txt`. Delete this page and both patches. Commit.
## Fix rights and stop
- A `gset keep` line, a `gconv: moved` line or a build error that names a list: `git checkout -- src 3ds/tools/gfxin`, `git clean -fdq -- 3ds/tools/gfxin`,
  remove that source from gset.d/FXLIT.txt, revert its call-site edit, run step 4 again (2 tries per source). Name the removed source in the commit.
- A host FAIL or a cmp DIFF: 2 tries per named cause, then write the output to build3ds/out/ and send the path to the oracle (CLAUDE.md rule 5).
Stop: group 2 committed and gated. Hand-back (SubagentHandback): commit hashes, the gconv and gset lines, 2 cmp results, listcen lc0 and lc1 totals, elfgate line, sources taken out.
