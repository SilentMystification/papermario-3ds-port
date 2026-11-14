# ws_GCONVW: world, battle and UI lists join the converter (own-TU sources, N(x) names; enables C5, C6, C12)
Model: Haiku. PM_ROW, PM_LANE = GCONVW. Branch GCONVW, worktree build3ds/wt/GCONVW. Budget: 5 Azahar runs. Start now (main cac60bf48 has `Merge FXENT3`). Files: 3ds/tools/gen_gfx_syms.py, 3ds/tools/nd_gconv.h, 3ds/tools/gen_gfx_tab.sh, 3ds/tools/nd_host_test.sh (line 75 only), 3ds/tests/host.d/conv2c.sh (line 9 only),
new 3ds/tests/host.d/gconvw.sh, new 3ds/tests/gconvw/ (fixture), new 3ds/tools/gconv_src.d/GCONVW.txt. Edit no file under src/.
Counter: no C counter moves in this page. `ntab tables` (nd_host) rises. The GSET row after it moves C5, C6, C12. FACT nd_gfx_conv.c:475: a list with a light (gsSPSetLights1 = MOVEMEM) is unsup, so dgb_01/gfx.c (168 lists, all with lights) stays out.
## Group 1: tool (host only, commit `GCONVW 1: ...`)
1. Before any edit: `sh 3ds/tools/nd_host_test.sh > build3ds/out/host_g0.txt`, `grep '^ntab' build3ds/out/host_g0.txt > build3ds/out/ntab_g0.txt`.
2. gen_gfx_syms.py: N(x) names. Before (:10) `DEF = re.compile(r"^(Gfx|Vtx)\s+(\w+)\s*\[[^\]]*\]\s*=", re.M)`
   After `DEF = re.compile(r"^(Gfx|Vtx)\s+(?:N\((\w+)\)|(\w+))\s*\[[^\]]*\]\s*=", re.M)`. Same change for MTX (:12): `^Mtx\s+(?:N\((\w+)\)|(\w+))\s*(\[[^\]]*\])?\s*=`.
   Name = `f"{ns}_{x}"` for N(x), else the plain name. ns: copy elfgate.py:33 (NS_DEF) and elfgate.py:124-134 (file_namespace). scan() gets a parameter ns=None and passes it to its
   includes; a file with no ns uses the ns of its parent. N(x) with no ns, or a file with a non-empty `#define NAME_PREFIX` or `NAME_SUFFIX`: sys.exit(f"gen_gfx_syms: {path}: ...").
3. gen_gfx_syms.py: include path (:27-31). Try assets/us/<inc>, then src/<inc>, then path.parent/<inc> (example: kkj_03/main.c:3 `world/common/atomic/TexturePan.inc.c` is in src/).
4. gen_gfx_syms.py: own TUs. A source is own-TU when it does not start with `src/effects/`, `src/entity/model/` or `3ds/tools/gfxin/`. The shared file (argv[1]) includes the other
   sources as now. Own-TU source k gets the file `<dir of argv[1]>/gconv_tu<k>.c`: `#include "<source>"`, `#include "nd_gconv.h"`, its Img `_gend` externs, its entries. Keep the
   dedupe over all sources in argument order (a name goes to the file of its first source). Every file writes its entries as
   `static const GSym gconv_sym_<tag>[] GCONV_SEC = { ... };` (tag `in` or `tu<k>`; no array when it has no entry). Delete the `gconv_nsym` line (:80). Print each gconv_tu<k>.c path on stdout, one per line.
5. nd_gconv.h:12-13. Before `extern const GSym gconv_sym[];` `extern const unsigned gconv_nsym;` After:
   `#define GCONV_SEC __attribute__((section("gconv_sym"), used, aligned(4)))` `extern const GSym __start_gconv_sym[];` `extern const GSym __stop_gconv_sym[];`
   `#define gconv_sym __start_gconv_sym` `#define gconv_nsym ((unsigned)(__stop_gconv_sym - __start_gconv_sym))`.
