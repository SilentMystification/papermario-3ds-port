# ws_PBUF (Haiku): effect part buffers become nd_at.h units (C1 build -30, C6 data_arrays -7, C7 gfxtype -7)
Designer: the oracle (SendMessage). Worktree build3ds/wt/PBUF, branch PBUF. PM_ROW=PBUF PM_LANE=PBUF. Start now.
No shared file with MODELRM, GMERLEE, FXWRAP, WSCMB, EVAR. NMDL0 (later) also adds lines to 3ds/include/nd_at.h: a merge keeps both.
## Objective and counters
- FACT (code read): 7 effects fill a local `Gfx g[n]` with gbi macros and pass it to nd_parts_add (nd_rec.c:306). For a local packet
  nd_gx_w writes the typed unit (nd_rec.c:455). The nd_at.h and nd_tx.h O forms with ND_NR_AT (nd_nr.h:11) write the same 2 words.
- FACT (oracle host run, 3ds/tests/host.d/pbuf.sh): 6 shapes x 9 values, `pbuf 54 cases bad 0`. The seeded fault fails. The 7 files pass gcc -fsyntax-only.
- FACT (oracle listcen run, main eaf3865490 with the patch): build 518 -> 488 (-30), data_arrays 182 -> 175 (-7), gfxtype 794 -> 787 (-7).
  gfxpos, data, recdl, reader, model: 0. Bench difference: none (the unit words are equal).
## Files (all in docs/handoff/ws_PBUF.patch). Edit only these 12.
3ds/include/nd_at.h (new NDCTL_CALL), src/effects/{effect_3D,embers,pink_sparkles,shimmer_burst,snaking_static,snowfall,water_block}.c,
new: 3ds/tests/pbuf/pbuf.c, 3ds/tests/host.d/pbuf.sh, 3ds/tools/azt.d/PBUF.json, 3ds/tools/bench.d/PBUF.json.
## Edit shape (the patch does it; use it to check a reject)
Before (src/effects/embers.c:222-228):
`Gfx g[5];` `gSPMatrix(&g[0], M, F);` `gDPSetTileSize(&g[1], T, A, B, C, D);` `gSPDisplayList(&g[3], L);` `gSPPopMatrix(&g[4], G_MTX_MODELVIEW);`
`nd_parts_add(&pb, (const uint32_t*)g);`
After: `uint32_t g[10], *nrp = g;` `NDXF_PUSH(ND_NR_AT, M, F);` `NDTX_TSZ(ND_NR_AT, T, A, B, C, D);` `NDCTL_CALL(ND_NR_AT, L);`
`NDXF_POP(ND_NR_AT);` `nd_parts_add(&pb, g);`. gDPSetPrimColor(&g[k], 0, 0, r, g, b, a) -> `NDVAL_PRIM(ND_NR_AT, 0, 0, r, g, b, a);`.
The buffer size doubles (n Gfx -> 2n uint32_t). Each file gets `#include "nd_at.h"` after nd_api.h, embers and snaking_static also nd_tx.h.
## Loop (1 build, 1 Azahar start; base runs not charged)
1. `python 3ds/tools/listcen.py > build3ds/out/pbuf_lc0.txt`.
2. `tr -d '\r' < docs/handoff/ws_PBUF.patch | git apply -`. A reject: stop, send the output to the oracle.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/pbuf_host.txt 2>&1`. Read the last 20 lines. Pass: `pbuf ok`, `ND HOST TESTS PASS`.
4. `python 3ds/tools/listcen.py > build3ds/out/pbuf_lc1.txt`. Pass vs pbuf_lc0.txt: total build -30, data_arrays -7, gfxtype -7,
   no other total line changes. Commit the 12 files: `PBUF 1: effect part buffers are nd_at.h units; pbuf host test; pbuf_fx_drec bench`.
5. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0.
6. `python 3ds/tools/bench.py base pbuf_fx_drec`. Pass: the base log has `drec f676 draws`.
7. Gate: `python 3ds/tools/bench.py run pbuf_fx_drec --row PBUF --why "PBUF gate"`, then
   `python 3ds/tools/bench.py cmp pbuf_fx_drec --row PBUF --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal.
   Coverage (FACT, 3ds/src/pm_fxtest.c FXLIST order, SLOT 44 at :205): fxtest 50 shimmer_burst, 55 effect_3D, 63 snowfall,
   81 snaking_static, 84 water_block, 105 embers, 124 pink_sparkles; drec frames 412 to 676 step 44 (copy of fxp_drec, PMOVE).
## Fail
- A failing test or a compile error: you may edit only the 12 files. Compare each with the patch. 2 tries for one cause.
  Then write the output to build3ds/out/ and send the path to the oracle.
- A base run that fails or hangs, or a byte diff in the gate: stop. Send the log or cmp output path to the oracle.
## Stop
The gate passes. The last commit `PBUF 2: gate` deletes ws_PBUF.md and ws_PBUF.patch.
## Hand-back (one SubagentHandback message)
Commits. `pbuf` line. listcen build, data_arrays, gfxtype before and after. memcheck margin. The cmp line.
