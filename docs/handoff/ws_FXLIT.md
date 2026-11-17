# ws_FXLIT: lit draws and PRIMDEPTH, SCISSOR, FILLCOLOR, RECT become units only; 46 lists move (FXSEED gap a, part of gap d, 6 refused src lists)
Model: Haiku (design closed, oracle dry run done; the user allowed Sonnet, not needed). PM_ROW, PM_LANE = FXLIT. Branch FXLIT, worktree build3ds/wt/FXLIT.
Start: now. Budget: 1 build, 8 Azahar starts. Owns: 3ds/tools/nd_gfx_conv.c (v2_walk), 3ds/tests/host.d/fxlit.sh, src/effects/gfx/{quizmo_stage,underwater,star_spirits_energy}.c,
src/effects/{quizmo_stage,underwater,star_spirits_energy}.c, src/screen_overlays.c, src/curtains.c, src/world/area_{kmr/kmr_20/records,mgm/mgm_00/scoreboard,hos/hos_05/intro}.c,
their twins under 3ds/tools/gfxin (gset.py writes them), the 4 FXLIT files in azt.d, bench.d, gset.d. Edit no other file. Never edit by hand a file that gset.py wrote.
Design (oracle, code read). A table runs its units on the walker: nd_ntab_run = nd_nr_exec = nd_walk_state (nd_walk.c:384, :600). A lit vertex load is op_geo (nd_walk.c:282),
the same handler as a list walk. So a lit draw and the units PRIMZ, SCISSOR, FILL, RECT need no v2 op. RARE and MISC stay refused.
Counters. FACT (oracle dry run on main 023db9ca81, listcen): build 386 -> 374, gfxpos 89 -> 77, data 1207 -> 793, data_arrays 92 -> 46, gfxtype 580 -> 563.
recdl 11, reader 789, model 15 do not change. FACT (host): gconv ntab tables 1934 -> 1950, unsup 18 -> 5, gset ok 1858 -> 1904, ntab moved 1891 -> 1937.
GUESS: elfgate data and gfxnames fall (46 moved lists). Main moves while you work: the deciding checks are the lines named below, not these totals.
## Group 1: converter (0 builds, 0 runs, commit `FXLIT 1: ...`)
1. `git apply docs/handoff/ws_FXLIT.patch` (nd_gfx_conv.c and the new host.d/fxlit.sh). Example of the edit, nd_gfx_conv.c:509.
   Before `        case NDO_FILL: case NDO_PRIMZ: case NDO_SCISSOR: case NDO_RARE: case NDO_RECT: case NDO_MISC:` After
   `        case NDO_FILL: case NDO_PRIMZ: case NDO_SCISSOR: case NDO_RECT: /* FXLIT: a unit only ... */` / `            break;` / `        case NDO_RARE: case NDO_MISC:`.
   The patch also deletes nd_gfx_conv.c:554 `if (cx->lit && (s->geom & G_LIGHTING)) { v2_unsup("lit draw after a light unit", k); break; }` and the field `lit`.
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/fxlit_h1.txt 2>&1`. Pass: `ND HOST TESTS PASS`, `fxlit ok`, `lit1 ok`, `ntab tables ... bad 0`, no `gconv: moved` line.
   Seed fact (oracle): on main the first name of fxlit.sh (D_09006C98_39F858) is not in gset_ok.txt, so fxlit.sh fails on the old converter.
   Write the `gconv ntab tables` and `gconv gset ok` lines to the commit text. Commit.
## Group 2: call sites, moves, gate (1 build, 2 Azahar runs, commit `FXLIT 2: ...`)
3. `python 3ds/tools/listcen.py > build3ds/out/fxlit_lc0.txt`. `git apply docs/handoff/ws_FXLIT2.patch` (12 call sites, 2 gset.d files, 3 new benches).
   Example, src/screen_overlays.c:295. Before `            gSPDisplayList(gMainGfxPos++, D_8014E8F0);` After `            nd_draw_list(D_8014E8F0, ND_SELF);`.
   The world files also get `#include "nd_gtab.h"` (example src/world/area_kmr/kmr_20/records.c:2).
4. `python 3ds/tools/gset.py 3ds/tools/gset.d/FXLIT.txt build3ds/out/gset_ok.txt > build3ds/out/fxlit_set2.txt` (FACT dry: `gset moved 40 files 3 keep 0`, `gset ok`).
   `python 3ds/tools/gset.py 3ds/tools/gset.d/FXLITW.txt build3ds/out/gset_ok.txt > build3ds/out/fxlit_set3.txt` (FACT dry: `gset moved 6 files 5 keep 0`, `gset ok`).
   Example, src/effects/quizmo_stage.c:146. Before `nd_draw_list(D_09006E28_39F9E8, pe_quizmo_stage_137);` After `nd_draw_list(NDN(D_09006E28_39F9E8), pe_quizmo_stage_137);`.
5. `B=gold_72_drec,gold_87_drec,gold_123_drec,spot_mac01_drec,curt_title_drec`. `python 3ds/tools/bench.py base $B --row FXLIT --commit $(git merge-base HEAD main)`.
   `sh 3ds/tools/rowloop.sh --row FXLIT --bench $B --why "FXLIT 2"`. Pass: `LOOP PASS`, 5 times `byte-equal`, host `ND HOST TESTS PASS` with `fxlit ok`.
   Until lines verified against code: `warp: mac_01 entry 0` (pm_warp.c:839), `drec f413 draws N bytes M` (pm_gpu.c:618). fxtest ids: 72 quizmo_stage, 87 underwater,
   123 star_spirits_energy (pm_fxtest.c:105, :120, :156, index = line - 33).
6. `python 3ds/tools/listcen.py > build3ds/out/fxlit_lc1.txt`. `python 3ds/tools/elfgate.py > build3ds/out/fxlit_elf.txt`. Delete this page and both patches. Commit.
## Fix rights and stop
- A `gset keep` line, a `gconv: moved` line or a build error that names a list: `git checkout -- src 3ds/tools/gfxin`, `git clean -fdq -- 3ds/tools/gfxin`,
  remove that source from its gset.d file, run step 4 again (2 tries per source). Name the removed source in the commit.
- A host FAIL or a cmp DIFF: 2 tries per named cause, then write the output to build3ds/out/ and send the path to the oracle (CLAUDE.md rule 5).
- OPEN: quizmo_stage draws its walls only when data->unk_3C != 255 (src/effects/quizmo_stage.c:139). unk_3C is not set (include/effects.h:1357). The host test is the proof.
Stop: group 2 committed and gated. Hand-back (SubagentHandback): commit hashes, the gconv and gset lines, 5 cmp results, listcen lc0 and lc1 totals, elfgate line, sources taken out.
