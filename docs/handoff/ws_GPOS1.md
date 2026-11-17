# ws_GPOS1 (Haiku): the task start without gMainGfxPos and without list parameters (C2 gfxpos -4, C7 gfxtype -2)
Designer: oracle DESIGN3 (SendMessage). Worktree build3ds/wt/GPOS1, branch GPOS1 from main, PM_ROW=GPOS1 PM_LANE=GPOS1.
Edit only src/main.c, src/main_loop.c, src/cam_main.c, include/nu/nusys.h, 3ds/src/pm_nusys.c, 3ds/src/game_stubs.c.
Shares no file with IMGFXT, MDLTYPE, BARRICADE. Model: Haiku (line deletes, one signature).
## Objective and counters
- FACT (code read, main c9540957ad): gMainGfxPos is nd_rec_top at all times. nd_gx_w takes each packet back (nd_rec.c:458).
  stray_word resets it (nd_rec.c:206). So the 3 task-start assignments write the value that the pointer already has.
- FACT: nuGfxTaskStart gets nullptr and 0 from all 3 callers (main.c:186, main_loop.c:183, :302). The size goes only to fields
  that no code reads (pm_nusys.c:161, :164, :166; key_size is "log only", pm_nusys.c:79).
- Move (FACT, listcen rules): gfxpos -4 (main.c 1, main_loop.c 2, cam_main.c 1). gfxtype -2 (cam_main.c:29 -1, nusys.h:790 -1,
  pm_nusys.c:151 -1, game_stubs.c +1). build, data, recdl: no change.
## Edits (line numbers are main c9540957ad; match the text)
1. main.c:151, main_loop.c:178, main_loop.c:195: delete the line `gMainGfxPos = nd_rec_task(); /* DIRECT D9: ... */`.
2. cam_main.c:29: delete `Gfx* gMainGfxPos;`.
3. game_stubs.c: before `void pm_game_bind(void) {` add the line
   `Gfx* gMainGfxPos; /* GPOS1: the packet of the redirected gbi macros (nd_gbi.h); pm_game_bind sets it once (was cam_main.c) */`.
   Keep game_stubs.c:12 `gMainGfxPos = nd_rec_task();`. The host tests define their own gMainGfxPos: do not put it in nd_rec.c.
4. nusys.h:790 -> `extern void nuGfxTaskStart(u32 ucode, u32 flag);`.
5. pm_nusys.c:151 -> `void nuGfxTaskStart(u32 ucode, u32 flag) {`. Delete :159 `(void)gfxList; ...`.
   :161 `key_size[nkey++] = gfxListSize;` -> `key_size[nkey++] = 0u;`. :164 and :166 `.n = gfxListSize;` -> `.n = 0u;`.
6. The 3 callers drop the first 2 arguments. Example, main_loop.c:302:
   before `nuGfxTaskStart(nullptr, 0, NU_GFX_UCODE_F3DEX2, NU_SC_TASK_LODABLE | NU_SC_SWAPBUFFER); /* DIRECT D9: ... */`
   after  `nuGfxTaskStart(NU_GFX_UCODE_F3DEX2, NU_SC_TASK_LODABLE | NU_SC_SWAPBUFFER); /* DIRECT D9: ... */`
   Same form at main.c:186 and main_loop.c:183. Keep each comment.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edit. `git grep -n "gMainGfxPos\|nuGfxTaskStart(nullptr" src/main.c src/main_loop.c src/cam_main.c` prints nothing.
2. Commit `GPOS1 1: task start without gMainGfxPos writes and list parameters (nuGfxTaskStart(ucode, flag))`.
3. `python 3ds/tools/bench.py base spot_mac01_drec --row GPOS1 --commit $(git merge-base HEAD main)` (base run, not charged).
4. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row GPOS1 --bench spot_mac01_drec --why "GPOS1 gate" > build3ds/out/gpos1_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, the cmp line of spot_mac01_drec byte-equal. Budget: 1 build, 2 Azahar starts.
- `python 3ds/tools/listcen.py | grep -E "^listcen total (gfxpos|gfxtype) "` on HEAD and on `$(git merge-base HEAD main)`:
  gfxpos 4 lower, gfxtype 2 lower. Any other difference fails.
## Fix rights: only the 6 files. A compile error, a host FAIL, a cmp DIFF or a counter move that differs: stop.
  Write the output path to the oracle (SendMessage DESIGN3) and wait for the reply.
## Stop and hand-back
Last commit `GPOS1 2: gate. spot_mac01_drec byte-equal; gfxpos <n> gfxtype <m>` deletes this page.
One SubagentHandback: hashes, LOOP line, the two counter pairs.
