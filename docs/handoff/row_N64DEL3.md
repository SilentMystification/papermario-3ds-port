# Row N64DEL3: delete the N64-only branches of the root src files

(Model: Haiku. 60 lines or fewer. N64DEL1's tools as N64DEL2 left them. Plan v3 phase H: C3 and C4 of src/*.c to 0.)
## Code read (ORACLE8, refreshed at main 182ac5091: 28 files, 255 directive lines, root n64 counts 165 and 163)
- src/*.c (root only): 28 files with 255 TARGET_3DS directive lines (ORACLE2 recount at 5c30af943). One is src/entity.c:16
  `#elif defined(TARGET_3DS)` inside `#ifdef SHIFT` (:8). unifdef3ds.py (process, :116-155) classifies `#ifdef SHIFT` as an
  ordinary block, copies the #elif and prints no skip line (as include/macros.h:77 in N64DEL2). N64DEL4 removes it by hand. So
  254 blocks and 0 skip lines. Two `#ifndef TARGET_3DS /* MEM-B v2 ... */` have a comment after the name (N64DEL2's tool edit).
- N64-only counts of the root files (listcen groups, sum): build_n64 165, gfxpos_n64 163. Largest: cam_main.c 22 and 22, model.c
  21 and 7, entity_model.c 19 and 19, item_entity.c 12 and 12, main_loop.c 4 and 10, animator.c 9 and 9.
- After N64DEL2 the totals are build_n64 165 and gfxpos_n64 163 (all in the root files). This row brings them to 0.
## Blocker
N64DEL2 merged (its unifdef3ds.py comment edit and ppcmp.sh `-D__LINE__=0`). Start: `sh 3ds/tools/wt_new.sh N64DEL3`.
PM_ROW=N64DEL3 PM_LANE=N64DEL3 on every call.
## Touch only (places)
src/*.c (root only, not subfolders): only TARGET_3DS directive lines and the lines they guard. No tool edit.
New names: none. Host link: none (no 3ds/src file).
Needs: 3ds/tools/ppcmp.sh has -D__LINE__=0
## Exact edits
Gate rule (ORACLE2): compare only the changed files. 1 build, 0 Azahar. ppcmp has `-dD` after N64DEL2 (macros compared too).
1. The list L of the 28 files: `git grep -l -E '^\s*#\s*(if|ifdef|ifndef|elif).*TARGET_3DS' -- ':(glob)src/*.c' > build3ds/out/pp_list.txt`.
   `sh 3ds/tools/ppcmp.sh build3ds/out/pp_before $(cat build3ds/out/pp_list.txt)` (one docker run).
2. `python 3ds/tools/unifdef3ds.py $(cat build3ds/out/pp_list.txt)`.
3. `sh 3ds/tools/ppcmp.sh build3ds/out/pp_after $(cat build3ds/out/pp_list.txt)`. One commit.
## Gate (expected values)
- Tool output: no skip line and `unifdef3ds: total blocks 254`. src/entity.c:16 `#elif defined(TARGET_3DS)` stays. A second
  run changes nothing.
- `diff -r build3ds/out/pp_before build3ds/out/pp_after` prints nothing. Seeded fault first: append `int ppcmp_seed;` to one
  pp_after file, diff names it, delete the line.
- `python 3ds/tools/listcen.py`: totals build_n64 0 and gfxpos_n64 0 (counter C3 and C4 at 0). The active totals build and
  gfxpos are equal to a listcen run before edit 2 (hand back both runs).
- `PM_ROW=N64DEL3 PM_LANE=N64DEL3 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. nd_host PASS. Budget 0 Azahar.
## Stop and end
A build error, a skip line, a block count other than 254, a changed active count, a total above 0, or a
preprocessor difference: stop, commit, hand back the exact output (2 tries per named cause). Delete this row file at the end.
