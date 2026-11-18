# ws_NDR2C (Haiku): HUD rectangles, complex HUD quads and the transition stencil take a core 1 pso snap (stage S4, measure; counter: none of C1-C12)
Designer: oracle FIN. WAIT: branch from main after the NDR2A merge (nd_dir_rec, nd_dir_drop, nd_dir_prim1, snapchk dir). Runs at the same time as NDR2B (disjoint files).
Worktree build3ds/wt/NDR2C, branch NDR2C. PM_ROW=NDR2C PM_LANE=NDR2C. Edit only: src/hud_element.c, src/screen_overlays.c (pm_tsten_run part), src/screen_render_util.c.
## Facts (main 136c960247; find each line by its text)
- hud_rect_direct (hud_element.c:471-547) makes the pso in its fmt switch (:484-518) and :526-528; case 0 reads the walker prim (`nd_direct_prim()`, :492). Its only caller is hud_rect_run (:554).
  The record site (:874-876) records `nd_rec_napi(hud_rect_run, &rec, 8u + 6u * (unsigned)rec.npc);` then hud_rect_feed.
- cplx_quad_run (:599-635) makes its pso from the record (:607-612); the record site (:1917) records `nd_rec_napi(cplx_quad_run, &rec, sizeof rec / 8u);` then cplx_quad_feed.
- pm_tsten_run (screen_overlays.c:83-118) makes its pso at :100-104 (`full` from u, :91); the record site screen_render_util.c:286 records `nd_rec_napi(pm_tsten_run, rec, 3u);`, prim = rec[4].
- Game files do not include nd_ds.h: core 1 reads the record-time prim with nd_dir_prim1() (NDR2A, nd_api.h).
## Edits (keep CRLF: the Edit tool)
1. hud_element.c: `static int hud_rect_direct(const NdHudRec* r)` becomes `static int hud_rect_direct(const NdHudRec* r, NdPsoN64* snap_pd, u32* snap_prim)` (NULL: core 0 draws; else core 1 takes the pso).
   Case 0: `: nd_direct_prim();` becomes `: (snap_pd ? nd_dir_prim1() : nd_direct_prim());`. Move the 3 lines `pd.cw0 = cw[0]; pd.cw1 = cw[1];`, `pd.omh |= filt ? ...;`, `pd.omh_m = ...;` up, below `wrap[2] = (unsigned)r->masks; ...`.
   Below them: `if (snap_pd) { *snap_pd = pd; *snap_prim = prim; return 0; } /* NDR2C: core 1 stops here */`. hud_rect_run calls `hud_rect_direct((const NdHudRec*)u, NULL, NULL)` and, after that line, `nd_dir_drop(); /* NDR2C */`.
   Record site before: `nd_rec_napi(hud_rect_run, &rec, 8u + 6u * (unsigned)rec.npc);` after:
   `{ NdPsoN64 pd; u32 prim; if (!hud_rect_direct(&rec, &pd, &prim)) nd_dir_rec(hud_rect_run, &rec, 8u + 6u * (unsigned)rec.npc, &pd, prim); else nd_rec_napi(hud_rect_run, &rec, 8u + 6u * (unsigned)rec.npc); } /* NDR2C */`.
2. hud_element.c above cplx_quad_run: `static void cplx_quad_pso(const NdCHudRec* r, NdPsoN64* pd)` = the static cw_47, the filt, aa, xlu lines and the 5 `pd.` lines of cplx_quad_run with `pd->`. cplx_quad_run calls `cplx_quad_pso(r, &pd);`
   in their place; delete its filt, aa, xlu lines (:603-604, then unused). Before its fallback block `{ static uint32_t buf[64];` add `nd_dir_drop(); /* NDR2C */`.
   Record site before: `nd_rec_napi(cplx_quad_run, &rec, sizeof rec / 8u);` after: `{ NdPsoN64 pd; cplx_quad_pso(&rec, &pd); nd_dir_rec(cplx_quad_run, &rec, sizeof rec / 8u, &pd, rec.prim); } /* NDR2C */`.
3. screen_overlays.c above pm_tsten_run: `void pm_tsten_pso(const uint32_t* u, NdPsoN64* pd)` = the 2 static cw arrays, `full` from u (`(int)(((const NdTStenRec*)u)->full_and_id & 1u)`) and the 5 `pd.` lines with `pd->`.
   pm_tsten_run calls `pm_tsten_pso(u, &pd);` in their place; `, full = ...` leaves the id line (:91). Before `tsten_emit(r, &nrp);` add `nd_dir_drop(); /* NDR2C */`.
4. screen_render_util.c: next to `extern void pm_tsten_run(...)` add `extern void pm_tsten_pso(const uint32_t* u, NdPsoN64* pd); /* screen_overlays.c, NDR2C */`.
   Before: `nd_rec_napi(pm_tsten_run, rec, 3u);` after: `{ NdPsoN64 pd; pm_tsten_pso(rec, &pd); nd_dir_rec(pm_tsten_run, rec, 3u, &pd, rec[4]); } /* NDR2C */`.
## Steps (output to build3ds/out/, read the last 20 lines; 1 build, 5 Azahar starts)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/ndr2c_host.txt 2>&1`: PASS. `grep -c "nd_dir_rec(" src/hud_element.c src/screen_render_util.c`: 2 and 1. Commit `NDR2C 1: HUD, complex HUD and stencil records take a pso snap`.
2. One call each: `python 3ds/tools/bench.py base <b>` and `sh 3ds/tools/rowloop.sh --row NDR2C --bench <b> --no-host` for b = axns_sam2_drec, wscmb_pause_drec, sten_enter_det.
3. One call each: `python 3ds/tools/bench.py run <s> --row NDR2C --why "NDR2C snapchk dir"` for s = nimgm_strip_snapchk, wscmb_pause_snapchk. `grep -a snapchk` of both logs > build3ds/out/ndr2c_snapchk.txt.
Gate (exact): host PASS; 3 benches byte-equal (FACT: the snap is a state op; core 0 still decides with nd_pso_decide); snapchk bad 0; wst not above the NDR2A hand-back. Report the dir counts (0 is not a fault).
Stop: a DIFF, a crash, a `dir` bad line, a tool fault: commit the outputs, send the paths to the oracle with SendMessage, wait. Fix rights: the listed files; compile errors.
Pitfalls: one host test at a time in a worktree (rowloop takes --no-host). PM_CACHE=off only as a prefix of one command, never exported. One bench per rowloop call. Each scene in its own call, no timeout option.
Keep each file's line endings (CRLF). A snap record is about 640 bytes (GUESS), the stream has 16 chunks of 32 KB per context (FACT nd_rec.c:30-31): the memo must stop a snap when the inputs stay the same.
Hand-back: last commit `NDR2C 2: gate. 3 benches byte-equal; snapchk bad <2>, dir <2>` deletes this page. One SubagentHandback: commit hash, gate values, dir counts.
