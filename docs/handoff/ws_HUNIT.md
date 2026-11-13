# ws_HUNIT: one host test per codemod value call shape (macro stream vs native record)

Model: Haiku. Worktree build3ds/wt/CODEMODW1, branch CODEMODW1 (it holds the nd_val.c fix 03d1bfe66 and the water_gfx.c codemod).
PM_ROW=HUNIT PM_LANE=HUNIT. First step: `git merge main` (orchestrator order; runs.jsonl conflict: CLAUDE coder answers).
## Objective and counters
- New host test 3ds/tools/nd_val_test.c + 3ds/tests/host.d/hunit.sh. It prints `hunit ok 7/7` or `hunit FAIL <shape>`.
- Counter: none moves here (C1 -3, C2 -3 of water_gfx.c come with the merge, FACT row_CODEMODW1.md:45). It unblocks ws_A.
## Facts (main b2442aa97)
- Macro path: nd_gbi.h:18 ND_GX -> nd_rec.c:442 nd_gx_w -> :459 nd_rec_state (syncs dropped at :457). Native path on main:
  nd_val.c:12-15 and nd_xf.c:11-19 call nd_rec_napi (an NDC_NAPI record). nd_model.c:296 `default: return 0;` rejects NDC_NAPI in a
  custom gfx range (nd_r11_walk :274). A range is nd_rec_mark .. nd_rec_end (src/model.c:4314-4317).
- The 7 shapes (codemod.py:37-43): gSPMatrix, gSPPopMatrix, gDPPipeSync (removed), gDPSetPrimColor, gDPSetEnvColor, gSPSegment, gDPSetTileSize.
## Edit 1 (nd_xf.c, same shape as CODEMODW1 nd_val.c). Before (nd_xf.c:11-14):
  `const uint32_t u[2] = { NDO_W0(NDO_MTX, 0x380000u | ((p ^ 1u) & 0xffu)), (uint32_t)(uintptr_t)m };` / `nd_rec_napi(nd_xf_run, u, 1u);`
  After: `nd_rec_state(NDO_W0(NDO_MTX, 0x380000u | ((p ^ 1u) & 0xffu)), (uint32_t)(uintptr_t)m);` and `#include "nd_cmd.h"`.
  Same for nd_xf_pop (:17-19). Delete nd_xf_run (nd_xf.c:9, nd_xf.h:11). Readers: only nd_xf_test.c (grep nd_xf_run: no other).
## Edit 2 (nd_xf_test.c:26-30 test double). Before: `void nd_rec_napi(NdNapiRun run, const void* rec, unsigned units) { ... memcpy(last, rec, units * 8u); nlast = units * 2u; }`
  After: `void nd_rec_state(uint32_t w0, uint32_t w1) { last[0] = w0; last[1] = w1; nlast = 2u; }`. Keep the nd_walk_state double (:31).
## Edit 3 (new nd_val_test.c). Link: nd_val_test.c nd_val.c nd_xf.c nd_rec.c nd_model.c nd_op.c nd_mtx.c nd_tmpl.c /tmp/ndcomb.o
  /tmp/ndcombs.c /tmp/ndlogs.c (built earlier in the same docker block, nd_host_test.sh:35, :59-65). Flags: hunit.sh = nxf.sh flags + $FP.
- Test doubles in nd_val_test.c (link errors name them; add or drop only doubles here): gMainGfxPos, gCurrentDisplayContextIndex,
  ModelRenderModes, gMapShapeData, __end__, pm_log, pm_log_e, pm_log_evt, pm_fatal_stop, pm_debug_has, pm_dbg_gen, is_host,
  nd_gtab_find, nd_mkey_a, nd_lstcen, nd_lc_frame, nd_walk_rec_model, nd_walk_spr, nd_walk_ent, nd_texrect_run, nd_parts_run,
  nd_parts_run_v, pm_interp_repl, pm_interp_find, f32tof24 (copy nd_model_test.c:10-14). nd_walk_state double: append each unit to a log.
- Include "ultra64.h" then "nd_gbi.h" so the macros take the redirect. Set `gMainGfxPos = nd_rec_task();` before each case.
- Per shape: A = mark=nd_rec_mark(); macro calls; nd_rec_end(). B = the same with the native calls. Compare (1) the stream words from
  mark to the NDC_END word, (2) the unit log of nd_stream_run(mark) (stream_run stops at NDC_END), (3) nd_r11_state(mark, ...) result,
  cw, has_cb, nmo, mo. Fixtures (real sites): prim water_gfx.c:14 `0,0,0,0,128,0` twice in a row (shadow drop); env
  attack_result_text.c:212 with 10,20,30,0; seg :187 `0x09, 0x00123450`; push :202 with p 0..7 and m 0x00123450; pop :208;
  tile size aura.c:351 with v1=5, v2=7; pipesync: macro `prim, gDPPipeSync, env` vs native `prim, env`.
## Before and after of a codemod site (src/world/area_mac/mac_00/water_gfx.c:14-15, done on the branch)
  Before: `gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 128, 0);` `gDPPipeSync(gMainGfxPos++);`  After: `nd_val_prim(0, 0, 0, 0, 128, 0);`
## Loop and gate (1 build, 2 Azahar runs plus the stored base pair)
1. Seeded fault in hunit.sh: `sed 's/NDO_ENV, 0u/NDO_PRIM, 0u/' 3ds/src/nd_val.c > /tmp/nd_val_seed.c`, link it: must print
   `hunit FAIL env`. Then the real link: `hunit ok 7/7`. `sh 3ds/tools/nd_host_test.sh > build3ds/out/host.txt 2>&1`: ND HOST TESTS PASS (nxf ok 9/9).
2. `sh 3ds/tests/codemod/run.sh`: PASS. Build: memcheck margin 310 or more.
3. row_CODEMODW1.md gate steps 2-4 (seed blue 128 to 64 must differ; spot_mac00w_det and fxpic_105 byte-equal).
## Edit only to fix a failing test: nd_val.c, nd_xf.c, nd_xf.h, nd_xf_test.c, nd_val_test.c, hunit.sh. No other file.
## Stop: 2 tries per named cause, then build3ds/out/hunit_fail.txt to the oracle. Hand-back (8 lines): hash, `hunit ok 7/7`,
cmp results, memcheck line. Commit per edit. The completing commit deletes ws_HUNIT.md and row_CODEMODW1.md.
