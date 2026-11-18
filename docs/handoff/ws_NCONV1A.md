# ws_NCONV1A (Haiku): the field-set ids ND_FS_ (core 1 names of the typed unit ids) and the listcen line core1 (stage S5; counter: none, core1 base)
Designer: oracle FIN. Branch from main. Worktree build3ds/wt/NCONV1A, branch NCONV1A. PM_ROW=NCONV1A PM_LANE=NCONV1A.
Edit only: new 3ds/include/nd_fs.h, new 3ds/tools/nd_fs_test.c, new 3ds/tests/host.d/ncv1a.sh, 3ds/tools/listcen.py.
## Facts (code read, main 59a0ffc265)
- The unit ids are nd_op.h:11-47 (NDO_OMH = 0x20 to NDO_MISC, then NDO_END_), the sub ids :49-52, NDO_W0 :54, NDO_COMB_W0 :55, NDO_A_RENDERMODE :56.
- listcen reader counts NDO_ tokens (listcen.py:52). In the end the setters and nd_ds.c use ND_FS_ names with the same values (UD1: class core1, outside the end proof).
- Readers of the listcen text: epic_status.sh:6 and host.d lcnobuild.sh, noinc.sh read `listcen total` lines. covsites.py imports the module (no text).
  So core1 prints as `listcen core1 <n>`, never as a `listcen total` line.
## Edits
1. nd_fs.h: the same enums as nd_op.h:11-56 with ND_FS_ in place of NDO_, the same order and the explicit first value 0x20:
   Before (nd_op.h:13-14): `NDO_OMH = 0x20,   /* G_SETOTHERMODE_H ... */` and `NDO_OML,`
   After (nd_fs.h): `ND_FS_OMH = 0x20, /* the other mode high word: a = shift | length << 8, b = value */` and `ND_FS_OML,`
   Copy every id (also END, DROP, the geometry and control ids), ND_FS_MW_*, ND_FS_RARE_*, ND_FS_MISC_*, ND_FS_RECT_TEX, ND_FS_W0(id, a),
   ND_FS_COMB_W0(a), ND_FS_A_RENDERMODE. Comments name the field, not the gbi op, and hold no ND_FS_ name. nd_fs.h includes only <stdint.h>. No G_ names in it.
2. nd_fs_test.c includes nd_op.h and nd_fs.h. One `_Static_assert(ND_FS_x == NDO_x, "x")` per id and sub id (compile check), and
   ND_FS_W0(ND_FS_TILE, 0x123456) == NDO_W0(NDO_TILE, 0x123456) at run time. Prints `ncv1a ids <n> bad <b>`. `--fault` compares ND_FS_TILE with NDO_TILESIZE.
   ncv1a.sh: copy of host.d/nst1.sh for nd_fs_test.c alone (no device file).
3. listcen.py: `CORE1_RX = r'\bND_FS_[A-Z0-9_]+'` next to READER_RX (:52). In main after the reader line (:301): `core1 = count_regex(reader_paths, CORE1_RX)`.
   At the end of main, after the total loop (:338-339): `print('listcen core1 %d' % sum(core1.values()))`. Do not add core1 to `plain` (no total line).
## Steps and tests (output to build3ds/out/, read the last 20 lines; no Azahar run: no device file changes)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh` PASS with `ncv1a ids <n> bad 0` and the fault seen. lcnobuild and noinc stay PASS.
2. `python 3ds/tools/listcen.py > build3ds/out/ncv1a_listcen.txt`. Pass: every `listcen total` line equals the one of main (diff), and
   `listcen core1 <n>` with n = `grep -oE 'ND_FS_[A-Z0-9_]+' 3ds/include/nd_fs.h | wc -l` (nd_fs.h is the only user today).
3. Commit `NCONV1A 1: ND_FS_ ids, listcen core1 line`.
## Fix rights: the listed files; compile errors. A changed total line, a failed host script: stop, send the output path to the oracle. Wait.
Stop: last commit `NCONV1A 2: gate. host PASS; totals unchanged; listcen core1 <n>` deletes this page. One SubagentHandback.
