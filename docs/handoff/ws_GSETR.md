# ws_GSETR (Haiku): 5 root sources join the converter; gset moves 35 ok lists of 11 sources (C5 C6 C7)
Designer to ask: the display-list oracle (oracle DL1, SendMessage; the orchestrator gives the id). Worktree build3ds/wt/GSETR, branch GSETR.
PM_ROW=GSETR PM_LANE=GSETR on every bench call. Parallel with ws_DEADSRC (no shared file).
Objective (FACT, oracle dry run on main 6a987519c3: gen_gfx_tab.sh in docker, then gset.py on a copy of src and include): `gset moved 35 files 10 keep 0`,
listcen data 3577 -> 3285 (-292), data_arrays 352 -> 317 (-35), gfxtype 977 -> 941 (-36). C1 and C2 do not change.
Files: 3ds/tools/gconv_src.d/GSETR.txt (new), 3ds/tools/gset.d/GSETR.txt (new), and the files gset.py writes: 12 sources under src/ and their twins
under 3ds/tools/gfxin/src/. Do not edit any other file. Do not edit a source by hand.
Reader facts (oracle code read): the sprite record keeps rmList and the final list as values only (nd_sprite.c:9, :14; no reader of .rm or .fin in 3ds/src).
A G_DL to an NDN key runs from the device entry (nd_walk.c:231). Entity model lists are read through nd_lkey_ (3ds/tests/lkey/allow.txt: nd_ent.c, entity_model.c KEY).
## Edit 1: 3ds/tools/gconv_src.d/GSETR.txt (5 lines, LF)
`src/screen_overlays.c` `src/curtains.c` `src/sprite.c` `src/msg_draw.c` `src/global_hud_scripts.c` (one path per line).
## Edit 2: 3ds/tools/gset.d/GSETR.txt (11 lines, LF)
The 5 paths of Edit 1, then `src/entity/Shadow.c` `src/battle/entity_model_icons.c` `src/item_entity.c` `src/screen_render_util.c`
`src/filemenu/filemenu_gfx.c` `src/world/area_nok/nok_04/npc.c` (one path per line). Do not add src/world/area_kmr/kmr_21/main.c (ws_DEADSRC edits it).
Example before (src/sprite.c:77-85): `Gfx TranslucentSpriteGfx[] = {` .. `};`. After: one line `NDN_DECL(TranslucentSpriteGfx);`.
Example before (src/sprite.c:445): `rmList = ((u8) opacity == 255) ? OpaqueSpriteGfx : TranslucentSpriteGfx;`.
After: `rmList = ((u8) opacity == 255) ? NDN(OpaqueSpriteGfx) : NDN(TranslucentSpriteGfx);`. The new twin 3ds/tools/gfxin/src/sprite.c starts with `#include "src/sprite.c"`.
## Loop (1 build, at most 6 Azahar starts; base runs not charged)
1. Edit 1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/gsetr_host1.txt 2>&1` (gen_gfx_tab.sh writes build3ds/out/gset_ok.txt). Pass: `ND HOST TESTS PASS`,
   `gconv lists <n> ... bad 0`. Expected: 27 more lines in build3ds/out/gset_ok.txt than on main (FACT dry run: 1719 -> 1746; GUESS: 2 or 3 more
   unsup lines than the dry run, which also had hud_element.c and state_title_screen.c). Commit `GSETR 1: 5 root sources join the converter`.
2. Edit 2. `python 3ds/tools/gset.py 3ds/tools/gset.d/GSETR.txt build3ds/out/gset_ok.txt > build3ds/out/gsetr_gset.txt 2>&1`.
   Pass: `gset moved 35 files 10 keep 0` and `gset ok`. Another number: stop and send the output to the oracle.
3. Host test again (> build3ds/out/gsetr_host2.txt). Pass: `ND HOST TESTS PASS`, `lkey census ok`, conv2c line `ntab tables <n> draws <d> bad 0`.
   Commit `GSETR 2: gset moves 35 lists` (the set file, the 12 sources, the twins).
4. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0. `python 3ds/tools/listcen.py > build3ds/out/gsetr_lc.txt`. Pass: the 3 numbers of the objective.
5. Gate. For each of spot_mac01_drec, sbk14_drec, fxr_drec: `python 3ds/tools/bench.py base <b>`, then
   `python 3ds/tools/bench.py run <b> --row GSETR --why "GSETR gate"`, then `python 3ds/tools/bench.py cmp <b> --row GSETR --commit $(git merge-base HEAD main)`.
   Pass: 1/1 byte-equal each. A cold first fxr_drec cmp: one warm rerun (bench.py rule).
   NOT proven by bench (GUESS: no bench draws them): curtains, screen stencils, the message box list, battle cursor icons, file menu. The host compare (step 3) is their proof.
## Fix scope
The two set files only. A compile error in one source after step 2: remove its line from gset.d/GSETR.txt, `git checkout HEAD -- src 3ds/tools/gfxin`,
run step 2 again, and write the new moved count in the commit. A host compare fault (`bad` above 0) names a list: remove its source from both set files the same way.
## Stop and hand-back
Stop: all 3 gates pass. Final commit `GSETR 3: gate` with the values. That commit deletes this page.
Hand-back: build3ds/out/handback_GSETR.txt (line 1 commit hash, then gate values, gset moved, listcen totals, memcheck margin).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
## Addendum 2026-11-16 (oracle DL2, the designer to ask from now): curtains.c leaves gset.d only
Cause (FACT, code read): BLOCK_OPEN (gset.py:31) does not match `Gfx TheaterGfx[] ={` (src/curtains.c:143). A BLOCK_OPEN fix does not help:
`gSPDisplayList(gMainGfxPos++, &TheaterGfx)` (curtains.c:276) is a hold line (gset.py:160, HOLD_RULE gset.py:22), so TheaterGfx stays.
Its 6 callees move, and gset_check (nd_gfx_conv.c:917-920) fails. Do not edit gset.py.
1. Remove the line `src/curtains.c` from 3ds/tools/gset.d/GSETR.txt. Keep it in 3ds/tools/gconv_src.d/GSETR.txt.
2. Before step 2, run `python 3ds/tools/listcen.py > build3ds/out/gsetr_lc0.txt` (the base).
3. Step 2 pass: `gset moved 26 files 9 keep 0` (FACT: the 35 run moved 9 curtains lists, curtains.c:17-141; coder run gsetr_gset2.txt).
4. Step 4 pass, against gsetr_lc0.txt: data -212, data_arrays -26, gfxtype -27 (FACT: -292 + 80, -35 + 9, -36 + 9;
   listcen regexes on curtains.c:17-141 count 80 data, 9 arrays, 9 Gfx, no comments).
5. Write the new numbers in the commit texts. Curtains goes to a later row (it needs the `&X` sites at curtains.c:257, 276, 301).
