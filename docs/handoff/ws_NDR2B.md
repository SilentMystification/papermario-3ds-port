# ws_NDR2B (Haiku): the 3 texture rectangle producers take a core 1 pso snap (nd_trect_rec); snapchk dir compares it at nd_direct_bind (stage S4, measure; counter: none of C1-C12)
Designer: oracle FIN. WAIT: branch from main after the NDR2A merge (nd_snap_pso, nd_snapchk_dir, nd_dir_rec, nd_dir_drop, the host stubs). Runs at the same time as NDR2C (disjoint files).
Worktree build3ds/wt/NDR2B, branch NDR2B. PM_ROW=NDR2B PM_LANE=NDR2B. Edit only: 3ds/src/nd_direct.c (the trect part), 3ds/include/nd_api.h, src/draw_img_util.c, src/msg.c, src/state_title_screen.c.
## Facts (main 136c960247; find each line by its text)
- The 3 producers record `nd_rec_napi(nd_trect_run, ...)` and then `nd_trect_feed(...)`: draw_img_util.c:169-170 (`&r`), msg.c:2256-2257 (`&r`), state_title_screen.c:522-523 (`t`, in ttl_run).
- nd_trect_run (nd_direct.c:336-362) states the pso `{ .cw0 = r->cw0, .cw1 = r->cw1, .oml = r->oml, .omh = r->omh, .omh_m = (3u << G_MDSFT_TEXTLUT) | (3u << G_MDSFT_TEXTFILT) }` (:350) and the prim
  `(r->pad & 1u) ? nd_direct_prim() : r->prim` (:352). nd_direct_prim is nd_ws.prim (:94); the core 1 copy is nd_ds.w.prim. The direct path needs nd_direct_ready() == 1, else the units walk (:359-361).
## Edits (keep CRLF: the Edit tool)
1. nd_direct.c above nd_trect_run: `static void trect_pso(const NdTRect* r, NdPsoN64* pd) { pd->cw0 = r->cw0; pd->cw1 = r->cw1; pd->oml = r->oml; pd->omh = r->omh; pd->omh_m = (3u << G_MDSFT_TEXTLUT) | (3u << G_MDSFT_TEXTFILT); } /* NDR2B: both cores */`.
   nd_trect_run: before `const NdPso id = nd_pso_n64(&pd);` the line `const NdPsoN64 pd = { ... };` becomes `NdPsoN64 pd; trect_pso(r, &pd);`.
   In nd_trect_run, before `if (nfb++ < 16u) pm_log_d("trect fallback f%u", pm_frame_count());` add `nd_dir_drop(); /* NDR2B: the walk takes no pso snap */`.
2. nd_direct.c below nd_trect_feed, core 1:
   `void nd_trect_rec(const NdTRect* r) { NdPsoN64 pd; trect_pso(r, &pd); nd_dir_rec(nd_trect_run, r, sizeof *r / 8u, &pd, (r->pad & 1u) ? nd_ds.w.prim : r->prim); nd_trect_feed((const uint32_t*)r); } /* NDR2B */`
3. nd_api.h after nd_trect_feed: `void nd_trect_rec(const NdTRect* r); /* nd_direct.c, core 1, NDR2B: pso snap, record (nd_rec_napi_s) and feed of one texture rectangle */`.
4. The 3 producers. Before (msg.c:2256-2257): `nd_rec_napi(nd_trect_run, &r, sizeof r / 8u);` and `nd_trect_feed((const uint32_t*)&r);`. After: `nd_trect_rec(&r); /* NDR2B */`. draw_img_util.c the same; state_title_screen.c `nd_trect_rec(t);`.
## Steps (output to build3ds/out/, read the last 20 lines; 1 build, 5 Azahar starts)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/ndr2b_host.txt 2>&1`: PASS. `grep -c "nd_rec_napi(nd_trect_run" src/draw_img_util.c src/msg.c src/state_title_screen.c`: 0 each.
   Commit `NDR2B 1: texture rectangle records take a pso snap`.
2. One call each: `python 3ds/tools/bench.py base <b>` and `sh 3ds/tools/rowloop.sh --row NDR2B --bench <b> --no-host` for b = curt_title_drec, wscmb_fsel_drec, wscmb_pause_drec.
3. One call each: `python 3ds/tools/bench.py run <s> --row NDR2B --why "NDR2B snapchk dir"` for s = nimgm_strip_snapchk, wscmb_pause_snapchk. `grep -a snapchk` of both logs > build3ds/out/ndr2b_snapchk.txt.
Gate (exact): host PASS; 3 benches byte-equal (FACT: the snap is a state op; core 0 still decides with nd_pso_decide); snapchk bad 0; wst not above the NDR2A hand-back. Report the dir counts (0 is not a fault).
Stop: a DIFF, a crash, a `dir` bad line, a tool fault: commit the outputs, send the paths to the oracle with SendMessage, wait. Fix rights: the listed files; compile errors.
Pitfalls: `bench.py run` does not build. Run `PM_LANE=NDR2B sh 3ds/build.sh` after each code change, before step 3 (FACT NDR0G2: STALE BUILD exit 5). Run every command in the foreground. One host test at a time in a worktree (rowloop takes --no-host). PM_CACHE=off only as a prefix of one command, never exported. One bench per rowloop call. Each scene in its own call, no timeout option.
Keep each file's line endings (CRLF). A snap record is about 640 bytes (GUESS), the stream has 16 chunks of 32 KB per context (FACT nd_rec.c:30-31): the memo must stop a snap when the inputs stay the same.
Hand-back: last commit `NDR2B 2: gate. 3 benches byte-equal; snapchk bad <2>, dir <2>` deletes this page. One SubagentHandback: commit hash, gate values, dir counts.
