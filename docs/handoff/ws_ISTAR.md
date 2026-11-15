# ws_ISTAR: gset.py own rule, src/starpoint.c lists move to the gfxin twin (moves C5 data, C6 data_arrays, C7 gfxtype)
Model: Haiku. PM_ROW, PM_LANE = ISTAR. Branch ISTAR, worktree build3ds/wt/ISTAR. Budget: 1 build, 2 Azahar runs. Start now (main f509bd7f46).
Owns: 3ds/tools/gset.py, 3ds/tests/host.d/gset.sh, 3ds/tests/gset/case_blk/src/pal.c (new), 3ds/tests/gset/expected_blk.txt, 3ds/tools/gset.d/ISTAR.txt (new),
src/starpoint.c and 3ds/tools/gfxin/src/starpoint.c (gset.py writes both). Edit no other file. Never edit by hand a file that gset.py rewrote.
FACT (listcen, main f509bd7f46): C5 data 12673, C6 data_arrays 1090, C7 gfxtype 1758; group src/starpoint.c 71/12/12.
FACT (cause, code read): rewrite_users (gset.py:171-212) also rewrites src/starpoint_en.c. That file defines its own copies of the 6 lists (starpoint_en.c:10, 22, 27, 38, 50, 55).
hold_line matches `X[` there: 6 keep lines. No cmake file builds starpoint_en.c (only 3ds/battle.cmake:10 names src/starpoint.c).
FACT (oracle dry run of the edited gset.py on a copy of main f509bd7f46): `gset own src/starpoint_en.c 6`, `gset moved 7 files 1 keep 0`, `gset ok`; src/starpoint.c 71/12/12 -> 38/5/5.
Expected: C5 -33, C6 -7, C7 -7. The 5 `#if VERSION_JP` lists stay (gen_gfx_syms drops them).
## Group 1: own rule and host test (0 builds, 0 runs, commit `ISTAR 1: ...`)
1. gset.py:21. After the `HOLD_RULE = True ...` line add
   `OWN_RULE = True  # ISTAR: a user file that defines a moved name keeps its own copy (PAL twins); the seeded test (gset.sh) runs a copy with this False`.
   gset.py:15 docstring: before `line must be edited by hand.` add `A user file that defines a moved name itself keeps that name and prints `gset own <file> <n>`.`
2. gset.py:186-188. Before (3 lines): `        if not shield.search(text):` / `            continue` / `        out, changed, keep_here = [], False, 0`.
   After (11 lines): `        if not shield.search(text):` / `            continue` / `        own = set(DEF.findall(text)) & set(names) if OWN_RULE else set()` / `        sh = shield` /
   `        if own:` / `            print(f"gset own {rel} {len(own)}")` / `            rest = [n for n in names if n not in own]` / `            if not rest:` / `                continue` /
   `            sh = re.compile(r"(?<!NDN\()(?<!NDN_DECL\()(?<!\w)(" + "|".join(re.escape(n) for n in rest) + r")(?!\w)")` / `        out, changed, keep_here = [], False, 0`.
3. gset.py, in the line loop below: change `shield.` to `sh.` on 3 lines. Example :190 Before `            if not shield.search(ln):` After `            if not sh.search(ln):`.
   Same for :198 `hit = shield.findall(ln)` and :206 `out.append(shield.sub(`. Line :186 keeps `shield`.
4. New fixture 3ds/tests/gset/case_blk/src/pal.c (LF), 7 lines: `Gfx D[] = {` / `    gsSPEndDisplayList(),` / `};` / `void pal(void) {` / `    gSPDisplayList(gMainGfxPos++, D);` /
   `    nd_draw_list(A, ND_SELF);` / `}`.
5. gset.sh:24, :30, :36: Before `src/user.c src/gconv_user.c; do` After `src/user.c src/gconv_user.c src/pal.c; do`.
6. expected_blk.txt. Line 1 Before `gset moved 2 files 3 keep 0` After 2 lines `gset own src/pal.c 1` / `gset moved 2 files 4 keep 0`. At the end add 9 lines:
   `== src/pal.c` / `#include "nd_gtab.h"` / `Gfx D[] = {` / `    gsSPEndDisplayList(),` / `};` / `void pal(void) {` / `    gSPDisplayList(gMainGfxPos++, D);` / `    nd_draw_list(NDN(A), ND_SELF);` / `}`.
7. gset.sh: before :38 `echo "gset test ok"` add the seeded fault (the form of :33-37, prefix OS, sed `s/^OWN_RULE = True/OWN_RULE = False/`, the 7-file list of step 5,
   message `gset own seeded fault not seen`). FACT (dry run): without the rule it prints `gset keep src/pal.c:1` and exits 1.
8. `sh 3ds/tools/nd_host_test.sh > build3ds/out/istar_h1.txt 2>&1`: `ND HOST TESTS PASS`, `gset test ok`. Commit.
## Group 2: the move and the gate (1 build, 2 runs, commit `ISTAR 2: ...`)
9. `python 3ds/tools/listcen.py > build3ds/out/istar_lc0.txt`. New 3ds/tools/gset.d/ISTAR.txt, 1 line: `src/starpoint.c`.
   `python 3ds/tools/gset.py 3ds/tools/gset.d/ISTAR.txt build3ds/out/gset_ok.txt > build3ds/out/istar_set.txt` (step 8 writes gset_ok.txt). The file must hold the 3 lines of the FACT above.
   Rewrite example (starpoint.c:201): Before `... STANDARD_ENTITY_MODEL_SCRIPT(D_802A2158_7A9AF8, RENDER_MODE_CLOUD_NO_ZCMP);` After `... STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802A2158_7A9AF8), RENDER_MODE_CLOUD_NO_ZCMP);`.
   `git status --short` lists only src/starpoint.c, 3ds/tools/gfxin/src/starpoint.c and gset.d/ISTAR.txt.
10. `sh 3ds/tools/nd_host_test.sh > build3ds/out/istar_h2.txt 2>&1`: PASS, `lit1 ok`, `bad 0`, no `gconv: moved` line. One build: rc 0, `memcheck ... margin` above 0 KB.
11. `python 3ds/tools/bench.py base spot_mac01_drec --row ISTAR --commit $(git merge-base HEAD main)`, `python 3ds/tools/bench.py run spot_mac01_drec --row ISTAR --why "ISTAR 2"`,
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row ISTAR --commit $(git merge-base HEAD main)`: byte-equal. `python 3ds/tools/listcen.py > build3ds/out/istar_lc1.txt`. Delete this page and its INDEX.md line. Commit.
Fix rights: a host FAIL in group 1: compare build3ds/out/istar_h1.txt with steps 2-7 (2 tries). Step 9 output differs from the FACT, a `gconv: moved` line or a build error:
`git checkout -- src include 3ds/tools/gfxin`, `git clean -fdq -- 3ds/tools/gfxin`, send the output path to the oracle (CLAUDE.md 5). A cmp difference: 2 tries per named cause, then the stop rule.
Stop: group 2 committed and gated. Hand-back build3ds/out/handback_ISTAR.txt, first 8 lines: commit hashes, the `gset own` and `gset moved` lines, the cmp result,
C5, C6, C7 before and after (lc0, lc1), memcheck margin, Azahar runs used.
