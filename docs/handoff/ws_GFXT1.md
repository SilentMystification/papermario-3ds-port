# ws_GFXT1 (Haiku): the list handles of 3 game structs lose the Gfx type (counter C7 gfxtype -5)
Designer: oracle (SendMessage). Worktree build3ds/wt/GFXT1, branch GFXT1 from main. PM_ROW=GFXT1 PM_LANE=GFXT1. Waits for nothing.
Edit only: src/effects/flame.c, src/entity/HiddenPanel.c, include/entity.h. No NST chain file. Model: Haiku (5 type edits, given below).
## Objective and counter
- C7 gfxtype falls by 5 (FACT: grep of `\bGfx\b` in code lines: flame.c:17, HiddenPanel.c:298, HiddenPanel.c:330, entity.h:248, entity.h:459).
- The fields keep a list handle (NDN(x) or a list name). The handle type is `NdLst` = `const void*` (3ds/include/nd_gtab.h:66).
- Size and layout do not change (FACT: 4-byte pointer either way). The object code does not change (GUESS). drec stays byte-equal.
## Readers (FACT, grep of every field use; none needs an edit)
- FlamePreset.dlist: initializers flame.c:32, :40, :48, :56 (`NDN(...)`), reader flame.c:198 `nd_draw_list(preset->dlist, ND_SELF)`.
- HiddenPanelData.renderDList: HiddenPanel.c:36, :332 pass it to `mdl_project_tex_coords(s32, const void*, ...)` (HiddenPanel.c:10).
- EntityModel.gfx.displayList: entity_model.c:115, :204, :275, :278, :299, :347, :469, :473, :490, :775, :787. Each one assigns
  a `void*`, compares or stores into a `const void*`, casts to `uintptr_t`, or passes it to `nd_rec_ent_draw(const void*, ...)` (nd_cmd.h:30).
## Edits (5 lines)
1. src/effects/flame.c:17. Before `    /* 0x0C */ Gfx* dlist;` After `    /* 0x0C */ NdLst dlist;` (flame.c:2 includes nd_gtab.h).
2. src/entity/HiddenPanel.c:298. Before `    Gfx* dlist;` After `    const void* dlist; /* NdLst */`.
3. src/entity/HiddenPanel.c:330. Before `    data->renderDList = ENTITY_ADDR(entity, Gfx*, dlist);`
   After `    data->renderDList = ENTITY_ADDR(entity, const void*, dlist);`
4. include/entity.h:248. Before `    /* 0x78 */ Gfx* renderDList;` After `    /* 0x78 */ const void* renderDList; /* NdLst */`.
5. include/entity.h:459. Before `                    Gfx* displayList;` After `                    const void* displayList; /* NdLst */`.
   Use `const void*` in entity.h, as entity.h:179 does. Do not add an include to entity.h.
## Steps (each output to build3ds/out/, read the last 20 lines). Budget: 1 build, 1 Azahar run (base not charged).
1. `python 3ds/tools/listcen.py > build3ds/out/gfxt1_lc0.txt` before the edits. Keep the line `listcen total gfxtype <n>`.
2. Do edits 1-5. `python 3ds/tools/listcen.py > build3ds/out/gfxt1_lc1.txt`. Pass: gfxtype is n - 5. Commit `GFXT1 1: list handles without Gfx`.
3. `python 3ds/tools/bench.py base fxops_drec --commit $(git merge-base HEAD main)` (base run, not charged).
4. `sh 3ds/tools/rowloop.sh --row GFXT1 --bench fxops_drec --why "GFXT1 gate" > build3ds/out/gfxt1_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, `cmp fxops_drec: ... byte-equal` against the merge-base.
- gfxtype total of step 2 is exactly 5 lower than step 1. Any other listcen total is equal.
## Fix rights
- A compile error in the 3 files: fix the type in the same 3 files only. Never edit entity_model.c or an NST chain file.
- A DIFF, a gfxtype move other than -5, a tool fault: stop. Send the output path to the oracle with SendMessage. Wait.
## Stop and hand-back
- Last commit `GFXT1 2: gate. fxops_drec byte-equal; gfxtype <n> -> <n-5>` deletes this page. One SubagentHandback: hashes, LOOP lines, gfxtype values.
