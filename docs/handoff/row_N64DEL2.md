# Row N64DEL2: delete the N64-only branches of include and the src subfolders other than src/effects

(Model: Haiku. 60 lines or fewer. N64DEL1's tools over more folders. Plan v3 phase H: C3 and C4 of these groups to 0.)
## Code read (ORACLE8, refreshed at main 182ac5091: the block counts and the n64 counts below are unchanged; C3 272, C4 280)
- TARGET_3DS blocks: include 14 files 27, src/world 12 files 38, src/entity 6 files 20, src/battle 9 files 19, src/pause 1 file 1,
  src/filemenu 5 files 18, src/audio 8 files 30, src/evt 4 files 5, src/os 1 file 1. Total 159.
- N64-only counts (listcen groups): world build_n64 42 gfxpos_n64 47, entity 29 and 29, battle 36 and 41. Sum 107 and 117.
- Correction (ORACLE8, 2026-11-13, from the coder's dry run at 6fce31e6f): the tool prints 7 skip lines, and N64DEL4 removes
  them by hand. include/PR/os_convert.h:74 and include/common_structs.h:2258 (`#if defined(TARGET_3DS)` blocks that have an
  #elif), include/common.h:20 (`&& !defined(ND_GBI_WORDS)`), include/include_asm.h:7 (`!defined(SPLAT) && !defined(__CTX__)`),
  src/battle/area/kzn2/actor/lava_piranha.c:292 and src/world/common/lava_piranha/part1.inc.c:9 (`defined(SHIFT) || defined(TARGET_3DS)`),
  src/battle/common/stage/lib/WaterEffects.inc.c:12 (`defined(AVOID_UB) || defined(TARGET_3DS)`). include/macros.h:77 `#elif defined(TARGET_3DS)`
  sits in an `#ifdef DEBUG` block. The tool copies it, prints no skip line and counts no block. Blocks: 159 - 7 - 1 = 151.
- Forms with a comment after the name: include `#ifndef TARGET_3DS /* DIRECT D9 ... */` 1, src/world `#ifdef TARGET_3DS /* HC4f3 ...
  */` 2 (and src/*.c 2 for N64DEL3). include/macros.h and include/PR/libaudio.h use __LINE__: a deleted line moves the __LINE__
  value of every later ASSERT in a .c file, so `gcc -E` needs `-D__LINE__=0`.
## Blocker
N64DEL1 merged (3ds/tools/unifdef3ds.py, ppcmp.sh). Start: `sh 3ds/tools/wt_new.sh N64DEL2`. PM_ROW=N64DEL2 PM_LANE=N64DEL2.
## Touch only (places)
3ds/tools/ppcmp.sh: the gcc line. Files of
`git ls-files include src/world src/entity src/battle src/pause src/filemenu src/audio src/evt src/os` ending .c or .h: only
TARGET_3DS directive lines and the lines they guard.
New names: none. Host link: none (no 3ds/src file).
Needs: 3ds/tools/unifdef3ds.py has total blocks
## Exact edits (correction 3, ORACLE2 2026-11-13: the gate compares only the changed files. Keep the worktree edits)
State in build3ds/wt/N64DEL2: 6fce31e6f (ppcmp `-D__LINE__=0`) plus 55 changed files, not committed (unifdef3ds.py was run).
1. Old edit 1 is already on main: unifdef3ds.py:20-23 (clean) strips the comment of a directive. Do not edit unifdef3ds.py.
2. ppcmp.sh gcc line: add `-dD` (the #define lines go into the output, so a changed macro shows in the compare).
3. Commit the 55 files and ppcmp.sh: `N64DEL2: delete the N64-only branches of include and the src subfolders (unifdef3ds)`.
4. The file list L (about 55 names), from the worktree root:
   `git diff --name-only 6fce31e6f HEAD -- '*.c' '*.h' | grep -v '\.inc\.c$'`, plus the 3 includers of the changed fragments:
   src/world/area_dgb/dgb_04/entity.c (SuperBlock.inc.c), src/world/area_kzn/kzn_19/npc.c (part2.inc.c),
   src/world/area_mim/mim_01/hint.c (GetFirstTriangleNormal.inc.c). Write L to build3ds/out/pp_list.txt.
5. The before tree (a plain copy, no git worktree): `mkdir -p build3ds/out/n64base`,
   `git archive 6fce31e6f 3ds include src | tar -x -C build3ds/out/n64base`, then
   `cp 3ds/tools/ppcmp.sh build3ds/out/n64base/3ds/tools/ppcmp.sh` (the same gcc line on both sides).
6. `sh build3ds/out/n64base/3ds/tools/ppcmp.sh pp_before $(cat build3ds/out/pp_list.txt)` (it writes
   build3ds/out/n64base/pp_before) and `sh 3ds/tools/ppcmp.sh build3ds/out/pp_after $(cat build3ds/out/pp_list.txt)`. Each is
   one docker run.
## Gate (expected values; 1 build, 0 Azahar)
1. Host check of the tool: `python 3ds/tools/unifdef3ds.py $(git diff --name-only 6fce31e6f HEAD -- '*.c' '*.h')` again
   prints the 7 skip lines of the Facts and `unifdef3ds: total blocks 0`, and `git status --short` stays empty (idempotent).
   Hand back the first run's `total blocks` line if it was saved (expected 151).
2. `diff -r build3ds/out/n64base/pp_before build3ds/out/pp_after` prints nothing. Seeded fault first: append
   `int ppcmp_seed;` to one pp_after file, diff names it, delete the line. Both ppcmp runs exit 0.
3. `python 3ds/tools/listcen.py`: totals build_n64 165 and gfxpos_n64 163. The active totals build and gfxpos equal
   build3ds/out/listcen_before_totals.txt.
4. `PM_ROW=N64DEL2 PM_LANE=N64DEL2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. nd_host PASS.
## Stop and end
A ppcmp exit other than 0, a build error, a skip line that is not in the Facts, a changed active count, or a preprocessor
difference: stop, commit, hand back the exact output. Delete this row file in the completing commit.
