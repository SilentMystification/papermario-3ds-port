# ws_C: N64 branches (C3, C4), then the state calls of the root src files (C1, C2)

Model: Haiku (tools edit; the setters in group 3 are one-line bodies). Design: docs/handoff/design_state_calls.md.
Counters (FACT, listcen at b2442aa97): C3 build_n64 272, C4 gfxpos_n64 280. Root state calls 899, root value calls 325 (codemod --dry).
## Group 0: N64DEL2 (worktree build3ds/wt/N64DEL2, branch N64DEL2, head 24c44cd17)
- 24c44cd17 already commits the 55 edited files (row step 3 is done). Do not merge main (the row diffs against 6fce31e6f).
- Run row_N64DEL2.md "Exact edits" 2, 4, 5, 6, then its "Gate" 1-4. Expected: 7 skip lines, total blocks 151, diff -r empty,
  listcen build_n64 165 and gfxpos_n64 163. The 14 .inc.c differences of the last try are out: step 4 lists no .inc.c file.
- Commit, delete row_N64DEL2.md, hand the hash to the orchestrator (merge).
## Group 1: N64DEL3, group 2: N64DEL4 (rows exist; each a new branch from main after the group before merged)
- Run row_N64DEL3.md, then row_N64DEL4.md, exactly as written. After group 2: C3 0 and C4 0. Shape (N64DEL2, src/pause/pause_gfx.c:629):
  Before: `#ifdef TARGET_3DS` / <3DS lines> / `#else` / `    return gPauseMessages[index];` / `#endif`  After: <3DS lines> only.
## Group 3: the setters (new branch WSC from main after group 2 merged; `sh 3ds/tools/wt_new.sh WSC`)
- New 3ds/include/nd_st.h, 3ds/src/nd_st.c with the setters of the design table. Shape (as nd_val.c on CODEMODW1). Before
  (nd_gbi.h:22): `#define gSPSetOtherMode(pkt, cmd, sft, len, data) ND_GX(pkt, ND_W0_SOM(cmd, sft, len), (unsigned int)(data))`.
  After (nd_st.c): `void nd_pso_cyc(unsigned t) { const uint32_t w0 = ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_CYCLETYPE, 2), w1 = t;`
  `nd_rec_state(ND_X0(w0, w1), ND_X1(w0, w1)); }` (the sft and len of each setter: its gbi.h line, design table).
- New 3ds/tools/nd_st_test.c and 3ds/tests/host.d/stunit.sh: copy nd_val_test.c and hunit.sh. One case per setter (fixtures from
  the root sites, for example draw_img_util.c:21 G_CYC_1CYCLE, :26 G_RM_TEX_EDGE, G_RM_TEX_EDGE2, :27 G_CC_DECALRGBA twice).
  Seeded fault: a copy of nd_st.c with one shift changed must print `stunit FAIL <name>`. Pass: `stunit ok <n>/<n>`.
## Group 4: codemod --state (same branch). codemod.py: STATE_MAP (name -> native, argument indexes) used only with `--state`.
- One case pair per new name in 3ds/tests/codemod/cases (in.c, out.c, log). run.sh PASS with its seeded fault.
## Groups R1-R6 (design order). Per group: codemod with `--state` and the value map on the group's files only.
- Shape (draw_img_util.c:21, :26-27). Before: `gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);`
  `gDPSetRenderMode(gMainGfxPos++, G_RM_TEX_EDGE, G_RM_TEX_EDGE2);` `gDPSetCombineMode(gMainGfxPos++, G_CC_DECALRGBA, G_CC_DECALRGBA);`
  After: `nd_pso_cyc(G_CYC_1CYCLE);` `nd_pso_rm(G_RM_TEX_EDGE, G_RM_TEX_EDGE2);` `nd_pso_cc(G_CC_DECALRGBA, G_CC_DECALRGBA);`
  plus `#include "nd_st.h"` after the last include (and nd_val.h, nd_xf.h when used).
- Loop: listcen before, codemod --dry (v = value + state rewritten), real run, includes, nd_host (hunit, stunit), codemod run.sh,
  1 build (memcheck margin 310 or more), listcen build and gfxpos = before - v exactly, 1-2 drec cmp runs: R1 spot_mac01_drec,
  R2 spot_mac01_drec and gold_12_drec, R3-R6 spot_mac01_drec and sbk14_drec. Commit per group: `WSC R<n>: ...`.
## What you may edit to fix a failing test
Groups 0-2: what their row names. Group 3-4: nd_st.h, nd_st.c, nd_st_test.c, stunit.sh, codemod.py STATE_MAP, codemod cases.
R groups: include lines only. A cmp or listcen difference: do not commit the group, write build3ds/out/wsc_<g>.txt, send it to the
oracle, go on with the next group. Never edit a src subdirectory file (ws_A and ws_B own them).
## Stop and hand-back
Hand back after group 0 (merge), after group 2, and after R6. Readers (C8) and recdl (C10) get the next page from the oracle.
Hand-back (8 lines): hash, gate values, listcen C1 C2 C3 C4, memcheck line. Details: build3ds/out/handback_WSC.txt.
