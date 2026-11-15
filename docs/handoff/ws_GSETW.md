# ws_GSETW: gset.py moves `Gfx N(x)[]` blocks (namespace form); the 12 battle item lists move (C5 -228, C6 -12, C7 -12)
Model: Haiku. PM_ROW, PM_LANE = GSETW. Branch GSETW, worktree build3ds/wt/GSETW. Budget: 1 build, 6 Azahar runs. Start now. Parallel with ws_PMOVE (disjoint files).
Owns: 3ds/tools/gset.py, 3ds/include/nd_gtab.h (lines 69-70 only), 3ds/tests/host.d/gset.sh, 3ds/tests/gset/case_ns/ (new), 3ds/tools/gset.d/GSETW.txt (new),
the 7 files src/battle/move/item/{coconut,dusty_hammer,egg_missile,insecticide_herb,mystery,pebble,sleepy_sheep}.c and their gfxin twins. Edit no other file.
FACT (oracle nd_host on main 2a9ae46068): the 12 lists battle_item_* are in gset_ok.txt (`gconv gset ok 1211 of 1546`). gset.py:25 BLOCK_OPEN misses `Gfx N(x)[] = {`.
FACT (listcen): the 7 files hold C5 data 228, C6 data_arrays 12, C7 gfxtype 12. N(x) expands to NAMESPACE_x (include/macros.h:45, gen_gfx_syms.py:8).
## Group 1: the tool (0 builds, 0 runs, commit `GSETW 1: ...`)
1. nd_gtab.h:69-70. Before `#define NDN_DECL(sym) extern const NdNDev ndd_##sym` and `#define NDN(sym) ((Gfx*)&ndd_##sym) /* GSET: ... */`.
   After (4 lines): `#define NDN_DECL(sym) NDN_DECL_(sym)`, `#define NDN_DECL_(sym) extern const NdNDev ndd_##sym`, `#define NDN(sym) NDN_(sym) /* GSETW: N(x) expands first */`,
   `#define NDN_(sym) ((Gfx*)&ndd_##sym) /* GSET: the key of a moved list (nd_draw_list, handles) */`. Lines 66-67 (ND_GCONV_IN) stay.
2. gset.py. Add after line 22: `NS_RULE = True  # GSETW: N(x) is rewritten only in the files of its namespace; the seeded test (gset.sh) runs a copy with this False`.
   Replace line 25 with `BLOCK_OPEN = re.compile(r"^Gfx (?:N\((\w+)\)|(\w+))\[\] = \{")`. Add `NS_DEF = re.compile(r"^\s*#\s*define\s+NAMESPACE\s+(\w+)", re.M)`,
   `NDECL = re.compile(r"^(\s*)extern\s+Gfx\s+(N\(\w+\))\s*\[\s*\]\s*;")` and `NSN = {}  # GSETW: moved N(x) name -> (token, namespace, directory)`.
3. Add `file_ns(p)`: return the group of NS_DEF in p; else, for each `#include "<h>.h"` of p with p.parent/<h> a file, the NS_DEF of that header; else, for each q in
   sorted(p.parent.glob("*.c")) (q != p) whose text has the line `#include "<p.name>"`, return file_ns(q); else None. Read with encoding utf-8, errors replace.
4. find_blocks(lines, ns=None, tok=None): on a match, group 2 appends (name, i, j) as now. Group 1 with ns appends (ns + "_" + x, i, j) and sets tok[ns + "_" + x] = "N(" + x + ")".
   Group 1 without ns appends nothing. In every match with an end line, i = j + 1.
5. move_blocks: `ns = file_ns(p)`, `tok = {}`, `blocks = find_blocks(lines, ns, tok)`, `T = lambda n: tok.get(n, n)`. In the hold loop and in the fixpoint `bad` set, search
   `r"(?<!\w)" + re.escape(T(n)) + r"(?!\w)"` and call `hold_line(ln, {T(n) for n in hit})`. The hold loop skips NDECL lines as it skips DECL lines. The device line is `f"NDN_DECL({T(n)});"`. Before the return: `for n in moved:` `if n in tok:`
   `NSN[n] = (tok[n], ns, p.parent)`. hold_line: write `(?<!\w)` in place of the first `\b` and `(?!\w)` in place of the last `\b` (a token can end in `)`).
6. Add `rewrite_ns(root)`: group NSN by (directory, ns). For each f in sorted(d.glob("*.c")) + sorted(d.glob("*.h")): skip f when NS_RULE and file_ns(f) != ns. Per line: NDECL match
   with group 2 in the tokens -> `f"{indent}NDN_DECL({tok});"`. Else the shield `r"(?<!NDN\()(?<!NDN_DECL\()(?<!\w)(" + alternation of re.escape(tokens) + r")(?!\w)"`: a hold_line
   line prints `gset keep <rel>:<line>` (as rewrite_users), any other hit line gets `NDN(\1)`. A changed file without "nd_gtab.h" gets add_gtab. Return (files changed, keeps).
   main: after rewrite_users, `nf2, nk2 = rewrite_ns(root)`, add both to the totals before the `gset moved` print.
