# ws_NDR4 (Haiku): a sprite record takes a snap; pm_gpu_spr_build reads env, prim, other mode, geometry, combine words and mix from it (stage S4; counter: none of C1-C12)
Designer: oracle FIN. WAIT: branch from main after the NDR2A merge (NDR1C nd_snap_live, NDR0G2 pm_drec_now). Runs at the same time as NDR2B and NDR2C (disjoint files). NDR2D waits for this row.
Worktree build3ds/wt/NDR4, branch NDR4. PM_ROW=NDR4 PM_LANE=NDR4. Edit only: 3ds/src/nd_rec.c (3 lines), 3ds/src/nd_walk.c (1 line), 3ds/src/nd_snap.c (end), 3ds/include/nd_snap.h, 3ds/src/pm_gpu.c (pm_gpu_spr_build).
## Facts (main 136c960247; find each line by its text)
- sprite.c records the sprite state (spr_state_run record and its feed, or the flat units) and then nd_rec_spr (nd_rec.c:277, sprite.c:467): at nd_rec_spr nd_ds holds the sprite draw state.
- stream_run clears the arm at every command but JUMP, TEXRECT and the snap-taking NAPI records (nd_rec.c:685): NDC_SPR clears it before nd_walk_spr runs (:691).
- nd_walk_spr (nd_walk.c:169-182) draws natively (pm_gpu_spr_native) or walks the 2 geometry words (draw_tri: the armed snap path of NDR0E2-NDR0G2 is right there too: the same nd_ds state).
- pm_gpu_spr_build (pm_gpu.c:4575-4637) reads the walker: sticky geo and combine words (:4595, pm_gbi_sticky nd_comb.c:282), env and prim (:4609), omode_h and omode_l (:4611), the SHROUD mix of nd_ws.omode_l with fogc and blendc (:4612-4618). nd_snap_mix (nd_snap_in.c:61) is that mix on the snap words (FACT: the NDR0F1 host case). prim_lod, om_kh, om_kl, tss and pm_gbi_gv_at stay walker reads (not in the snap).
## Edits (keep CRLF: the Edit tool)
1. nd_rec.c: above nd_rec_spr `void nd_snap_mesh(void); /* NDR4: strong in nd_snap.c */`; first line of nd_rec_spr `nd_snap_mesh(); /* NDR4: the snap of the sprite draw */`.
   stream_run: `op != NDC_JUMP && op != NDC_TEXRECT &&` (the arm line) becomes `op != NDC_JUMP && op != NDC_TEXRECT && op != NDC_SPR &&`.
2. nd_walk.c nd_walk_spr: `if (sh) { if (__builtin_expect(dbg & WD_CEN, 0)) walk_dbg_nat(); return; }` becomes
   `if (sh) { extern unsigned char nd_snap_arm; nd_snap_arm = 0; /* NDR4: the native sprite took its snap */ if (__builtin_expect(dbg & WD_CEN, 0)) walk_dbg_nat(); return; }`.
3. nd_snap.c at the end (core 0), with `static unsigned sc_sfr = ~0u;`:
   ```
   int nd_snap_spr(uint32_t o[9]) { /* NDR4: env prim omh oml geom cw0 cw1 mixk mixc of the live snap; 0 when none */
       if (!nd_snap_live) return 0;
       o[0] = sc_cur.in.env; o[1] = sc_cur.in.prim; o[2] = sc_cur.in.omode_h; o[3] = sc_cur.oml; o[4] = sc_cur.geom;
       o[5] = sc_cur.in.w0; o[6] = sc_cur.in.w1; o[7] = (uint32_t)sc_cur.mixk; o[8] = sc_cur.mixc;
       if (pm_drec_now() && pm_frame_count() != sc_sfr && (o[0] != nd_ws.envc || o[1] != nd_ws.prim || o[2] != nd_ws.omode_h || o[3] != nd_ws.omode_l ||
           o[4] != nd_ws.geom || o[5] != nd_ws.comb_w0 || o[6] != nd_ws.comb_w1 || sc_cur.fogc != fogc || sc_cur.blendc != blendc)) { sc_sfr = pm_frame_count(); pm_log_d("sprsnap f%u", sc_sfr); }
       return 1;
   }
   ```
   nd_snap.h: declare it.
4. pm_gpu.c pm_gpu_spr_build. After `pm_gbi_sticky(&sw_tss, &sw_geo, &sw_cw0, &sw_cw1);` (:4595) add
   `uint32_t sv[9]; const int sn = nd_snap_spr(sv); /* NDR4 */` and `if (sn) { sw_geo = sv[4]; sw_cw0 = sv[5]; sw_cw1 = sv[6]; }`; above the function `int nd_snap_spr(uint32_t o[9]); /* nd_snap.c, NDR4 */`.
   :4609 `c.env = nd_ws.envc; c.prim = nd_ws.prim;` becomes `c.env = sn ? sv[0] : nd_ws.envc; c.prim = sn ? sv[1] : nd_ws.prim;` (the rest of the line stays).
   :4611 `c.base_oh = nd_ws.omode_h; c.base_ol = nd_ws.omode_l;` becomes `c.base_oh = sn ? sv[2] : nd_ws.omode_h; c.base_ol = sn ? sv[3] : nd_ws.omode_l;`.
   Before the SHROUD block `{ /* SHROUD: ...` add `if (sn) { c.r_mix = (int)sv[7]; c.mixcol = sv[7] ? sv[8] : r_mixcol; } else /* NDR4: the mix of the snap */` so the block is its else body.
## Steps (output to build3ds/out/, read the last 20 lines; 1 build, 5 Azahar starts)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/ndr4_host.txt 2>&1`: PASS. Commit `NDR4 1: sprite records take a snap; the sprite build reads it`.
2. One call each: `python 3ds/tools/bench.py base <b>` and `sh 3ds/tools/rowloop.sh --row NDR4 --bench <b> --no-host` for b = spot_mac01_drec, axns_sam2_drec, nimgm_strip_drec.
   For a DIFF bench: `python 3ds/tools/drecsum.py <b> --row NDR4`, `grep -a "sprsnap\|drec frame" build3ds/azt/logs/<b>.txt`.
3. One call each: `python 3ds/tools/bench.py run <s> --row NDR4 --why "NDR4 snapchk"` for s = nimgm_strip_snapchk, wscmb_pause_snapchk. `grep -a snapchk` of both logs > build3ds/out/ndr4_snapchk.txt.
Gate (exact): host PASS; 3 benches byte-equal (GUESS: the snap words equal the walker words at a sprite); snapchk bad 0; wst not above the NDR2A hand-back.
Stop: a DIFF (state its first DIFF draw, frame and the sprsnap frames), a crash, a bad line, a tool fault: commit the outputs, send the paths to the oracle with SendMessage, wait. A DIFF in a sprsnap frame is a stale walker: only the user can allow an expected-diff gate for this row. Fix rights: the listed files; compile errors.
Pitfalls: one host test at a time in a worktree (rowloop takes --no-host). PM_CACHE=off only as a prefix of one command, never exported. One bench per rowloop call. Each scene in its own call, no timeout option.
Keep each file's line endings (CRLF). A snap record is about 640 bytes (GUESS), the stream has 16 chunks of 32 KB per context (FACT nd_rec.c:30-31): the memo must stop a snap when the inputs stay the same.
Hand-back: last commit `NDR4 2: gate. 3 benches byte-equal; snapchk bad <2>` deletes this page. One SubagentHandback: commit hash, gate values, sprsnap frames if any.
