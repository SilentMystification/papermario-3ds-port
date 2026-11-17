# ws_ENTSELF (Haiku): 6 list calls of self-contained v2 tables become nd_draw_list(X, ND_SELF) (C1 -6, C2 -6)
Designer: the oracle (SendMessage). Worktree build3ds/wt/ENTSELF, PM_ROW=ENTSELF PM_LANE=ENTSELF. Main at release: 020b846d3f.
Start: now. Files: src/item_entity.c, src/effects/flower_splash.c, flower_trail.c, bombette_breaking.c, src/pause/pause_gfx.c.
No other page edits them.
## Objective
- FACT (build3ds/out/ntab_tables.txt of the post-merge build 43b35ab465): every list these sites can call is a self-contained v2
  table: Gfx_RM2_ALPHATEST, Gfx_RM1_ALPHATEST, D_09000E08_32DCC8, D_09000E20_32DCE0, D_09000E38_32DCF8, D_090017D0_37F1A0
  (gen_ntab_entries names() of the site argument), and the 9 PauseGfx* labels of PauseGfxLabels (pause_gfx.c:382-392).
- FACT (code): ND_SELF is the null entry (nd_gtab.h:88), so nd_draw_list(X, ND_SELF) is nd_rec_dl(X) (nd_gtab.c:132): the CONV2F form.
- Counters (FACT, oracle listcen on the patched tree): build 386 -> 380, gfxpos 89 -> 83, recdl 11 (no change).
## Design (all in docs/handoff/ws_ENTSELF.patch, 7 files; edit only these)
item_entity.c:1131 and :1133, flower_splash.c:166, flower_trail.c:187, bombette_breaking.c:241, pause_gfx.c:425, new
3ds/tools/azt.d/ENTSELF.json and bench.d/ENTSELF.json (scenario and bench entself_fx_drec: fxtest=3,4,60, drec=412:3:44,
until `drec f500 draws [1-9]\d* bytes \d+`; the line is pm_gpu.c:618; the fxtest ids are FXLIST rows 3, 4, 60 of pm_fxtest.c).
FACT: `git apply --check` OK on 020b846d3f. Each file already has `#include "nd_gtab.h"`.
## Edit shape (flower_splash.c:166). Before: `            gSPDisplayList(gMainGfxPos++, dlist);`
After: `            nd_draw_list(dlist, ND_SELF);`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_ENTSELF.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 7 files: `ENTSELF 1: 6 list calls of v2 tables to nd_draw_list ND_SELF, bench entself_fx_drec`.
3. `python 3ds/tools/bench.py base entself_fx_drec > build3ds/out/entself_base.txt 2>&1` (the new bench needs its base).
4. `sh 3ds/tools/rowloop.sh --row ENTSELF --bench entself_fx_drec,wscmb_pause_drec,spot_mac01_drec --why "ENTSELF gate"
   > build3ds/out/entself_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, the 3 cmp lines byte-equal. GUESS: wscmb_pause_drec draws the HP label (pause_stats.c:295).
- `python 3ds/tools/listcen.py | grep "^listcen total"`: build 380, gfxpos 83, recdl 11. Other totals as main.
- Seeded fault (new bench self-test): in flower_splash.c:164 change `112, 96, 24` to `113, 96, 24` (uncommitted), rebuild with
  `sh 3ds/tools/rowloop.sh --row ENTSELF --bench entself_fx_drec --no-host`: the cmp must say DIFF. Undo it (git checkout the file).
## What you may edit to fix a fail
Only the 7 files. A base run that fails or a DIFF of the real edit: stop. Send build3ds/out/loop_ENTSELF_cmp_<bench>.txt to the
oracle. Budget: 2 builds (edit, seeded fault), 6 Azahar starts.
## Stop and hand-back
Last commit `ENTSELF 2: gate. entself_fx_drec, wscmb_pause_drec, spot_mac01_drec byte-equal; seeded fault DIFF; build 380 gfxpos 83`
deletes ws_ENTSELF.md and the patch. One SubagentHandback: commit hashes, the LOOP lines, the listcen totals, the fault cmp line.