7. Fixture 3ds/tests/gset/case_ns (LF): list.txt `src/battle/item/a.c`, `src/world/m1/gfx.c`. gset_ok.txt `ns_a_dl`, `m1_g1`, `m1_g2`. a.c: `#include "common.h"`, `#define NAMESPACE ns_a`,
   block `Gfx N(dl)[] = {` / `    gsSPEndDisplayList(),` / `};`, then `EntityModelScript N(ems) = STANDARD_ENTITY_MODEL_SCRIPT(N(dl), 1);`. b.c: the same text with ns_b (not in the list).
   m1/m1.h: `#include "common.h"`, `#define NAMESPACE m1`, `extern Gfx N(g2)[];`. m1/gfx.c: `#include "m1.h"`, block N(g1) with `    gsSPDisplayList(N(g2)),` and the END line,
   block N(g2), `#include "skel.inc.c"`. m1/skel.inc.c: `StaticAnimatorNode N(node) = {` / `    .displayList = N(g1),` / `};`.
8. gset.sh, before `echo "gset test ok"`: run gset.py on a copy of case_ns. Check with grep -x (CR dropped): output `gset moved 3 files 3 keep 0` and `gset ok`; a.c has `NDN_DECL(N(dl));` and
   `EntityModelScript N(ems) = STANDARD_ENTITY_MODEL_SCRIPT(NDN(N(dl)), 1);`; m1.h has `NDN_DECL(N(g2));`; skel.inc.c has `    .displayList = NDN(N(g1)),`; gfx.c has `NDN_DECL(N(g1));`;
   the twin 3ds/tools/gfxin/src/world/m1/gfx.c has `Gfx N(g2)[] = {`; `cmp` b.c with the fixture b.c. Seeded fault: a copy with `NS_RULE = False` must change b.c (cmp differs).
9. `sh 3ds/tools/nd_host_test.sh > build3ds/out/gsetw_h1.txt 2>&1`: `ND HOST TESTS PASS`, `gset test ok`. Commit.
## Group 2: the move (0 builds, 0 runs, commit `GSETW 2: ...`)
10. Before the move: `python 3ds/tools/listcen.py > build3ds/out/gsetw_lc0.txt`. Write gset.d/GSETW.txt (LF), one line per file: src/battle/move/item/<f>.c for the 7 names above.
11. `python 3ds/tools/gset.py 3ds/tools/gset.d/GSETW.txt build3ds/out/gset_ok.txt > build3ds/out/gsetw_set.txt 2>&1`: last lines `gset moved 12 files 7 keep 0`, `gset ok` (GUESS: 7 files).
    Example (coconut.c:27). Before `Gfx N(displayList)[] = {` .. `};` After `NDN_DECL(N(displayList));` (the block goes to the twin).
    Example (coconut.c:49). Before `... = STANDARD_ENTITY_MODEL_SCRIPT(N(displayList), RENDER_MODE_ALPHATEST);` After `... = STANDARD_ENTITY_MODEL_SCRIPT(NDN(N(displayList)), RENDER_MODE_ALPHATEST);`.
    Example (egg_missile.c:142). Before `ems_Draw(N(Frame1Gfx), 2)` After `ems_Draw(NDN(N(Frame1Gfx)), 2)`.
12. nd_host_test.sh into gsetw_h2.txt: `ND HOST TESTS PASS`, `bad 0` and `moved 1136` on the `gconv lists` line, no `gconv: moved` line. Commit.
## Group 3: the gate (1 build, 6 runs, commit `GSETW 3: ...`)
13. One build: rc 0, memcheck margin above 0 KB (FACT main: 438 KB). For B in spot_mac01_drec sbk14_drec fxr_drec: `python 3ds/tools/bench.py base $B --row GSETW --commit $(git merge-base HEAD main)`,
    `python 3ds/tools/bench.py run $B --row GSETW --why "GSETW 3"`, `python 3ds/tools/bench.py cmp $B --row GSETW --commit $(git merge-base HEAD main)`. Pass: byte-equal (one bit fails).
    psochk: no run (gen_ntab_entries.py scans src/effects only; no entry changes). The moved lists draw on the entity path (nd_rec_ent_draw), as the IMOVE lists (FACT sbk14_drec passed).
14. `python 3ds/tools/listcen.py > build3ds/out/gsetw_lc1.txt`: C5 -228, C6 -12, C7 -12 against lc0 (FACT by file count). Delete this page. Commit.
Fix rights: a gset.sh mismatch in your own fixture text: fix the fixture or the test, not the rule. Step 11 not as given, a keep line, a `gconv: moved` line or a build error:
`git checkout -- src 3ds/tools/gfxin`, `git clean -fdq -- 3ds/tools/gfxin`, send the output path to the oracle. A cmp difference: send the cmp output path to the oracle (CLAUDE.md 5). Never change a base.
Not in this task: dgb_01 (FACT: its 176 lists get 127 tables, 49 `vertex slots` unsup, so gset ok 0 by the call edges), kmr_21, nok_04, isk_01, kkj (SetCustomGfx holds).
Stop: group 3 committed and gated. Hand-back build3ds/out/handback_GSETW.txt, first 8 lines: commit hashes, the `gset moved` line, the 3 cmp results, C5, C6, C7 (lc0, lc1),
memcheck margin, Azahar runs used.
