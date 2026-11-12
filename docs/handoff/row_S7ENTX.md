# Row S7ENTX: entity list sources the S7ENT set left out (SaveBlock, Shadow) into the tables; SaveBlock call sites

(Model: Haiku. A 20-line scan filter in one Python function, two names on one line, two call sites. Every edit is below.)

## Cause (ORACLE7 code read, main 87a256ef5 and branch S7ENT 3aa495373)
- gen_gfx_syms.py scan() collects `Gfx`/`Vtx` definitions and INCLUDE_IMG names with regexes over the whole text, so it also takes
  the `#if VERSION_PAL` ones of src/entity/model/SaveBlock.c (:37-39 D_PAL_0A002170_EBA460, :53-65 LoadTextureLetterG, :101-128
  RenderLetterG, :195-201 RenderBlock_es). The device build is VERSION_US: those symbols do not exist, gconv_in.c fails, so S7ENT
  left SaveBlock.c out.
- src/entity/Shadow.c holds the entity shadow lists (Entity_RenderSquareShadow, Entity_RenderCircularShadow, ...: :27-114, a
  TARGET_3DS branch and an #else branch with the same names; the scan keeps one entry per name, the host compile takes the
  TARGET_3DS one). It is not in the set. Every block entity draws a shadow list, so these are candidates for ent_kmr03_pic's
  `notab 5` (try 2 census).
- SaveBlock call sites: src/entity/SaveBlock.c:73 and :110 `gSPDisplayList(gMainGfxPos++, dlist);` (outside the TARGET_3DS blocks).

## Blocker and start
S7ENT merged (its `.c` path rule and the `# S7ENT` line). `sh 3ds/tools/wt_new.sh S7ENTX`; PM_ROW=S7ENTX PM_LANE=S7ENTX.

## Touch only
3ds/tools/gen_gfx_syms.py (scan() and a new helper above it; S7FLAME edits main() only), 3ds/tools/gen_gfx_tab.sh (the `# S7ENT`
line only), src/entity/SaveBlock.c (the two call sites and one include).

## Exact edits
1. gen_gfx_syms.py, a helper above scan(): `active_text(text)` returns the text with the lines of inactive `#if VERSION_PAL`
   branches blanked. Keep a stack of (known, taking) per `#if`/`#ifdef`/`#ifndef`: `#if VERSION_PAL` pushes (True, False),
   `#if !VERSION_PAL` pushes (True, True), any other opener pushes (False, True); `#else` flips taking only when known;
   `#elif` sets the top to (False, True); `#endif` pops. A line is kept when every taking flag on the stack is True; directive
   lines become empty. scan(): `text = active_text(path.read_text(...))` (the INC includes are then filtered too).
2. gen_gfx_tab.sh, the `# S7ENT` line: add `src/entity/model/SaveBlock.c src/entity/Shadow.c` (keep the list sorted by path:
   Shadow.c goes after the model files, at the end).
3. SaveBlock.c :73 and :110: under `#ifdef TARGET_3DS` `nd_rec_dl(dlist);`, the old line in `#else`. Add `#include "nd_gtab.h"`
   inside the existing `#ifdef TARGET_3DS` include block (:6-8).

## Gate (expected values)
- `PM_ROW=S7ENTX PM_LANE=S7ENTX sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: gen_gfx_tab ran, memcheck OK (hand back the margin).
- `bench.py run nd_host --row S7ENTX --why "S7ENTX gtab"`: PASS, `gtab lists <n> ... bad 0` with n above main's (hand back both).
- `bench.py base ent_kmr03_det` and `bench.py base spot_mac01_det`; one list `bench.py run ent_kmr03_det,spot_mac01_det,ent_kmr03_pic
  --row S7ENTX --why "S7ENTX: SaveBlock and Shadow tables"` (3 starts). cmp ent_kmr03_det and spot_mac01_det: 6/6 byte-equal each.
- ent_kmr03_pic entsync entry at f300: hand back tab, notab, gapt, gaps, gapn (S7ENT try 2: tab 22 notab 5 gapt 15 gaps 0
  gapn 7). Expected: notab below 5 and tab above 22 if the shadow lists were the missing ones; gaps 0 stays a stop rule.
- Budget 8 Azahar starts (7 planned).

## Stop
gen_gfx_syms or gconv_in.c errors, `bad` above 0, a byte difference or `gaps` above 0: stop, commit, hand back the exact output
(2 tries per named cause). A notab value of 5: not a stop; hand it back. Delete this row file in the completing commit.
