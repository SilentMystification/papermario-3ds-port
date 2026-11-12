# Row N64DEL3: delete the N64-only branches of the root src files

(Model: Haiku. N64DEL1's tool over src/*.c. Plan: row_ENDGAME.md phase A; moves C3 and C4 of the root files to 0.)

## Code read (ORACLE7, main 235bb8721)
- src/*.c: 27 files with 245 TARGET_3DS blocks; N64-only lines hold the rest of C3/C4 (largest: model.c 21, screen_overlays.c 22,
  cam_main.c 22, entity_model.c 19, state_title_screen.c 13, status_icons.c 13, item_entity.c 12).

## Start
N64DEL1 merged. May run with N64DEL2 (other files). `sh 3ds/tools/wt_new.sh N64DEL3`; PM_ROW=N64DEL3 PM_LANE=N64DEL3.

## Touch only
src/*.c (root only, not subdirectories): only TARGET_3DS conditional lines and the lines they guard.

## Exact edits
1. `python 3ds/tools/unifdef3ds.py src/*.c`. 2. One commit listing the skip lines.

## Gate (expected values)
- A second tool run changes nothing. Build: memcheck OK. nd_host PASS.
- `listcen.py`: build_n64 and gfxpos_n64 totals equal to the sums left in skipped blocks only (hand back the totals; after
  N64DEL1-3 the expected total is 0 plus skips).
- Exact preprocessor compare (plan_endgame.md section 5, no Azahar): before the edit, `gcc -E -P` of every touched file with the
  device defines and include paths of 3ds/CMakeLists.txt (as gen_gfx_tab.sh's DEF and INC) into build3ds/out/pp_before/; after the
  edit the same into pp_after/; both runs add `-D__LINE__=0 -w` (line numbers move by design) and drop empty lines
  (`grep -v '^[[:space:]]*$'`); `diff -r` prints nothing. Budget 0 Azahar starts.

## Stop
As N64DEL1. Delete this row file in the completing commit.
