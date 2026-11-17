# ws_ENTSELF (Haiku, row ENTSELF2): 6 list calls of self-contained v2 tables become nd_draw_list(X, ND_SELF) (C1 build -6, C2 gfxpos -6)
Designer: the oracle (SendMessage). Worktree build3ds/wt/ENTSELF2, branch ENTSELF2 from main, PM_ROW=ENTSELF2 PM_LANE=ENTSELF2.
Main at release: 52f9b15c42. Start: now. Do not use the old branch ENTSELF (it holds old NTABFIND commits). Edit only the 7 files of the patch.
## Objective
- FACT (code): ND_SELF is the null entry (3ds/include/nd_gtab.h:88). nd_draw_list(X, ND_SELF) is nd_rec_dl(X) (nd_gtab.c:136).
- FACT (ENTSELF 66cea2: wscmb_pause_drec crashed on base and head). FACT (NTABFIND e19e9fe0c8, merged): the cause was the imgfx
  NdNDev address in nd_rec_ndev (nd_gtab.c:138 now records a copy). So pause_gfx.c:425 goes to ND_SELF again.
- Counters (FACT, oracle listcen on main + patch): build 341 -> 335, gfxpos 44 -> 38. Other totals as main.
## Design (docs/handoff/ws_ENTSELF.patch, 7 files; FACT: `git apply --check` OK on 52f9b15c42)
item_entity.c:1131 :1133, effects/flower_splash.c:166, flower_trail.c:187, bombette_breaking.c:241, pause/pause_gfx.c:425,
new 3ds/tools/azt.d/ENTSELF.json and bench.d/ENTSELF.json (bench entself_fx_drec: fxtest=3,4,60, drec=412:3:44).
FACT (ENTSELF branch 66cea2): entself_fx_drec ran 1/1 byte-equal, 343 draws. Its bench lines work.
## Edit shape (flower_splash.c:166). Before: `            gSPDisplayList(gMainGfxPos++, dlist);`
After: `            nd_draw_list(dlist, ND_SELF);`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_ENTSELF.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 7 files: `ENTSELF2 1: 6 list calls of v2 tables to nd_draw_list ND_SELF, bench entself_fx_drec`.
3. `python 3ds/tools/bench.py base entself_fx_drec > build3ds/out/entself2_base.txt 2>&1`. A base fail is a tool fault: stop (rule 4).
4. `sh 3ds/tools/rowloop.sh --row ENTSELF2 --bench entself_fx_drec,wscmb_pause_drec,spot_mac01_drec --why "ENTSELF2 gate"
   > build3ds/out/entself2_loop.txt 2>&1`.
5. Seeded fault (self-test of the new bench): in flower_splash.c:164 change `112, 96, 24` to `113, 96, 24` (no commit). Run
   `sh 3ds/tools/rowloop.sh --row ENTSELF2 --bench entself_fx_drec --no-host > build3ds/out/entself2_fault.txt 2>&1`.
   The cmp must say DIFF. Then `git checkout src/effects/flower_splash.c`.
## Tests that decide
- `LOOP PASS` in step 4: host PASS, build rc 0, 3 cmp lines byte-equal.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: build 335, gfxpos 38, recdl 11.
- GUESS: wscmb_pause_drec draws a pause label (pause_stats.c:295 calls pause_draw_menu_label). The byte compare decides.
## What you may edit to fix a fail
Only the 7 files. A DIFF of the real edit, a crash or a base fail: stop. Send build3ds/out/loop_ENTSELF2_cmp_<bench>.txt or
the run log path to the oracle. Budget: 2 builds (edit, seeded fault), 6 Azahar starts.
## Stop and hand-back
Last commit `ENTSELF2 2: gate. entself_fx_drec, wscmb_pause_drec, spot_mac01_drec byte-equal; seeded fault DIFF; build 335 gfxpos 38`
deletes ws_ENTSELF.md and ws_ENTSELF.patch. One SubagentHandback: commit hashes, LOOP lines, listcen totals, fault cmp line.
