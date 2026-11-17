# ws_ENTLST (Haiku): entity list handles take the type NdLst (C7 gfxtype -61)
Designer: oracle NEXT1 (SendMessage). Worktree build3ds/wt/ENTLST, branch ENTLST from main, PM_ROW=ENTLST PM_LANE=ENTLST.
Main at release: 66cd65f3cd. Shares no file with FLTGT, EFXGT, RECBUF. Model: Haiku (type renames, one regex).
## Objective
- FACT (nd_gtab.h:66): `typedef const void* NdLst;`. FACT (entity.h:66): `ENTITY_ADDR(entity, type, data)` is `(type)(data)`.
- Every Gfx token below is a list handle that only goes to nd_draw_list or into another handle. A type change gives the same code.
- FACT (gen_ntab_entries.py:75): the entry scan matches only `ENTITY_ADDR(e, Gfx*, x)`. Tweester.c:44 and SaveBlock.c:46 use it.
- Counter FACT (listcen on 66cd65f3cd): src/entity gfxtype 57, include/entity.h 9. Kept: HiddenPanel.c:10 :298 :330 (the list goes to
  mdl_project_tex_coords, model.c:3731, which reads list words), entity.h:248 :459. Move: gfxtype -61 (416 -> 355 without FLTGT).
## Edits (directory order)
1. include/entity.h lines 96 145 146 179 265 271 286 only: `Gfx**` -> `const void**`, `Gfx*` -> `const void*` (entity.h has no nd_gtab.h).
   Before `    /* 0x008 */ Gfx** fragmentsGfx;` after `    /* 0x008 */ const void** fragmentsGfx; /* NdLst* */`.
2. These 20 files, every `Gfx**` -> `NdLst*`, then every `Gfx*` -> `NdLst`: src/entity/{Chest,HeartBlock,SaveBlock,SuperBlock,ShatteringBlock,
   ShatteringBlock_common,WoodenCrate}.c, default/{BoardedFloor,BombableRock}.c, jan_iwa/{ArrowSign,Plants1}.c, sbk_omo/{Tweester,StarBoxLauncher}.c,
   model/{BoardedFloor,BombableRock,ShatteringBrickBlock,ShatteringHammer1Block,ShatteringHammer2Block,ShatteringHammer3Block,WoodenCrate}.c.
   One command: `sed -i -E 's/\bGfx\*\*/NdLst*/g; s/\bGfx\*/NdLst/g' <the 20 files>`. Each file includes nd_gtab.h (FACT grep).
   Before (ShatteringBlock_common.c:195) `fragmentDlist = ENTITY_ADDR(entity, Gfx*, *gfx++);` after `... ENTITY_ADDR(entity, NdLst, *gfx++);`.
   Before (model/WoodenCrate.c:744) `Gfx* Entity_WoodenCrate_FragmentsRender[] = {` after `NdLst Entity_WoodenCrate_FragmentsRender[] = {`.
3. src/entity/HiddenPanel.c line 33 only: `ENTITY_ADDR(entity, Gfx*,` -> `ENTITY_ADDR(entity, NdLst,`. Lines 10, 298, 330 stay.
4. 3ds/tools/gen_ntab_entries.py:75 regex `Gfx\s*\*` -> `(?:Gfx\s*\*|NdLst)`. Line 11 docstring: add `or ENTITY_ADDR(e, NdLst, x)`.
5. 3ds/tests/entsrc/x/w.c: new line 8 `    nd_draw_list(ENTITY_ADDR(entity, NdLst, NDN(Ent_B)), pe_w_7);` (the `}` moves to 9).
   3ds/tests/entsrc/a.txt: add line 3, line 1 with `Ent_A` -> `Ent_B` and `w.c:6` -> `w.c:8`.
   3ds/tests/host.d/entsrc.sh: after the ENTITY_ADDX fault, a second seeded fault `sed 's/|NdLst//'` on a copy; its output must differ from a.txt.
## Loop (each output to build3ds/out/, read the last 20 lines)
0. Before step 1, the command of gen_gfx_tab.sh:28 with a new output file:
   `python3 3ds/tools/gen_ntab_entries.py src/effects $(cat $(ls 3ds/tools/ntab_ent.d/*.txt | LC_ALL=C sort) | sed 's/#.*//' | tr -s ' \t\r\n' ' ') > build3ds/out/entlst_gne0.txt`
6. After step 5: the same command into build3ds/out/entlst_gne1.txt. `cmp` the two files. Commit `ENTLST 1: entity list handles NdLst`.
7. `python 3ds/tools/bench.py base sbk14_drec --row ENTLST`, then the same for entcape_kmr03_drec (base runs, not charged).
8. `sh 3ds/tools/rowloop.sh --row ENTLST --bench sbk14_drec,entcape_kmr03_drec --why "ENTLST gate" > build3ds/out/entlst_loop.txt 2>&1`.
## Tests that decide
- `cmp` of entlst_gne0.txt and entlst_gne1.txt: equal. `entsrc ok` in build3ds/out/loop_ENTLST_host.txt.
- `LOOP PASS`: host PASS, build rc 0, both cmp lines byte-equal. Budget: 2 builds, 4 Azahar starts.
- `python 3ds/tools/listcen.py | grep "^listcen total gfxtype"`: 355 (FLTGT not merged) or 351 (FLTGT merged).
## What you may edit to fix a fail
Only the files above. A compile error on one handle line: send build3ds/out/loop_ENTLST_build.txt to the oracle. A gne diff or a cmp DIFF: stop, send the path.
## Stop and hand-back
Last commit `ENTLST 2: gate. sbk14_drec, entcape_kmr03_drec byte-equal; gfxtype <n>` deletes this page.
One SubagentHandback: commit hashes, LOOP lines, the gne cmp result, listcen gfxtype.
