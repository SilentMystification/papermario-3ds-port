# Row N64DEL2: delete the N64-only branches of include and the src subfolders other than src/effects

(Model: Haiku. 60 lines or fewer. N64DEL1's tools over more folders. Plan v3 phase H: C3 and C4 of these groups to 0.)
## Code read (ORACLE8, refreshed at main 182ac5091: the block counts and the n64 counts below are unchanged; C3 272, C4 280)
- TARGET_3DS blocks: include 14 files 27, src/world 12 files 38, src/entity 6 files 20, src/battle 9 files 19, src/pause 1 file 1,
  src/filemenu 5 files 18, src/audio 8 files 30, src/evt 4 files 5, src/os 1 file 1. Total 159.
- N64-only counts (listcen groups): world build_n64 42 gfxpos_n64 47, entity 29 and 29, battle 36 and 41. Sum 107 and 117.
- Correction (ORACLE8, 2026-11-13, from the coder's dry run at 6fce31e6f): the tool prints 7 skip lines, and N64DEL4 removes
  them by hand. include/PR/os_convert.h:74 and include/common_structs.h:2258 (`#if defined(TARGET_3DS)` blocks that have an
  #elif), include/common.h:20 (`&& !defined(ND_GBI_WORDS)`), include/include_asm.h:7 (`!defined(SPLAT) && ...`),
  src/battle/area/kzn2/actor/lava_piranha.c:292 and src/world/common/lava_piranha/part1.inc.c:9 (`SHIFT || TARGET_3DS`),
  src/battle/common/stage/lib/WaterEffects.inc.c:12 (`AVOID_UB || TARGET_3DS`). include/macros.h:77 `#elif defined(TARGET_3DS)`
  sits in an `#ifdef DEBUG` block. The tool copies it, prints no skip line and counts no block. Blocks: 159 - 7 - 1 = 151.
- Forms with a comment after the name: include `#ifndef TARGET_3DS /* DIRECT D9 ... */` 1, src/world `#ifdef TARGET_3DS /* HC4f3 ...
  */` 2 (and src/*.c 2 for N64DEL3). include/macros.h and include/PR/libaudio.h use __LINE__: a deleted line moves the __LINE__
  value of every later ASSERT in a .c file, so `gcc -E` needs `-D__LINE__=0`.
## Blocker
N64DEL1 merged (3ds/tools/unifdef3ds.py, ppcmp.sh). Start: `sh 3ds/tools/wt_new.sh N64DEL2`. PM_ROW=N64DEL2 PM_LANE=N64DEL2.
## Touch only (places)
3ds/tools/unifdef3ds.py: the regex that reads the condition of a directive. 3ds/tools/ppcmp.sh: the gcc line. Files of
`git ls-files include src/world src/entity src/battle src/pause src/filemenu src/audio src/evt src/os` ending .c or .h: only
TARGET_3DS directive lines and the lines they guard.
## Exact edits
1. unifdef3ds.py: before the condition is matched, remove a trailing `/* ... */` or `// ...` comment and trailing blanks from
   the directive text. Nothing else changes.
2. ppcmp.sh: add `-Wno-builtin-macro-redefined -D__LINE__=0` to the gcc options.
3. `sh 3ds/tools/ppcmp.sh build3ds/out/pp_before <the .c and .h files of the list> src/main.c src/model.c src/hud_element.c`.
   `python 3ds/tools/unifdef3ds.py <the same files of the list>`. ppcmp again into build3ds/out/pp_after. One commit (tools and
   files). The message lists the skip lines.
Resume: edit 2 is done (6fce31e6f). Do edit 1 and edit 3. The 7 skip lines are expected now.
## Gate (expected values)
- Tool output: the 7 skip lines of the Facts and `unifdef3ds: total blocks 151`. A second run changes nothing.
- `diff -r build3ds/out/pp_before build3ds/out/pp_after` prints nothing. Seeded fault first: append `int ppcmp_seed;` to one
  pp_after file, diff names it, delete the line.
- `python 3ds/tools/listcen.py`: no group of src/world, src/entity, src/battle has a build_n64 or gfxpos_n64 line. Totals
  build_n64 and gfxpos_n64 fall from 272 and 280 to 165 and 163. Active totals stay build 3697 and gfxpos 3127.
- `PM_ROW=N64DEL2 PM_LANE=N64DEL2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. nd_host PASS. Budget 0 Azahar.
## Stop and end
A build error, a skip line that is not in the Facts, a block count other than 151, a changed active count, or a preprocessor
difference: stop, commit, hand back the exact output (2 tries per named cause). Delete this row file in the completing commit.
