# ws_GSETB: bulk gset.py pass (moves C5 data and C6 data_arrays; 552 lists move, GUESS by oracle simulation on 10b0ddc9a)
Model: Haiku. PM_ROW, PM_LANE = GSETB. Branch GSETB, worktree build3ds/wt/GSETB. Budget: 2 builds (1 head, 1 base lane), 4 Azahar runs. Start now (main 10b0ddc9a).
FACT (oracle nd_host, 10b0ddc9a): `gconv gset ok 954 of 1437 lists`, `gconv lists 1437 parts 179 defs 1501 ops 14226 bad 0 moved 388`. 566 ok lists are not moved. FACT listcen: data 17808, data_arrays 1651. FACT memcheck (last main build): margin 338.
Design (oracle). Block mode takes every source outside src/effects/gfx. In the device file, code and table uses of a moved list become NDN(X) (nd_walk.c:230 and nd_gtab.c:120 find the key).
A hold line keeps the list in its file: an index, an address, sizeof, BranchList, or SetCustomGfx (nd_rec_list reads the list words: model.c:4132, nd_rec.c:520). The host compile of the
converter inputs gets -DND_GCONV_IN: NDN(X) is X there (the twin defines X). The gconv_src.d keep rule goes. N(x) lists stay (10 lists in src/world, 7 with SetCustomGfx). Add no src/world file.
Files: 3ds/tools/gset.py gen_gfx_syms.py gen_gfx_tab.sh, 3ds/include/nd_gtab.h, 3ds/tests/host.d/gset.sh, 3ds/tests/gset/case_blk/**, 3ds/tests/gset/expected_blk.txt, 3ds/tools/gset.d/GSETB.txt (new), and the files that gset.py writes (src/**, include/**, 3ds/tools/gfxin/**). Never edit by hand a file that gset.py rewrote.
## Group 1: tools and host test (0 builds, 0 runs, commit `GSETB 1: ...`)
1. gset.py:227-230. Before `if src.startswith(BLOCK_SRC):` / `moved, twin = move_blocks(...)` / `else:` / `moved, twin = move_includes(...)`. After `if src.startswith(SKIP_DIR):` / move_includes call / `else:` / move_blocks call. Delete BLOCK_SRC (:28).
2. gset.py: after :21 add `HOLD_RULE = True  # GSETB: the seeded test (gset.sh) runs a copy with this False and expects a diff`. Before move_blocks add `def hold_line(ln, hit):` that returns
   `"BranchList" in ln or "sizeof" in ln or "SetCustomGfx" in ln or any(re.search(r"\b" + re.escape(n) + r"\s*\[|&\s*" + re.escape(n) + r"\b", ln) for n in hit)`.
3. gset.py move_blocks:120-127. After `moving = {...}` add: `in_block` = every line index of every block (find_blocks ranges). When HOLD_RULE: for each line k not in in_block and not DECL.match,
   `hit` = the names of `moving` on the line (`(?<!\w)name(?!\w)`); when hit and hold_line(ln, hit): `moving -= set(hit)`. Then the fixpoint searches only the unmoved blocks:
   Before :126 `outside = "\n".join(ln for k, ln in enumerate(lines) if k not in inside)` After `unmoved = "\n".join(lines[k] for k in sorted(in_block) if k not in inside)` (:127 searches `unmoved`).
4. gset.py rewrite_users: delete `conv = gconv_rows(root)` (:175) and def gconv_rows (:156-166). :194 `and rel not in conv:` -> `:`. :199 -> `bad = KEEP_RULE and hold_line(ln, hit)`.
   Docstring: line 4 block mode is "any other device source", line 11 says "not named on a hold line or in an unmoved block of the file", delete the gconv_src.d sentence of line 15.
5. gen_gfx_syms.py:75 `SHARED = ("src/effects/", "src/entity/model/", TWIN)` -> `SHARED = ("src/effects/", "src/entity/model/")  # GSETB: a twin is shared when its device source is`.
   :135 `if s.startswith(SHARED):` -> `if (s[len(TWIN):] if s.startswith(TWIN) else s).startswith(SHARED):`.
6. nd_gtab.h:65-66: put the 2 lines in the `#else` part of `#ifdef ND_GCONV_IN /* GSETB: host compile of the converter inputs (gen_gfx_tab.sh); the gfxin twin defines the list */`.
   The `#ifdef` part: `#define NDN_DECL(sym) extern Gfx sym[]` and `#define NDN(sym) (sym)`. End with `#endif`. gen_gfx_tab.sh:17 and :19: `$DEF $INC -c` -> `$DEF -DND_GCONV_IN $INC -c` (not :21).
