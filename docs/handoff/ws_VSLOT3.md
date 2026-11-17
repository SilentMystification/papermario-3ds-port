# ws_VSLOT3 (Sonnet, user yes 2026-11-17): mixed caller and own vertex slots in nd_vslot.c; D_8024B708 moves (C1 build -2, C2 gfxpos -2, C5 data -4, C6 data_arrays -1, C7 gfxtype -2)
Designer: the oracle (SendMessage). Worktree build3ds/wt/VSLOT3, branch VSLOT3 from main, PM_ROW=VSLOT3 PM_LANE=VSLOT3.
Main at release: 3ad3164cdd. Start: now, parallel with NMDL6 (no shared file). Edit only the 10 files of docs/handoff/ws_VSLOT3.patch:
3ds/tools/nd_vslot.c, nd_vslot.h, nd_vslot_test.c, nd_gfx_conv.c, nd_gtab_test.c, 3ds/include/nd_gtab.h, 3ds/tests/host.d/vslot.sh,
3ds/tools/gfxin/src/filemenu/filemenu_gfx.c, src/filemenu/filemenu_gfx.c, src/filemenu/filemenu_common.c.
## Objective
- FACT (build3ds/out/ntab_inherit.txt:216 on main): `D_8024B708: itab unsup vertex slots (nd_vslot_resolve) (unit 1 of D_8024B708)`. Its TRI2 (filemenu_gfx.c:25)
  uses slots 0-1 of D_8024B6F0 (caller) and its own slots 2-3. nd_vslot.c:36 refuses a triangle with 1 or 2 loaded slots. It is the only such list (same file).
- New form: that triangle is an ND_VS_MIX draw. A slot the list did not load is the ref {ND_VS_IN, slot}. nd_gfx_conv.c writes that ref into ndnv_<table> as
  `(const void*)(uintptr_t)<slot>u` (below 32). Readers of ndnv: only nd_gtab_test.c:471 (FACT grep: no 3ds/src reader of NDN_DRAW, the device runs NdNDev units).
- nd_gtab_test.c m_tri (the model walk) takes a mixed triangle: a slot that is not loaded gives base NULL, index = slot (as the all-caller case).
- D_8024B708 moves to the twin (the gset.py form of D_8024B6F0). filemenu_common.c:996 :1018 call `nd_draw_list(NDN(D_8024B708), ND_SELF)` as :987 does.
- FACT (oracle, main + this patch + ws_NMDL6.patch, PM_CACHE=off): ND HOST TESTS PASS, `vslot ok 10/10`, conv2c `ntab tables 1986 draws 36797 bad 0`,
  build3ds/out/ntab_inherit.txt has `D_8024B708: itab (unseeded table, its draws inherit the caller state)` and no `vertex slots` line.
- FACT (oracle listcen, main + patch): build 324 -> 322, gfxpos 22 -> 20, data 323 -> 319, data_arrays 9 -> 8, gfxtype 420 -> 418.
- FACT (azt_scenarios.json:14192): the only copy-arrow scene (cheatsave_copy) is not det. No byte scene reaches :996 :1018. OPEN: a console look by the user.
## Edit shapes (real code)
- nd_vslot.c:41. Before: `tarr = slot[s[0]].arr;` After: `tarr = slot[s[0]].loaded ? slot[s[0]].arr : ND_VS_MIX;`
- filemenu_common.c:996. Before: `gSPDisplayList(gMainGfxPos++, D_8024B708);` After: `nd_draw_list(NDN(D_8024B708), ND_SELF);`
- filemenu_common.c:167. Before: `extern Gfx D_8024B708[];` After: `NDN_DECL(D_8024B708);`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_VSLOT3.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. `PM_CACHE=off sh 3ds/tools/nd_host_test.sh > build3ds/out/vslot3_host.txt 2>&1`. With the cache on, nmdl0.sh can fail on a missing /tmp/ndcombs.c (FACT: a hostsel tool fault).
3. Commit the 10 files: `VSLOT3 1: mixed caller and own vertex slots (vslot 10/10); D_8024B708 moves, filemenu_common.c calls nd_draw_list`.
4. `python 3ds/tools/bench.py base wscmb_fsel_drec,spot_mac01_drec --row VSLOT3 > build3ds/out/vslot3_base.txt 2>&1` (base runs, not charged).
5. `sh 3ds/tools/rowloop.sh --row VSLOT3 --bench wscmb_fsel_drec,spot_mac01_drec --no-host --why "VSLOT3 gate" > build3ds/out/vslot3_loop.txt 2>&1`.
## Tests that decide
- vslot3_host.txt: `ND HOST TESTS PASS`, `vslot ok 10/10`, `ntab tables <n> draws <m> bad 0` (twice), `ntab seeded <k> bad 0`.
- `grep D_8024B708 build3ds/out/ntab_inherit.txt` prints the itab line above and no `unsup` line.
- `LOOP PASS`: build rc 0, both cmp lines byte-equal (base = merge-base of HEAD and main). The gate proves the regenerated tables change no other draw.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: build 322, gfxpos 20, data 319, data_arrays 8, gfxtype 418 (with NMDL6 merged: 315, 13, 319, 8, 417).
## What you may edit to fix a fail
Only the 10 files. A build error: fix the one line it names, 1 try. A cmp DIFF, a `bad` above 0 or a vslot FAIL: stop. Send the path of
build3ds/out/loop_VSLOT3_cmp_<bench>.txt or vslot3_host.txt to the oracle. Budget: 2 builds, 4 Azahar starts.
## Stop and hand-back
Last commit `VSLOT3 2: gate. wscmb_fsel_drec, spot_mac01_drec byte-equal; build 322 gfxpos 20 data 319` deletes ws_VSLOT3.md and ws_VSLOT3.patch.
One SubagentHandback: commit hashes, LOOP lines, listcen totals, the D_8024B708 line of ntab_inherit.txt.
