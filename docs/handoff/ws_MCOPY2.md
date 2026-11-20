# ws_MCOPY2 (Haiku): core 1 writes no model copy ranges; the 4 range words after the model snap go
Designer: oracle NATIVE16 (plan_perf.md 6a). Start after MCOPY1 is merged. Branch from main. Worktree build3ds/wt/MCOPY2, branch MCOPY2. PM_ROW=MCOPY2 PM_LANE=MCOPY2.
Why Haiku: a closed deletion list; after MCOPY1 no reader of the ranges is left (FACT: nd_msnap_ab had 2 readers, nd_recs.c:44 and :48, both gone).
Objective: nd_ds_model writes no alt range. Proxy (Azahar): drec end line `altw f<n> c1 m <k>` (pm_gpu.c:632): k 0 in the tip log (base: k > 0).
Console number to move: c1a m 10975 words per step to 0 (FACT capture 7569); c1t alt 8.2 ms and lgms 44.4 ms down (GUESS 2 to 4 ms together). c0gate 0 / 0 / 1.
Lines: main 01e0780881; grep the quoted text.
## Edits
1. nd_ds.c:461-495 nd_ds_model: delete the parameter `uint32_t* ab`, `dm_on` (:462), the 4 `nd_alt_lst` calls (:480, :483, :485), the 2 `nd_altw[0] +=`.
   Each of the 4 nd_alt_open and nd_alt_close calls (:468, :488, :490, :493) becomes `nd_rec_shreset();` at the same place (FACT: they reset the stream shadow,
   nd_rec.c:534-535 `st_hdr = NULL; sh_all();`; the pre range :489 records units through sh_drop). New in nd_rec.c: `void nd_rec_shreset(void) { rec_open();
   st_hdr = NULL; sh_all(); }`; a weak no-op twin in nd_ds.c next to :461 for the host units. dm_u (:463) before:
   `static void dm_u(NdDs* s, uint32_t w0, uint32_t w1, NdDsUnitFn unit) { if (!dm_on) { if (unit) unit(s, w0, w1); else nd_ds_apply(s, w0, w1); return; } ... }`
   after: `static void dm_u(NdDs* s, uint32_t w0, uint32_t w1, NdDsUnitFn unit) { if (unit) unit(s, w0, w1); else nd_ds_apply(s, w0, w1); }`.
   Keep the weak nd_alt_open, nd_alt_close, nd_alt_lst (:461) when another caller is left (grep); else delete them.
2. include/nd_ds.h:42: the prototype without `uint32_t* ab`.
3. nd_rec.c nd_rec_model (:224-238): `rec_take(... + 2u * ND_MSNAP_U + 4u)` loses `+ 4u`; the jump `j + 2u + 2u * ND_MSNAP_U + 4u` loses `+ 4u`; the nd_ds_model call
   loses its last argument. Comments `NAT5B: the 2 alt ranges follow the snap` go.
4. nd_msnap.c:33 nd_msnap_ab and include/nd_msnapw.h:13 go. nd_walk.c:416 nd_mdl_alt and nd_recs.c:17 its prototype go.
   nd_rec.c:445 nd_alt_lst, then nd_walk.c:413-414 nd_mls_run and nd_mle_run with their weak twins (nd_rec.c): delete each one that has no caller left (grep).
5. 3ds/tools/nd_ds_test.c:139 loses the last argument; the block around :152 that tests the alt range (`ab`) goes.
## Checklist (hw_guidelines.md 3, 6, 7): fewer stream words (about 44 KB per step, FACT 10975 words x 4 B); no new code. PICA: no change.
## Tests (build3ds/out/, last 20 lines). Gate: 1 build, 2 Azahar runs
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. Commit `MCOPY2 1: no model copy ranges`.
2. Build; c0gate 0 / 0 / 1; memcheck OK. `bench.py base <b> --commit $(git merge-base HEAD main)`, then run and cmp spot_mac01_drec and fxparts_drec: byte-equal
   (core 0 ran no copy since MCOPY1). altw `c1 m 0` in both tip logs.
Fix rights: compile errors; the host test lines of item 5. A DIFF, a crash, `c1 m` not 0: stop, send the output path to oracle NATIVE16.
Stop: last commit `MCOPY2 2: gate` deletes this page. Hand-back: one SubagentHandback: hashes, host, c0gate, the 2 cmp lines, altw c1 m base and tip.
