# ws_IMOVE: the lists that ws_IENTRY made ok move to the gfxin twins, 11 effect and 20 entity model files (C5 -1439, C6 -124, C7 -125)
Model: Haiku. PM_ROW, PM_LANE = IMOVE. Branch IMOVE, worktree build3ds/wt/IMOVE. Budget: 1 build, 6 Azahar runs. Start after `Merge IENTRY` (not in parallel: it needs the IENTRY gset_ok).
Owns: 3ds/tools/gset.d/IMOVE.txt (new). gset.py writes the twins under 3ds/tools/gfxin, the 31 listed files and their users (src/effects/*.c, src/entity/**/*.c).
Edit no other file. Never edit by hand a file that gset.py rewrote.
FACT (listcen, main 09607a5033): C5 data 12182, C6 data_arrays 1039, C7 gfxtype 1702.
FACT (oracle dry run of gset.py with the gset_ok.txt of the IENTRY dry run): `gset moved 124 files 22 keep 0`, `gset ok`; 67 files change. listcen after: C5 10743 (-1439),
C6 915 (-124), C7 1577 (-125). C1 742, C8 777, C10 16 do not change. Converter on IENTRY dry run + this move: `bad 0 moved 1124`, no `gconv: moved` line.
## Group 1: the move (0 builds, 0 runs, commit `IMOVE 1: ...`)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/imove_h0.txt 2>&1`: `ND HOST TESTS PASS`, `ientry ok`, `gconv gset ok 1211 of 1546 lists`. `python 3ds/tools/listcen.py > build3ds/out/imove_lc0.txt`.
   Write imove_e0.txt (step 5 gives the command).
2. Write 3ds/tools/gset.d/IMOVE.txt (LF) with this one command:
   `for f in effect_63 floating_flower flower_splash_trail green_impact landing_dust misc_particles radial_shimmer shockwave snowflake sun underwater; do echo src/effects/gfx/$f.c; done > 3ds/tools/gset.d/IMOVE.txt`
   `for f in BellbellPlant_gfx BlueWarpPipe Hammer1Block Hammer2Block Hammer3Block HiddenPanel HitFloatingYellowBlock_gfx HitRedBlock_gfx HitYellowBlock_gfx InertRedBlock InertYellowBlock MunchlesiaChewing_gfx PinkFlower_gfx RedBlock Signpost StarBoxLauncher SuperBlockContent TrumpetPlant_gfx Tweester YellowBlock; do echo src/entity/model/$f.c; done >> 3ds/tools/gset.d/IMOVE.txt`
   `grep -c "" 3ds/tools/gset.d/IMOVE.txt` must print 31.
3. `python 3ds/tools/gset.py 3ds/tools/gset.d/IMOVE.txt build3ds/out/gset_ok.txt > build3ds/out/imove_set.txt 2>&1`. The file must end with `gset moved 124 files 22 keep 0` and `gset ok`.
   Rewrite example (src/entity/Signpost.c:5). Before `extern Gfx Entity_Signpost_Render[];` After `NDN_DECL(Entity_Signpost_Render);`.
   Rewrite example (src/entity/Signpost.c:28). Before `... = STANDARD_ENTITY_MODEL_SCRIPT(Entity_Signpost_Render, RENDER_MODE_SURFACE_OPA);`
   After `... = STANDARD_ENTITY_MODEL_SCRIPT(NDN(Entity_Signpost_Render), RENDER_MODE_SURFACE_OPA);`. gset adds `#include "nd_gtab.h"` as line 2 of Signpost.c.
   Effect example (src/effects/sun.c:9). Before `extern Gfx D_09000980_40C520[];` After `NDN_DECL(D_09000980_40C520);`.
4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/imove_h1.txt 2>&1`: `ND HOST TESTS PASS`, `bad 0` on the `gconv lists` line, no `gconv: moved` line. Commit.
## Group 2: the gate (1 build, 6 runs, commit `IMOVE 2: ...`)
5. One build: rc 0, `memcheck ... margin` above 0 KB. For B in spot_mac01_drec sbk14_drec fxr_drec: `python 3ds/tools/bench.py base $B --row IMOVE --commit $(git merge-base HEAD main)`,
   `python 3ds/tools/bench.py run $B --row IMOVE --why "IMOVE 2"`, `python 3ds/tools/bench.py cmp $B --row IMOVE --commit $(git merge-base HEAD main)`. Pass: byte-equal (one bit fails).
   psochk: no Azahar run. psochk runs only from an entry record (nd_walk.c:651-652). gset.py keeps every nd_draw_list entry (it writes NDN(X), and gen_ntab_entries.py:71
   reads NDN(X) as X). Only the line numbers move. In step 1 run `cut -d' ' -f1,2,4- build3ds/out/ntab_entries.txt > build3ds/out/imove_e0.txt`. After step 4 the same
   command into imove_e1.txt, then `cmp build3ds/out/imove_e0.txt build3ds/out/imove_e1.txt` prints nothing.
6. `python 3ds/tools/listcen.py > build3ds/out/imove_lc1.txt`. Delete this page. Commit.
Fix rights: step 3 output differs from the FACT, a `gset keep` line, a `gconv: moved` line or a build error: `git checkout -- src 3ds/tools/gfxin`, `git clean -fdq -- 3ds/tools/gfxin 3ds/tools/gset.d`,
send the output path to the oracle. A cmp difference: send the cmp output path to the oracle (CLAUDE.md 5). Never change a base. A tool fault stops the gate (CLAUDE.md 4).
Not in this task: the 12 other newly ok lists (src/item_entity.c 2, src/battle/entity_model_icons.c 3, src/entity/Shadow.c 2, tst_13 shockwave 1, 4 more): no gset.d form for them yet.
Stop: group 2 committed and gated. Hand-back build3ds/out/handback_IMOVE.txt, first 8 lines: commit hashes, the `gset moved` line, the 3 cmp results, C5, C6, C7 before and after
(lc0, lc1), memcheck margin, Azahar runs used.
