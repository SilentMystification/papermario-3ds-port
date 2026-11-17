# ws_FLTGT (Haiku): the flame target lists move (colour image unit); flame.c:190 and :197 to ND_SELF (C1 -2, C2 -2, C5 -49, C6 -2)
Designer: oracle DESIGN2 (SendMessage). Worktree build3ds/wt/FLTGT, branch FLTGT from main, PM_ROW=FLTGT PM_LANE=FLTGT.
Main at release: 3ad3164cdd. Start: now, parallel with ws_OVLIMG (no shared file). Model: Haiku. The patch is complete (no design step).
Edit only the 5 files of docs/handoff/ws_FLTGT.patch: 3ds/tools/nd_gfx_conv.c, 3ds/tools/gfxin/src/effects/gfx/flame.c,
src/effects/flame.c, src/effects/gfx/flame.c, new 3ds/tests/host.d/fltgt.sh.
## Objective
- FACT (src/effects/gfx/flame.c:36, :47): D_090009E0_354590 unit 0 is gsDPSetColorImage(&D_800B3AE0), unit 15 loads &D_800B3AE0.
  FACT (nd_gfx_conv.c:440-457, :532-534): the converter refused unit 15 (no symbol) and unit 0 (NDO_MISC). D_09000800_3543B0 unit 12 the same.
- The patch gives the twin buffers D_800B32E0 and D_800B3AE0 an initializer, so the RAW rule (gen_gfx_syms.py:21) maps them as GS_IMG.
  v2_unit_add names the image of an NDO_MISC_CIMG unit. v2_walk keeps that unit as a unit only (switch CIMG_UNIT).
- FACT (code): the device runs the unit by nd_ntab_run (nd_walk.c:600), then gbi_setcimg (nd_walk.c:346), the same setter as the list word.
- The 2 blocks move by hand from src/effects/gfx/flame.c to the twin (gset.py include mode moves only #include lines).
- FACT (oracle host run, main + patch): `fltgt ok`, `gconv lists 1939 ... bad 0 moved 1938`, both lists in ntab_tables.txt and gset_ok.txt.
- Counters FACT (oracle listcen, main + patch): build 324 -> 322, gfxpos 22 -> 20, data 323 -> 274, data_arrays 9 -> 7, gfxtype 420 -> 416.
## Edit shapes (src/effects/flame.c)
- :190 before `        gSPDisplayList(gMainGfxPos++, D_090009E0_354590);` after `        nd_draw_list(NDN(D_090009E0_354590), ND_SELF);`
- :197 before `    gSPDisplayList(gMainGfxPos++, preset->dlist);` after `    nd_draw_list(preset->dlist, ND_SELF);`
- :19 before `extern Gfx D_09000800_3543B0[];` after `NDN_DECL(D_09000800_3543B0);` (:31 :39 :47 :55 `.dlist = NDN(D_09000800_3543B0),`).
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_FLTGT.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 5 files: `FLTGT 1: colour image unit (CIMG_UNIT, fltgt.sh); D_09000800, D_090009E0 move; flame.c:190 :197 ND_SELF`.
3. `python 3ds/tools/bench.py base fxops_drec --row FLTGT > build3ds/out/fltgt_base.txt 2>&1` (base runs, not charged).
4. `sh 3ds/tools/rowloop.sh --row FLTGT --bench fxops_drec --why "FLTGT gate" > build3ds/out/fltgt_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, the cmp line byte-equal (base = merge-base of HEAD and main).
- `grep -h "fltgt ok" build3ds/out/loop_FLTGT_host.txt` prints 1 line.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: build 322, gfxpos 20, data 274, data_arrays 7, gfxtype 416.
- FACT (azt.d/FXOPS.json:8, pm_fxtest.c:60): fxops_drec runs fxtest 27, the flame (FX_FLAME_RED), on mac_01.
  FACT (flame.c:186): :190 runs once per frame with a live flame. :197 runs once per flame.
## What you may edit to fix a fail
Only the 5 files. Host log line `FAIL hostsel store (read trace)` with `WinError 5` (hostsel.py:123) and every unit ok: run step 4
once more (FACT: it came once in the oracle dry run. GUESS: a store race on the shared build3ds/hostsel). A second time: stop, send the host log path to the oracle.
A line `gconv: moved ...` or a DIFF: stop. Send build3ds/out/loop_FLTGT_host.txt or loop_FLTGT_cmp_fxops_drec.txt to the oracle.
Budget: 2 builds, 4 Azahar starts.
## Stop and hand-back
Last commit `FLTGT 2: gate. fxops_drec byte-equal; build 322 gfxpos 20 data 274` deletes ws_FLTGT.md and ws_FLTGT.patch.
One SubagentHandback: commit hashes, LOOP lines, listcen totals.
