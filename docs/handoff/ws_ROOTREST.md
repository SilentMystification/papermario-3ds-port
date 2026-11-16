# ws_ROOTREST (Haiku): 12 gbi call sites in root and world files become nd_* calls (C1 build -12, C2 gfxpos -12)
Designer: the oracle (SendMessage). Worktree build3ds/wt/ROOTREST, branch ROOTREST. PM_ROW=ROOTREST PM_LANE=ROOTREST. Start now.
No shared file with NMDL0, GFXINC2, MAINBIN, VINE, DLCCHK (FACT: git diff --name-only of each branch, oracle 2026-11-16).
## Objective and counters
- FACT (codemod dry, main d644c38ba8, fresh ntab_tables.txt of build 0bfe6ce6f6): 5 list calls keep `no-entry` only because codemod.py dl_key
  (codemod.py:192) does not expand N(x). Each list has a v2 table (ntab_tables.txt): kmr_21_Gfx_TexSetup_TitleImage, nok_04_ThreadGfx,
  iwa_01_splash_gfx, dro_02_card_setup_gfx, D_8021CF40_5A3620. They take ND_SELF, as 142 sites on main do.
- FACT (code read): 7 state calls have an nd_* form with the same words: nd_val_fog (nd_tx.h:32), nd_tex_load_tile (nd_tx.h:81; the syncs of the
  macro are dropped by nd_gx_w, nd_rec.c:468), nd_tex_tile (nd_tx.h:16-18), nd_xf_viewport (nd_tx.h:34-35), nd_pso_scis (nd_st.c:67, mode 0),
  nd_rect_tex (nd_tx.h:53; nd_gx_w gives the same nd_rec_texrect, nd_rec.c:462-466).
- FACT (listcen BUILD_RX, listcen.py:45): each site is 1 build and 1 gfxpos. Expected: build -12, gfxpos -12. Other totals: 0 change.
## Files (all in docs/handoff/ws_ROOTREST.patch). Edit only these 10.
src/background.c, src/background_gfx.c, src/hud_element.c, src/inventory.c (+ `#include "nd_st.h"`), src/sprite.c, src/world/area_kmr/kmr_21/main.c,
src/battle/area/kzn2/actor/lava_piranha.c, src/world/area_iwa/iwa_01/splash.c, src/world/area_nok/nok_04/npc.c, src/world/area_dro/dro_02/npc_merlee.c.
## Edit shapes (the patch does them; use them to check a reject)
Before (src/world/area_nok/nok_04/npc.c:287): `gSPDisplayList(gMainGfxPos++, NDN(N(ThreadGfx)));`
After: `nd_draw_list(NDN(N(ThreadGfx)), ND_SELF);`
Before (src/inventory.c:751): `gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, a, b, c, d) /* PT58: ... */;`
After: `nd_pso_scis(G_SC_NON_INTERLACE, a, b, c, d); /* PT58: ... */`
Before (src/sprite.c:225): `gSPViewport(gMainGfxPos++, vpAlt);` After: `nd_xf_viewport(vpAlt);`
Rule: remove the first argument `gMainGfxPos++`. Keep every other argument in the same order.
## Loop (1 build, 2 Azahar starts; base runs not charged)
1. `python 3ds/tools/listcen.py > build3ds/out/rootrest_lc0.txt`.
2. `tr -d '\r' < docs/handoff/ws_ROOTREST.patch | git apply -`. A reject: stop, send the output to the oracle.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/rootrest_host.txt 2>&1`. Read the last 20 lines. Pass: `ND HOST TESTS PASS`, `lkey census ok`.
4. `python 3ds/tools/listcen.py > build3ds/out/rootrest_lc1.txt`. Pass vs rootrest_lc0.txt: total build -12, gfxpos -12, no other total line changes.
   Commit the 10 files: `ROOTREST 1: 12 root and world gbi sites take nd_* calls (5 ND_SELF lists, 7 state calls)`.
5. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0.
6. `python 3ds/tools/bench.py base spot_mac01_drec` and `python 3ds/tools/bench.py base evar_iwa01_drec`.
7. Gate, for each bench B of step 6: `python 3ds/tools/bench.py run B --row ROOTREST --why "ROOTREST gate"`, then
   `python 3ds/tools/bench.py cmp B --row ROOTREST --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
   Coverage: mac_01 draws the status bar (inventory.c:751, GUESS hud_element.c:2164); iwa_01 entry 0 (GUESS: the splash of splash.c:69).
## Fail
- A failing test or a compile error: you may edit only the 10 files. Compare each with the patch. 2 tries for one cause.
  Then write the output to build3ds/out/ and send the path to the oracle.
- A base run that fails or hangs, or a byte diff in the gate: stop. Send the log or cmp output path to the oracle.
## Stop
The gate passes. The last commit `ROOTREST 2: gate` deletes ws_ROOTREST.md and ws_ROOTREST.patch.
## Hand-back (one SubagentHandback message)
Commits. listcen build and gfxpos before and after. memcheck margin. The 2 cmp lines.