7. Fixture case_blk: gset_ok.txt add lines D, E, F. list.txt add `src/own.c`. Delete 3ds/tools/gconv_src.d/T.txt. New src/own.c: `#include "common.h"`, blocks `Gfx D[] = {` /
   `    gsSPEndDisplayList(),` / `};` (the same for E and F), then `void own(void) {` / `    gSPDisplayList(gMainGfxPos++, D);` / `    n = sizeof E;` / `    evt(SetCustomGfx, F);` / `}`.
8. gset.sh: :23 want rc 0 (message `want 0`). Add `src/own.c 3ds/tools/gfxin/src/own.c` to the file lists of :24 and :30. After :31 add a seeded case in the form of :27-31: a copy with
   `sed 's/^HOLD_RULE = True/HOLD_RULE = False/'`; its text must differ from expected_blk.txt, else print `gset hold seeded fault not seen` and exit 1.
9. Write expected_blk.txt from the block case output. Its diff to the old file: first lines `gset moved 2 files 3 keep 0` / `gset ok`. gconv_user.c: line 1 `#include "nd_gtab.h"`, `nd_draw_list(NDN(A), ND_SELF);`.
   own.c: `#include "nd_gtab.h"` after line 1, `NDN_DECL(D);` in place of the D block, `gSPDisplayList(gMainGfxPos++, NDN(D));`. Twin own.c: `#include "src/own.c"` and the D block. Another diff: fix the code.
10. `sh 3ds/tools/nd_host_test.sh > build3ds/out/gsetb_h1.txt 2>&1`: `ND HOST TESTS PASS`, `gset test ok`, the 2 gconv FACT lines unchanged. Commit.
## Group 2: the pass and the gates (1 build, 4 runs, commit `GSETB 2: ...`)
11. GSETB.txt, 39 lines. src/effects/gfx/<x>.c for x = snowfall big_snowflakes effect_46 ending_decals energy_shockwave explosion floating_rock gather_energy_pink huff_puff_breath light_rays
   lightning music_note quizmo_assistant quizmo_audience radial_shimmer snowflake star_outline star_spirits_energy. src/entity/model/<x>.c for x = BoardedFloor BombableRock ShatteringBrickBlock
   ShatteringHammer1Block ShatteringHammer2Block ShatteringHammer3Block WoodenCrate. Then src/battle/190B20.c src/battle_ui_gfx.c src/filemenu/filemenu_gfx.c src/filemenu/filemenu_msg.c src/pause/pause_gfx.c
   src/pause/pause_partners.c src/state_logos.c src/43F0.c src/draw_box.c src/entity/Shadow.c src/imgfx.c src/item_entity.c src/msg.c src/screen_render_util.c. Not spiky_white_aura (keep line, spiky_white_aura.c:8).
12. `python 3ds/tools/listcen.py` (data, data_arrays before). `python 3ds/tools/gset.py 3ds/tools/gset.d/GSETB.txt build3ds/out/gset_ok.txt > build3ds/out/gsetb_set.txt`: last line `gset ok`.
   Rewrite example (pause_gfx.c:617): Before `    [PAUSE_LBL_HP]        PauseGfxBannerHp,` After `    [PAUSE_LBL_HP]        NDN(PauseGfxBannerHp),`.
13. nd_host PASS, `bad 0`, no `gconv: moved` line. One build rc 0, `memcheck ... margin` above 0. For B = spot_mac01_drec, then sbk14_drec: `bench.py base B --row GSETB --commit $(git merge-base HEAD main)`,
   `bench.py run B --row GSETB --why "GSETB 2"`, `bench.py cmp B --row GSETB --commit $(git merge-base HEAD main)`: byte-equal. listcen after. Delete this page. Commit.
Fail rule (steps 12-13): a `gset keep <file>:<line>`, a `gconv: moved X` line, or a build error that names a list X: `git checkout -- src include 3ds/tools/gfxin`, `git clean -fdq -- 3ds/tools/gfxin`, remove the source of that list from GSETB.txt, do step 12 again. 2 tries per source. A failed cmp: the stop rule (CLAUDE.md 5) with the cmp output.
Stop: group 2 committed. Hand-back file, first 8 lines: commits, the `gset moved` line, the gconv lists line, the 2 cmp results, data and data_arrays before and after, memcheck margin, sources taken out.
Stay list (FACT 10b0ddc9a, not ok): effects/gfx 86 files inherit 171 unsup 18 notab 136 table-not-ok 31 | entity/model 28 files inherit 67 unsup 6 table-not-ok 31 | battle/move 7 files inherit 12 |
other 5 files inherit 9 table-not-ok 1 | world 1 file inherit 1. Out of the set: N(x) 10, hold 3 (draw_box.c:460, pause_partners.c:426, spiky_white_aura), snowfall D_09000C00_38DC70 (in the .c) 1. Copy it; add a line per source taken out.