6. gen_gfx_tab.sh:15-17. `TUS=$(python3 3ds/tools/gen_gfx_syms.py ...same arguments...)`. Compile gconv_in.c to "$T/gconv_sh.o" and each $TUS file to its .o, all with
   `-ffunction-sections -fdata-sections`. Then `gcc -m32 -r -nostdlib -o "$T/gconv_in.o" "$T/gconv_sh.o" <tu objects>`. Add `-Wl,--gc-sections` to the nd_gfx_conv link (:17),
   nd_host_test.sh:75 and conv2c.sh:9 (the only edit in those two files).
7. Fixture 3ds/tests/gconvw/: w.h (`#define NAMESPACE tw`), w.c (`#include "w.h"`, `#include "common.h"`, `Gfx N(a)[] = {` gsSPEndDisplayList `};`, `Mtx N(m) = {{...}};`), exp_tu0.c
   written by hand from rule 4. host.d/gconvw.sh: run gen_gfx_syms.py /tmp/gcw/gconv_in.c 3ds/tests/gconvw/w.c, cmp /tmp/gcw/gconv_tu0.c with exp_tu0.c, compile it with the
   gen_gfx_tab.sh flags. Seeded fault: a copy of gen_gfx_syms.py with `N\((\w+)\)|` removed from DEF must give a file that differs. Print `gconvw ok`.
8. Gate: nd_host PASS, `gconvw ok`, and `grep '^ntab'` equals build3ds/out/ntab_g0.txt byte for byte (`cmp`). Commit.
## Group 2: 10 world sources (1 build, 2 runs, commit `GCONVW 2: ...`)
9. GCONVW.txt, one path per line: src/world/area_kkj/kkj_03/main.c, kkj_13/main.c, kkj_14/main.c, kkj_15/main.c, kkj_18/main.c, kkj_27/main.c (all under area_kkj),
   src/world/area_kmr/kmr_21/main.c, src/world/area_isk/isk_01/haze.c, src/world/area_nok/nok_04/npc.c, src/world/area_tst/tst_13/main.c.
10. Drop rule: a source that stops gen_gfx_syms, fails its compile, gives `undefined reference` or `multiple definition` from its object (host or device link): remove its
   line, append `gconvw drop <path> <first error line>` to build3ds/out/gconvw_drop.txt, rerun. All lines dropped: stop and send the file to the oracle.
11. nd_host PASS. `ntab tables` rises (GUESS by up to 10). Every `bad` value of the ntab lines equals its value in ntab_g0.txt. Build rc 0, memcheck margin above 0 KB.
12. Gate: `bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`, `bench.py run spot_mac01_drec --row GCONVW --why "GCONVW 2"`,
   `bench.py cmp spot_mac01_drec --row GCONVW --commit $(git merge-base HEAD main)`: byte-equal. Commit.
## Group 3: battle and UI sources (1 build, 1 run each, commit `GCONVW 3a: ...`, `GCONVW 3b: ...`)
13. 3a (not drawn in mac_01): src/battle/190B20.c, src/battle/area/kzn2/actor/lava_piranha.c, src/battle/area/sam2/dlist.c, src/battle/entity_model_icons.c,
   src/battle/move/item/ coconut.c dusty_hammer.c egg_missile.c insecticide_herb.c mystery.c pebble.c sleepy_sheep.c, src/battle_ui_gfx.c, src/filemenu/filemenu_gfx.c,
   src/filemenu/filemenu_msg.c, src/pause/pause_gfx.c, src/pause/pause_partners.c, src/state_logos.c, src/titlemenu.c. Steps 10-11, run and cmp as step 12 (the base exists). Commit.
14. 3b (field): src/43F0.c, src/draw_box.c, src/entity/Shadow.c, src/global_hud_scripts.c, src/imgfx.c, src/item_entity.c, src/msg.c, src/screen_render_util.c. Same steps. A cmp difference: remove the 3b lines, commit nothing for 3b, write the cmp output to build3ds/out/gconvw_3b.txt and send the path to the oracle.
Out of this page: model.c, sprite.c, hud_element.c, msg_draw.c, screen_overlays.c, curtains.c, level_up*, starpoint*, dgb_01 (lights). The commit after 3b deletes this page.
Fix rights: the files above. A host FAIL or a cmp difference with a named cause: 2 tries, then the stop rule (CLAUDE.md 5). Hand-back file, first 8 lines: hashes, ntab tables before and after each group, gconvw_drop.txt line count, cmp results, memcheck margin.
