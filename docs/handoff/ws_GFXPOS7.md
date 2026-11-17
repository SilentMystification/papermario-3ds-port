# ws_GFXPOS7 (Haiku): the gMainGfxPos halves of 2 stream compares and draw_box.c:462 go (C2 gfxpos -5, C7 gfxtype -3)
Designer: the oracle (SendMessage). Worktree build3ds/wt/GFXPOS7, branch GFXPOS7 from main, PM_ROW=GFXPOS7 PM_LANE=GFXPOS7.
Main at release: 358070e292. Start: now, parallel with THEATVP and BASEKEEP2 (no shared file). Edit only the 8 files of
docs/handoff/ws_GFXPOS7.patch: src/msg.c, src/msg_draw.c, src/filemenu/filemenu_msg.c, src/draw_box.c, 3ds/src/nd_st.c,
3ds/include/nd_st.h, 3ds/tools/nd_st_test.c, 3ds/tests/host.d/stunit.sh.
## Objective
- FACT (code): gMainGfxPos is nd_rec_top at every top-level point. nd_gx_w takes each packet back (nd_rec.c:458), rec_open resets a
  stray word (nd_rec.c:210, :206). So `gMainGfxPos != D_80151338` (msg_draw.c:334) and `gMainGfxPos != filemenu_savedGfxPos`
  (filemenu_msg.c:184) are false after the first message or char. Only the nd_rec_pos() half decides.
- D_80151338 becomes `const u32*` and holds nd_rec_pos(). clear_printers (msg.c:193) still sets it to nullptr: the next message resets.
- draw_box.c:462 calls the new nd_pso_unit(x0, x1) (nd_st.c). It drops ND_NOP_A as nd_gx_w does (nd_rec.c:468), else nd_nr.
- Counters (FACT, oracle listcen on main + patch): gfxpos 29 -> 24, gfxtype 424 -> 421. build 326, data 339 do not move.
- FACT (oracle host run, main 358070e292 + this patch + THEATVP + BASEKEEP2): ND HOST TESTS PASS, `stunit ok 20/20` (new shapes
  unit and unop), seeded fault `stunit FAIL unop` seen.
## Edit shapes
- draw_box.c:462. Before: `            nd_gx_w(gMainGfxPos++, cw->w0, cw->w1);` After: `            nd_pso_unit(cw->w0, cw->w1);`
- msg_draw.c:334. Before: `    if (gMainGfxPos != D_80151338 || nd_rec_pos() != msgStreamPos) {` After: `    if (nd_rec_pos() != D_80151338) {`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_GFXPOS7.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 8 files: `GFXPOS7 1: stream compares without gMainGfxPos, nd_pso_unit for draw_box.c:462 (stunit 20/20)`.
3. `python 3ds/tools/bench.py base wscmb_pause_drec,wscmb_fsel_drec --row GFXPOS7 > build3ds/out/gfxpos7_base.txt 2>&1`
   (base runs, not charged).
4. `sh 3ds/tools/rowloop.sh --row GFXPOS7 --bench wscmb_pause_drec,wscmb_fsel_drec --why "GFXPOS7 gate" > build3ds/out/gfxpos7_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, both cmp lines byte-equal (base = merge-base of HEAD and main).
- `grep -h "stunit ok" build3ds/out/loop_GFXPOS7_host.txt` prints `stunit ok 20/20`.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: gfxpos 24, gfxtype 421.
- GUESS: wscmb_pause_drec runs draw_msg (pause text) and draw_box; wscmb_fsel_drec runs filemenu chars and the ND_CMB_NOP styles
  (filemenu_styles.c:54). The byte compare decides.
## What you may edit to fix a fail
Only the 8 files. A build error: fix the one line it names, 1 try. A DIFF or a stunit FAIL: stop. Send the path of
build3ds/out/loop_GFXPOS7_cmp_<bench>.txt or loop_GFXPOS7_host.txt to the oracle. Budget: 2 builds, 4 Azahar starts.
## Stop and hand-back
Last commit `GFXPOS7 2: gate. wscmb_pause_drec, wscmb_fsel_drec byte-equal; gfxpos 24 gfxtype 421` deletes ws_GFXPOS7.md and
ws_GFXPOS7.patch. One SubagentHandback: commit hashes, LOOP lines, listcen totals.
