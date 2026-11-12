# Row N64DEL2: delete the N64-only branches of src/world, src/entity, src/battle, src/pause, src/filemenu and include

(Model: Haiku. N64DEL1's tool over more directories. Plan: row_ENDGAME.md phase A; moves C3 and C4 of these groups to 0.)

## Code read (ORACLE7, main 235bb8721)
- TARGET_3DS blocks: src/world 11 files 37 blocks, src/entity 6 files 20, src/battle 8 files 17, src/pause 1 file 1,
  src/filemenu 5 files 18, include 11 files 19. N64-only lines: build 107 (world 42, entity 29, battle 36), gfxpos 117.
- include headers are read by every file: a wrong cut there breaks the whole build, which the build gate shows.

## Start
N64DEL1 merged (3ds/tools/unifdef3ds.py). `sh 3ds/tools/wt_new.sh N64DEL2`; PM_ROW=N64DEL2 PM_LANE=N64DEL2.

## Touch only
Files under src/world, src/entity, src/battle, src/pause, src/filemenu (all .c and .h, including .inc.c) and include/*.h,
include/**/*.h: only TARGET_3DS conditional lines and the lines they guard. Not include/PR/gbi.h's macro bodies (S9DEL4).

## Exact edits
1. `python 3ds/tools/unifdef3ds.py <every .c/.h/.inc.c file under those directories>` (use `git ls-files` to list them).
2. One commit; the message lists the `unifdef3ds: skip` lines.

## Gate (expected values)
- A second tool run changes nothing. Build: memcheck OK. nd_host PASS.
- `listcen.py --group` for each directory: build_n64 0 and gfxpos_n64 0 (skips excepted); active counts unchanged.
- Exact preprocessor compare (plan_endgame.md section 5, no Azahar): before the edit, `gcc -E -P` of every touched file with the
  device defines and include paths of 3ds/CMakeLists.txt (as gen_gfx_tab.sh's DEF and INC) into build3ds/out/pp_before/; after the
  edit the same into pp_after/; both runs add `-D__LINE__=0 -w` (line numbers move by design) and drop empty lines
  (`grep -v '^[[:space:]]*$'`); `diff -r` prints nothing. Budget 0 Azahar starts.

## Stop
As N64DEL1. Delete this row file in the completing commit.
