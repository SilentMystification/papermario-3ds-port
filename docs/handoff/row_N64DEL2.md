# Row N64DEL2: delete the N64-only branches of include and the src subfolders other than src/effects

(Model: Haiku. 60 lines or fewer. N64DEL1's tools over more folders. Plan v3 phase H: C3 and C4 of these groups to 0.)
## Code read (ORACLE8, main b8b314758, merged LISTCEN2 tool)
- TARGET_3DS blocks: include 14 files 27, src/world 12 files 38, src/entity 6 files 20, src/battle 9 files 19, src/pause 1 file 1,
  src/filemenu 5 files 18, src/audio 8 files 30, src/evt 4 files 5, src/os 1 file 1. Total 159.
- N64-only counts (listcen groups): world build_n64 42 gfxpos_n64 47, entity 29 and 29, battle 36 and 41. Sum 107 and 117.
- Forms the tool must skip (6): include `#elif defined(TARGET_3DS)` 1, `#if !defined(SPLAT) && ... && !defined(TARGET_3DS)` 1,
  `#if defined(TARGET_3DS) && !defined(ND_GBI_WORDS)` 1; src/battle `#if defined(AVOID_UB) || defined(TARGET_3DS)` 1 and
  `#if defined(SHIFT) || defined(TARGET_3DS)` 1; src/world `#if defined(SHIFT) || defined(TARGET_3DS)` 1. LISTCEN2 counts
  these as active (no n64 line).
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
## Gate (expected values)
- Tool output: 6 skip lines (the forms of the Facts) and `unifdef3ds: total blocks 153`. A second run changes nothing.
- `diff -r build3ds/out/pp_before build3ds/out/pp_after` prints nothing. Seeded fault first: append `int ppcmp_seed;` to one
  pp_after file, diff names it, delete the line.
- `python 3ds/tools/listcen.py`: no group of src/world, src/entity, src/battle has a build_n64 or gfxpos_n64 line. Totals
  build_n64 and gfxpos_n64 fall by 107 and 117 from the value before the row. Active build and gfxpos totals unchanged.
- `PM_ROW=N64DEL2 PM_LANE=N64DEL2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. nd_host PASS. Budget 0 Azahar.
## Stop and end
A build error, a skip line that is not in the Facts, a block count other than 153, a changed active count, or a preprocessor
difference: stop, commit, hand back the exact output (2 tries per named cause). Delete this row file in the completing commit.
