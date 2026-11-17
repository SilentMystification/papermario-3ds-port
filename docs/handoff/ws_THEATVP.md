# ws_THEATVP (Haiku): a Vp symbol kind for the converter; TheaterInitGfx moves, 2 sites to ND_SELF (C1 -2, C2 -2, C5 -16, C6 -1)
Designer: the oracle (SendMessage). Worktree build3ds/wt/THEATVP, branch THEATVP from main, PM_ROW=THEATVP PM_LANE=THEATVP.
Main at release: 358070e292. Start: now, parallel with GFXPOS7 and BASEKEEP2 (no shared file). Edit only the 7 files of
docs/handoff/ws_THEATVP.patch: 3ds/tools/nd_gconv.h, 3ds/tools/gen_gfx_syms.py, 3ds/tools/nd_gfx_conv.c, new
3ds/tests/host.d/vpsym.sh, new 3ds/tools/gset.d/THEATVP.txt, 3ds/tools/gfxin/src/curtains.c, src/curtains.c.
## Objective
- FACT (build3ds/out/ntab_unsup.txt of a main host run, 2026-11-17 02:44): `TheaterInitGfx: unit address outside the symbol map (unit 0)`. Unit 0 is
  gsSPViewport(&TheaterViewport) (curtains.c:60). nd_gfx_conv.c:440 looked up every MOVEMEM as GS_LIT. No Vp was in the map.
- The patch adds kind GS_VP (nd_gconv.h:4), the `Vp NAME =` rule VP (gen_gfx_syms.py, seeded VP_RULE), the viewport lookup in
  v2_unit_add, a unit-only arm in the v2 op walk (as NDO_SCISSOR) and `extern Vp` in the table file. TheaterInitGfx is the only list
  with gsSPViewport (FACT grep src).
- The patch holds the output of `python 3ds/tools/gset.py 3ds/tools/gset.d/THEATVP.txt build3ds/out/gset_ok.txt` plus the 2 sites.
- FACT (oracle host runs, main 358070e292 + this patch + GFXPOS7 + BASEKEEP2): `vpsym ok`, `gconv lists 1939 ... bad 0 moved 1935`
  (main 1934), ntab_unsup.txt has only the 2 flame lists, ND HOST TESTS PASS.
- Counters (FACT, oracle listcen): build 326 -> 324, gfxpos 29 -> 27, data 339 -> 323, data_arrays 10 -> 9, gfxtype 424 -> 423.
- FACT (code): MOVEMEM is a state op for both record paths (nd_gtab.c:47, nd_rec.c:397 st_kind). The old site was inlined by
  rec_inline (nd_rec.c:399). The new site is a state-only key with ND_SELF (the INITGS form).
## Edit shape (curtains.c:133). Before: `        gSPDisplayList(gMainGfxPos++, TheaterInitGfx);`
After: `        nd_draw_list(NDN(TheaterInitGfx), ND_SELF);` (curtains.c:177 the same).
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_THEATVP.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 7 files: `THEATVP 1: GS_VP symbol kind (vpsym), TheaterInitGfx to a device key (gset.d THEATVP), 2 sites ND_SELF`.
3. `python 3ds/tools/bench.py base curt_title_drec --row THEATVP > build3ds/out/theatvp_base.txt 2>&1` (base runs, not charged).
4. `sh 3ds/tools/rowloop.sh --row THEATVP --bench curt_title_drec --why "THEATVP gate" > build3ds/out/theatvp_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, the cmp line byte-equal (base = merge-base of HEAD and main).
- `grep -c "^TheaterInitGfx$" build3ds/out/gset_ok.txt` prints 1. `grep -h "vpsym ok" build3ds/out/loop_THEATVP_host.txt` prints 1 line.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: build 324, gfxpos 27, data 323, data_arrays 9.
- GUESS: curt_title_drec runs curtains.c:133 (the CURT gate scene). FACT (code): curtains.c:177 runs only without controller 1
  (curtains.c:160). Azahar has one. Its proof is the host run (same key, ND_SELF).
## What you may edit to fix a fail
Only the 7 files. A build error that names TheaterViewport or Vp in nd_gfx_tab.c: send the build log path to the oracle, no edit.
A DIFF: stop. Send build3ds/out/loop_THEATVP_cmp_curt_title_drec.txt to the oracle. Budget: 2 builds, 4 Azahar starts.
## Stop and hand-back
Last commit `THEATVP 2: gate. curt_title_drec byte-equal; build 324 gfxpos 27 data 323` deletes ws_THEATVP.md and
ws_THEATVP.patch. One SubagentHandback: commit hashes, LOOP lines, listcen totals.
