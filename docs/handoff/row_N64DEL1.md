# Row N64DEL1: delete the N64-only branches of src/effects (and write the tool that N64DEL2 and N64DEL3 reuse)

(Model: Haiku. A deterministic tool plus its run over one directory. Decision 2026-11-12: the port stops building for N64; tag
pre-list-removal keeps the old tree. Plan: row_ENDGAME.md phase A; moves C3 build_n64 and C4 gfxpos_n64 of src/effects to 0.)

## Code read (ORACLE7, main 235bb8721)
- src/effects: 106 files with 373 `#ifdef/#ifndef TARGET_3DS` blocks; N64-only lines hold 379 gDP/gSP calls and 438 gMainGfxPos
  uses (census of row_ENDGAME.md section 1). The compiled 3DS code does not change when those lines go.

## Start
LISTCEN2 merged (its `--group` counts are the gate). `sh 3ds/tools/wt_new.sh N64DEL1`; PM_ROW=N64DEL1 PM_LANE=N64DEL1.

## Touch only
New 3ds/tools/unifdef3ds.py; src/effects/*.c (only TARGET_3DS conditional lines and the N64-only lines they guard).

## Exact edits
1. unifdef3ds.py `<file>...`: rewrites each file in place, line based, comments untouched. For a block opened by `#ifdef
   TARGET_3DS` or `#if defined(TARGET_3DS)` (no `&&`/`||`): keep the lines up to its `#else`/`#endif`, drop the `#else` part,
   drop the three directive lines. For `#ifndef TARGET_3DS` or `#if !defined(TARGET_3DS)`: drop the lines up to `#else`/`#endif`,
   keep the `#else` part, drop the directives. Nested blocks of other conditions inside are copied as they are (kept or dropped with
   their parent part). A block with `#elif` or a compound condition naming TARGET_3DS is left unchanged and printed as
   `unifdef3ds: skip <file>:<line>`. Prints `unifdef3ds: <file> blocks <n>` per changed file. Idempotent (a second run changes
   nothing).
2. Run it on src/effects/*.c. Commit the tool and the files in one commit; the commit message lists the skip lines.

## Gate (expected values)
- `python 3ds/tools/unifdef3ds.py src/effects/*.c` a second time: no file changes (`git status` clean).
- `PM_ROW=N64DEL1 PM_LANE=N64DEL1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. nd_host PASS.
- `python 3ds/tools/listcen.py --group src/effects`: build_n64 0 and gfxpos_n64 0 for every file (skips excepted: name them);
  active build 1388 and gfxpos 1365 unchanged (within the skips).
- Exact preprocessor compare (plan_endgame.md section 5, no Azahar): before the edit, `gcc -E -P` of every touched file with the
  device defines and include paths of 3ds/CMakeLists.txt (as gen_gfx_tab.sh's DEF and INC) into build3ds/out/pp_before/; after the
  edit the same into pp_after/; both runs add `-D__LINE__=0 -w` (line numbers move by design) and drop empty lines
  (`grep -v '^[[:space:]]*$'`); `diff -r` prints nothing. Budget 0 Azahar starts.

## Stop
A build error, an active count that changed, or a preprocessor difference: stop, commit, hand back the exact output (2 tries per named
cause). Delete this row file in the completing commit.
