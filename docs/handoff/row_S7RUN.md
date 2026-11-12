# Row S7RUN: the table runner (core 0) and nd_rec_dl (core 1): a static list call drawn from its NdGTab (NATIVEAPI S7, design core)

Model: Sonnet (pre-approved for the night run).
- Why not Haiku: the runner must give the walker's result for a G_DL of a static list. The walker routes such a call through
  three arms (op_ctl NDO_CALL: ent_gdl for entity-like lists, dlc_try for compiled map shapes, else nd_ops_run with the sync state
  kept; nd_walk.c:186-189 and the CALL arm). The runner must choose per table the same arm or prove equality, and keep the S6E
  value-order rule. These are choices inside shared core 0 code.
- Why not Opus: the table form and the def rule exist (plan_nativeapi.md section 6, nd_gtab.h, S6E ent_gap).
- Why not smaller: the S7EFX call-site rows (Haiku) all use this one API; it must exist and be proven first.

## Start
`sh 3ds/tools/wt_new.sh S7RUN`. PM_ROW=S7RUN PM_LANE=S7RUN on every call. Base: `git merge-base HEAD main` (main >= 07d0d9104).

## Facts (oracle 5, main 3cef2c284)
- nd_gtab.h: NdGTab {list, op[nop], flags, nvtx, ntri}; ops NDG_VTX, NDG_TRI, NDG_DEF (NdGDef: host-analysed NdSDef st 1,
  inherit, ctx, k0), NDG_UNIT (pointer to the typed unit), NDG_CALL (callee table), NDG_END. nd_gtab_find(list): core 1 lookup.
- S6E (merged 4678251d3) proved the value-order hazard: sc_exec (nd_scomp.c:202-221) runs value units after the first static
  unit behind the folded block and after the decision; ent_gap compiles only when `d->nv == d->pre` (spot_mac00_det went from
  0/6 to 6/6 with that rule). A table def with nv > pre must run its units as ops (nd_walk_state on d.u, d.n units).
- nd_part_draw(const NdGTab*) already draws a parts table (S7CONV); its ring and epoch handling is the model for NDG_VTX/NDG_TRI.

## Design (fixed API; the coder chooses the inner steps)
1. Core 1, 3ds/src/nd_gtab.c (end) and nd_gtab.h (end):
   `void nd_rec_dl(const void* list);` the replacement of `gSPDisplayList(gMainGfxPos++, list)` at call sites: the table of
   list (nd_gtab_find, with a memo of the last 8 lookups) is recorded as one NDC_NAPI record {runner nd_gtab_run, the table
   pointer}; no table: it writes the gSPDisplayList unit as before (counted). The branch form (gSPBranchList) is not handled
   (call sites keep it).
2. Core 0: `void nd_gtab_run(const uint32_t* rec);` runs the ops in order with nd_ops_run's list start and end (gv_epoch, dlc_on,
   gp_prev_op, pm_walk_side, the streak flushes):
   - NDG_DEF: nd_scomp_exec_own(&g->d) when `g->d.nv == g->d.pre`, else nd_walk_state(g->d.u, g->d.n);
   - NDG_UNIT: nd_walk_state(p, 1);
   - NDG_VTX / NDG_TRI: the walker's load and triangle path (nd_geom_vtx with the load word rebuilt from a, b, p; draw_tri with
     the tri word; tri_prep_ok as op_geo), so clip, cull, lit loads and the ring stay the walker's;
   - NDG_CALL: the callee table (recursion depth limit as ND_LIST_DEPTH; at the limit run callee->list with nd_ops_run);
   - a table whose list the walker sends to ent_gdl or dlc_try today (check the walker CALL arm: the tests it makes on the list
     address): the runner calls the same arm with t->list instead of walking ops, so those lists keep their native path. Name
     the rule in the commit.
3. Census: one line per 300 frames `gtabrun f<n> tab <t> dl <d> arm <ent>/<dlc>/<ops>` (records with a table, nd_rec_dl
   fallbacks to the list word, and the arm counts). pm_log_evt, so prof mode prints it.
4. First callers (to prove it): the setup list calls of the 6 S7CONV effects in src/effects (sparkles/pink_sparkles,
   effect_3D, snowfall, snaking_static, water_block, embers): each `gSPDisplayList(gMainGfxPos++, D_...)` of a list in
   GCONV_SRC becomes `nd_rec_dl(D_...)` under `#ifdef TARGET_3DS` (`#else` keeps the line). The S7EFX rows then do the rest.

5. Merge lines for the later rows: in 3ds/tools/gen_gfx_tab.sh, right after the `GCONV_SRC="..."` line, add 11 lines
   `GCONV_SRC="$GCONV_SRC" # S7EFX1` ... `# S7EFX8`, `# S7ENT`, `# S7MAP`, `# S7MDL`. Each later row fills only its own line, so
   rows that run together do not conflict.

## Files and places
3ds/src/nd_gtab.c (end), 3ds/include/nd_gtab.h (end), 3ds/src/nd_walk.c (the runner next to nd_parts_run_v; no other function),
the 6 effect files (their setup list call lines only), 3ds/tools/gen_gfx_tab.sh (the 11 placeholder lines). No row running now edits
these places.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS (gtab compare unchanged: the tables are not changed by this row).
- `bench.py base fxpic_63`, `bench.py base fxpic_124`; one list `bench.py run fxpic_63,fxpic_124 --row S7RUN --why "S7RUN: setup
  lists from tables"` (2 starts; both use the changed runner and nd_rec_dl). cmp: every shot byte-equal.
- Census (its own edits only): the gtabrun line of fxpic_63 shows tab > 0 and dl 0 for the frames with the effect (the 6
  effects' setup calls all have tables). spot_mac01_det unchanged is not needed (no shared caller outside the 6 files).
- Budget 8 Azahar starts.

## Stop
A byte difference or dl > 0 in the effect frames: stop, commit, hand back (2 tries per named cause). Delete this row file in
the completing commit. Hand back the final API names (the S7EFX rows use `nd_rec_dl(list)`).
