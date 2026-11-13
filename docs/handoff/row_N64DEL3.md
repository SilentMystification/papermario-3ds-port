# Row N64DEL3: delete the N64-only branches of the root src files

(Model: Haiku. 60 lines or fewer. N64DEL1's tools as N64DEL2 left them. Plan v3 phase H: C3 and C4 of src/*.c to 0.)
## Code read (ORACLE8, main b8b314758, merged LISTCEN2 tool)
- src/*.c (root only): 28 files with 255 TARGET_3DS blocks. One `#elif defined(TARGET_3DS)` (skip). Two `#ifndef TARGET_3DS
  /* MEM-B v2 ... */` with a comment after the name (N64DEL2's tool edit reads them).
- N64-only counts of the root files (listcen groups, sum): build_n64 165, gfxpos_n64 163. Largest: cam_main.c 22 and 22, model.c
  21 and 7, entity_model.c 19 and 19, item_entity.c 12 and 12, main_loop.c 4 and 10, animator.c 9 and 9.
- After N64DEL1 (430, 489) and N64DEL2 (107, 117) this row brings the totals build_n64 702 and gfxpos_n64 769 to 0.
## Blocker
N64DEL2 merged (its unifdef3ds.py comment edit and ppcmp.sh `-D__LINE__=0`). Start: `sh 3ds/tools/wt_new.sh N64DEL3`.
PM_ROW=N64DEL3 PM_LANE=N64DEL3 on every call.
## Touch only (places)
src/*.c (root only, not subfolders): only TARGET_3DS directive lines and the lines they guard. No tool edit.
## Exact edits
1. `sh 3ds/tools/ppcmp.sh build3ds/out/pp_before src/*.c`.
2. `python 3ds/tools/unifdef3ds.py src/*.c`.
3. `sh 3ds/tools/ppcmp.sh build3ds/out/pp_after src/*.c`. One commit. The message lists the skip line.
## Gate (expected values)
- Tool output: 1 skip line (the `#elif`) and `unifdef3ds: total blocks 254`. A second run changes nothing.
- `diff -r build3ds/out/pp_before build3ds/out/pp_after` prints nothing. Seeded fault first: append `int ppcmp_seed;` to one
  pp_after file, diff names it, delete the line.
- `python 3ds/tools/listcen.py`: totals build_n64 0 and gfxpos_n64 0 (counter C3 and C4 at 0). Active totals unchanged
  (build 3768, gfxpos 3198 unless a merge since b8b314758 changed them: compare with the run before the edit).
- `PM_ROW=N64DEL3 PM_LANE=N64DEL3 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. nd_host PASS. Budget 0 Azahar.
## Stop and end
A build error, a skip line other than the `#elif`, a block count other than 254, a changed active count, a total above 0, or a
preprocessor difference: stop, commit, hand back the exact output (2 tries per named cause). Delete this row file at the end.
