# ws_IMGFXT (Haiku): imgfx buffers as u32 words, imgfx and animator list members as void* (C7 gfxtype -9)
Designer: oracle NEXT2 (SendMessage). Worktree build3ds/wt/IMGFXT, branch IMGFXT from main, PM_ROW=IMGFXT PM_LANE=IMGFXT.
Main at release: 7c701ef597. Edit only src/imgfx.c, include/imgfx.h, include/common_structs.h (lines 1089, 2421, 2433 only).
Shares no file with ENTLST, EFXGT, EFFCEN, FLTGT, BLANKET, GTSMALL, FXDIG. Model: Haiku (type changes, same code).
## Objective
- Each edit keeps the machine code: the same byte counts, and pointer members that are only stored, compared, passed or cast.
- FACT (grep src include): no `+`, `-`, `[ ]`, `++`, `&` or sizeof on displayList, dlist, gfxOffset or gfxBufs. So void* gives no pointer step change.
- FACT (imgfx.c:1282-1290): gfxBufs[i] holds anim->gfxCount units of 8 bytes (imgfx_swap32 of gfxCount * 2 words). 8 = 2 * sizeof(uint32_t).
- FACT (common_structs.h:1108): AnimatorNodeBlueprint.displayList is already void*; animator.c:348, :1013, :1107 copy between them.
  FACT (nd_cmd.h:30, nd_gtab.h:81, :135): nd_rec_ent_draw, nd_rec_dl and nd_ntab_find take const void*.
- FACT (grep): ImgFXTexture.dlist has no user. ImgFXOverlayTexture.displayList: set by NDN() in monstar.c:158 and sam_05 npc.c:60, read at imgfx.c:1805.
- Counter FACT (listcen 038ac581fe): imgfx.c gfxtype 5 (:96 :1220 :1282 :1283 :1286), imgfx.h 1 (:16), common_structs.h 3. Move: gfxtype -9.
- lkey: keep imgfx.c:1293 `(uint32_t*)state->gfxBufs[i]` and animator.c:866 as they are (allow.txt counts stay; no allow.txt edit).
## Edits (match the text)
1. src/imgfx.c:96 `/* 0x70 */ Gfx* gfxBufs[2];` -> `/* 0x70 */ uint32_t* gfxBufs[2]; /* typed units, 2 words each */`.
2. :1220 `static void imgfx_make_dev(NdNDev* d, Gfx* buf, unsigned n) {` -> `static void imgfx_make_dev(NdNDev* d, uint32_t* buf, unsigned n) {`.
   :1221 stays.
3. :1282 and :1283 `heap_malloc(anim->gfxCount * sizeof(Gfx))` -> `heap_malloc(anim->gfxCount * 2 * sizeof(uint32_t))`.
   :1286 `romEnd = romStart + anim->gfxCount * sizeof(Gfx);` -> `romEnd = romStart + anim->gfxCount * 2 * sizeof(uint32_t);`.
4. include/imgfx.h:16 `/* 0x04 */ Gfx* gfxOffset; // Gfx for creating mesh from vertices` -> `/* 0x04 */ void* gfxOffset; // ROM offset of the mesh list units`.
5. include/common_structs.h:1089 `/* 0x00 */ Gfx* displayList;` -> `/* 0x00 */ void* displayList; /* a list handle */`.
   :2421 `/* 0x14 */ Gfx* displayList;` -> `/* 0x14 */ void* displayList; /* a list handle */`. :2433 `/* 0x14 */ Gfx* dlist;` -> `/* 0x14 */ void* dlist; /* unused */`.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edit. `grep -nw Gfx src/imgfx.c include/imgfx.h` prints nothing. `grep -cw Gfx include/common_structs.h` prints 1 (the :1115 comment; listcen strips comments).
   `python 3ds/tools/lkey_cen.py 3ds/tests/lkey/allow.txt .` prints `lkey census ok`. Do not run host.d scripts in Git Bash (docker only).
2. Commit `IMGFXT 1: imgfx buffers as u32 words; imgfx, overlay and animator list members void*`.
3. `python 3ds/tools/bench.py base <b> --row IMGFXT` for b = nimgm_anim_drec, axns_sam2_drec, axns_kzn2_drec (base runs, not charged).
4. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row IMGFXT --bench nimgm_anim_drec,axns_sam2_drec,axns_kzn2_drec --why "IMGFXT gate" > build3ds/out/imgfxt_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, 3 cmp lines byte-equal. Budget: 2 builds, 6 Azahar starts.
- `python 3ds/tools/listcen.py | grep "^listcen total gfxtype"`: 9 lower than on the merge-base (read both values).
## Fix rights: only the 3 files. A compile error, an lkey line, a host FAIL or a cmp DIFF: stop, send the path to the oracle, wait.
## Stop and hand-back
Last commit `IMGFXT 2: gate. nimgm_anim_drec, axns_sam2_drec, axns_kzn2_drec byte-equal; gfxtype <n>` deletes this page.
One SubagentHandback: hashes, LOOP lines, gfxtype.
