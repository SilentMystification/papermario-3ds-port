# ws_GSETI (Haiku): gset block mode moves asset includes; 25 sources join the converter
Objective: move the ok lists of the asset includes (`#include "x.gfx.inc.c"`) of block-mode sources to the twin. Counters (FACT, dry run on 1df64777c0,
listcen): C5 data 5149 -> 4378 (-771), C6 data_arrays 461 -> 383 (-78), C7 -1. gset moved 78. gconv lists 1763 -> 1828, bad 0, moved 1578 -> 1656; conv2c ok run `ntab tables 1789 draws 30428 bad 0`. Worktree build3ds/wt/GSETI, branch GSETI.
Files: 3ds/tools/gset.py, 3ds/tests/host.d/gset.sh, 3ds/tests/gset/case_blkinc/ (new), 3ds/tools/gconv_src.d/GSETI.txt (new), 3ds/tools/gset.d/GSETI.txt (new),
and the sources gset.py rewrites. Do not edit imgfx.c, nd_shape.c, pm_map.c, nd_dlc.c, pm_mapdata.c, heap.c, nd_mesh.h.
## Edit 1: gset.py
1. After the line `INC_HOLD = True ...` add: `INC_BLOCK = True  # GSETI: block mode moves the all-ok asset includes of a block source; seeded test in gset.sh`.
2. Replace the whole function `move_blocks` (from `def move_blocks(` to the line before `def rewrite_users(`) with block A of
   docs/handoff/ws_GSETI_code.md (exact text).
3. In `main`, before: `moved, twin = move_blocks(root, src, ok, "3ds/tools/gfxin")`. After: `moved, twin = move_blocks(root, src, ok, "3ds/tools/gfxin", assets)`.
4. OWN rule for include names. Before: `nf, nkeep = rewrite_users(root, sorted(set(all_moved)))`. After: `nf, nkeep = rewrite_users(root, sorted(set(all_moved)), assets)`.
   Before: `def rewrite_users(root, names):`. After: `def rewrite_users(root, names, assets=None):`.
   Before: `own = set(DEF.findall(text)) & set(names) if OWN_RULE else set()`. After (2 lines):
   `inc_def = {n for m in INC.finditer(text) if assets is not None and (assets / m.group(2)).is_file() for n in gfx_names((assets / m.group(2)).read_text(encoding="utf-8", errors="replace"))}`
   `own = (set(DEF.findall(text)) | inc_def) & set(names) if OWN_RULE else set()`
   Reason: src/starpoint_en.c (PAL) includes the same assets. Without this line gset rewrites its gs words (dry run FACT).
## Edit 2: sets
3ds/tools/gconv_src.d/GSETI.txt: block B of ws_GSETI_code.md (25 lines). 3ds/tools/gset.d/GSETI.txt: block C (36 lines).
Excluded (FACT, host compile or link fails): dro_02/npc_merlee.c, filemenu_selectlanguage.c, kzn2/actor/lava_piranha.c and sam2/dlist.c (NAMESPACE A(x)).
## Edit 3: host test (copy the form of the case_inc block in gset.sh)
Fixture 3ds/tests/gset/case_blkinc: src/b.c (block source) has `#include "ast/E.gfx.inc.c"` and one use `gSPDisplayList(gMainGfxPos++, E_gfx);`.
assets/us/ast/E.gfx.inc.c defines `Gfx E_gfx[] = {` .. `};`. gset_ok.txt: E_gfx. list.txt: src/b.c.
Pass: `gset moved 1`, b.c has the line `NDN_DECL(E_gfx);` and `gSPDisplayList(gMainGfxPos++, NDN(E_gfx));`, twin 3ds/tools/gfxin/src/b.c has `#include "ast/E.gfx.inc.c"`.
Seeded fault: a copy with `INC_BLOCK = False` must keep the include line in b.c. Print `gset blkinc seeded fault not seen` and exit 1 if it does not.
Example before (src/i_spy.c:28): `#include "ispy_icon.gfx.inc.c"`. After: `NDN_DECL(ispy_icon_gfx);`.
Before (src/i_spy.c:50): `gSPDisplayList(gMainGfxPos++, ispy_icon_gfx);`. After: `gSPDisplayList(gMainGfxPos++, NDN(ispy_icon_gfx));`.
## Loop (1 build, at most 8 Azahar starts; base runs not charged)
1. Edit 1 and 3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/gseti_host1.txt 2>&1`. Pass: ND HOST TESTS PASS, `gset test ok`. Commit `GSETI 1`.
2. Edit 2. Run the host test once more (gen_gfx_tab.sh copies gset_ok.txt to build3ds/out). Then `python 3ds/tools/gset.py 3ds/tools/gset.d/GSETI.txt build3ds/out/gset_ok.txt`.
   Pass: `gset moved 78 files <f> keep 0` and `gset ok` (78 is FACT of the dry run; another number: stop, send the output to the oracle).
3. Host test again (> build3ds/out/gseti_host2.txt). Pass: PASS, conv2c `ntab tables 1789 draws 30428 bad 0`, `ntab dev entries 1748 bad 0`, `ntab moved 1694` (FACT dry run).
4. One build. memcheck margin above 0 (GUESS: -10 to -40 KB, DGBJOIN moved 176 lists for -24 KB). Commit `GSETI 2` with listcen totals.
5. Gate: `bench.py base` then `bench.py run <b> --row GSETI --why "GSETI gate"` and `bench.py cmp <b> --row GSETI --commit $(git merge-base HEAD main)`
   for spot_mac01_drec, sbk14_drec, fxr_drec, fxp_drec. Pass: 1/1 byte-equal each. GUESS: no bench shows pra, iwa, kmr_20/22/23, kzn_19, mgm, hos_05, kkj_19.
   The host compare (step 3) is the proof for those lists. A cold first cmp: one warm rerun (bench.py rule).
Fix scope: gset.py, gset.sh, the fixture, the two set files. A source that fails the host compile: remove its line from both set files, note it in the commit.
Stop: all gates pass. Final commit `GSETI 3: gate` with the values. Delete this page and ws_GSETI_code.md in it. Hand-back: build3ds/out/handback_GSETI.txt.
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage).
