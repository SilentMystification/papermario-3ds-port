# ws_A: value calls to native calls with codemod, whole directories (counters C1, C2; C10 with --ntab)

Model: Haiku (the tool edits). Start after HUNIT merged: `sh 3ds/tools/wt_new.sh WSA` (new branch from main). PM_ROW=WSA PM_LANE=WSA.
## Objective and counters (FACT: codemod --dry at main b2442aa97, listcen at the same commit: C1 3612, C2 3042)
| group | files | value v | gate benches (drec, byte-equal to the stored base) |
|---|---|---|---|
| 1 src/effects/[a-e]*.c | 35 | 248 | gold_55_drec (effect_3D) |
| 2 src/effects/[f-l]*.c | 28 | 241 | fxpic_91_drec (fright_jar) |
| 3 src/effects/[m-r]*.c | 19 | 173 | gold_33_drec (music_note) |
| 4 src/effects/[s-z]*.c | 45 | 330 | gold_12_drec (sparkles), gold_63_drec (snowfall) |
| 5 src/world | 45 | 137 | spot_mac01_drec, spot_mac00w_det |
| 6 src/entity | 17 | 74 | sbk14_drec (Tweester) |
| 7 src/pause src/filemenu src/battle src/evt | 21 | 77 | spot_mac01_drec |
Groups 5-7 need N64DEL2 on main (`git log main --oneline | grep -q N64DEL2`): then `git merge main` first. Else hand back after
group 4. v of 5-7 falls after N64DEL2 (GUESS). Root src/*.c belong to ws_C: never run codemod on them or outside the group.
## Loop per group
1. `python 3ds/tools/listcen.py > build3ds/out/lc_before_<g>.txt`, `python 3ds/tools/codemod.py --dry $NTAB <files> > build3ds/out/cm_<g>.txt`.
   NTAB = `--ntab build3ds/out/ntab_tables.txt` when codemod.py has --ntab (CONV2F merged) and the file exists after a build, else empty.
2. Real run: the same command without --dry. Then add the includes: a file with `nd_val_` and no `#include "nd_val.h"` gets it
   after its last `#include` line; the same for `nd_xf_` and "nd_xf.h". One script for the whole group.
3. Host: `sh 3ds/tests/codemod/run.sh` and `sh 3ds/tools/nd_host_test.sh > build3ds/out/host.txt 2>&1` (hunit ok 7/7, PASS).
4. One build: `PM_ROW=WSA PM_LANE=WSA sh 3ds/build.sh > build3ds/out/build_<g>.txt 2>&1`. memcheck OK, margin 310 or more.
5. listcen after: totals build and gfxpos = before - v (v: dry `codemod total value`). NTAB: recdl = before - dry nd_rec_dl value.
6. Gate: for each bench of the group, `python 3ds/tools/bench.py run <b> --row WSA --why "WSA g<g>"` and `bench.py cmp <b> --row WSA`
   (a base that is not stored: `bench.py base <b>` first). One differing bit fails. At most 2 runs per group.
7. Commit the group: `WSA g<g>: codemod value calls of <dirs> (v <v>)`. Only the changed files.
## Edit shapes (before -> after; the tool writes them)
- big_smoke_puff.c (CODEMODFX7): `gSPSegment(gMainGfxPos++, 0x09, X);` -> `nd_val_seg(0x09, X);` :131. `gSPMatrix(gMainGfxPos++, &m, F);`
  -> `nd_xf_push(&m, F);` :136. `gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);` -> `nd_xf_pop(G_MTX_MODELVIEW);` :174.
- src/world/area_mac/mac_00/water_gfx.c:14-15: `gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 128, 0);` `gDPPipeSync(gMainGfxPos++);`
  -> `nd_val_prim(0, 0, 0, 0, 128, 0);` (the sync line is deleted). Include shape: `#include "model.h"` -> plus `#include "nd_val.h"`.
- With NTAB, water_splash.c:189 (CONV2F): `nd_rec_dl(D_09000100_3BCB90);` -> `nd_draw_list(D_09000100_3BCB90, ND_SELF);`.
## Tests that decide, and what you may edit
Pass: codemod run.sh PASS, nd_host PASS, build OK, the exact listcen drop, every cmp byte-equal. A build error in a group file:
edit only the include lines of that file. A `codemod keep` line is not a fault (the site stays). A cmp difference or a listcen
drop other than v: do not commit the group, write the output to build3ds/out/wsa_g<g>.txt, send the path to the oracle, go on
with the next group. Never edit codemod.py, 3ds/src or a call by hand.
## Stop and hand-back
Stop after group 7, or under the CLAUDE.md 5 rule. Stop test (orchestrator): after 3 merged groups C1 falls by 200 or more
(FACT, dry run: groups 1-3 give v 662). Hand-back (8 lines): last hash, per group v and cmp result, listcen build and gfxpos totals.
