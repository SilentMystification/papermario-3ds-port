# ws_NDR1C (Haiku): every texture rectangle takes a snap; its render state, combine, binds and tile words come from the live snap (stage S4; counter: none of C1-C12; expected-diff gate, user 2026-11-18 option A)
Designer: oracle FIN. WAIT: branch from main after the NDR0G2 merge (nd_bsrc, ND_B* macros, nd_snap_bnew, pm_drec_now, the `drec frame` log, sc_count).
Worktree build3ds/wt/NDR1C, branch NDR1C. PM_ROW=NDR1C PM_LANE=NDR1C. Edit only: 3ds/src/nd_rec.c, 3ds/src/nd_snap.c, 3ds/include/nd_snap.h, 3ds/src/nd_rect.c.
## Facts (main 136c960247; NDR0G2 does not touch nd_rect.c or nd_rec.c, so their line numbers hold; find each line by its text)
- nd_rec_texrect takes the snap only with snapchk (nd_rec.c:311). In a drec run no rectangle is armed: the NDR1B combine apply (nd_rect.c:241) never ran in a drec gate.
- nd_rec_cmds (nd_rec.c:661) does not count NDC_TEXRECT, and stream_run (:685) keeps the arm over TEXRECT: a run of rectangles with the same inputs takes one snap (memo nd_snap.c:51).
  Only the first rectangle after a snap is armed (nd_rect.c:221 clears it). So the later ones need a flag that a draw does not clear: nd_snap_live, cleared with the arm at :685.
- nd_texrect renders at nd_rect.c:205, :237, :238, :242 and binds at :245-248. Tile words: :190-191 (copy_clamp), :201-202 (zprim), :245, :252-253, :280-281. box_pattern_rect (:243, :109-157) stays walker.
- zcol (nd_rect.c:236) is a pure rule of the target and oml: `siz == 2 && (kind & (NDK_ZERO | NDK_INZ)) && !(oml & (1u << G_MDSFT_ZSRCSEL))`. nd_tgt_role takes it as zc (nd_tgt.h:27).
## Edits. Do the nd_rect.c seds first (main line numbers), then the manual edits. Keep CRLF: `sed -b -i`, the Edit tool.
1. nd_rect.c: with E of ws_NDR0G2 edit 1: `sed -b -i -E "188,207{$E}; 245,286{$E}" 3ds/src/nd_rect.c` (FACT dry run: 10 lines). Then `sed -b -i "205s/sync_render();/rc_render();/; 237s/sync_render();/rc_render();/; 238s/sync_render();/rc_render();/; 242s/sync_render();/rc_render();/" 3ds/src/nd_rect.c`.
   Before (:245) `int rtile = (nd_ws.use_t1 || (((w1 >> 24) & 7) == 1 && nd_ws.td[1].valid)) ? 1 : 0;` after `int rtile = (nd_ws.use_t1 || (((w1 >> 24) & 7) == 1 && ND_BTD(1)->valid)) ? 1 : 0;`.
2. nd_rect.c above the MERLON comment of nd_texrect_zprim: `static unsigned char rc_sn; /* NDR1C: the rectangle in progress has a live snap */` and `static void rc_render(void) { if (rc_sn) nd_snap_render(); else sync_render(); } /* NDR1C */`.
   nd_texrect: after the sn_armed line add `rc_sn = nd_snap_live; /* NDR1C */`. `if (sn_armed) nd_snapchk_tri();` becomes `if (sn_armed) nd_snapchk_tri(); else if (rc_sn) nd_snap_comb();`, next line `if (rc_sn) nd_snap_rlog(); /* NDR1C */`.
   Above `int rtile =`: `nd_bsrc = rc_sn ? nd_snap_cur_b() : NULL; /* NDR1C: the tile words of the snap */`. After `rect_done:`: `nd_bsrc = NULL; /* NDR1C */` (no return lies between the two lines: only `goto rect_done`).
3. nd_snap.c: `void nd_snap_mesh(void)` becomes `static void snap_take(int zc)`; in its nd_tgt_role call the last argument `0` becomes `zc`. Below it: `void nd_snap_mesh(void) { snap_take(0); }` and
   `void nd_snap_rect(void) { snap_take(sn_tgt.siz == 2 && (sn_tgt.kind & (NDK_ZERO | NDK_INZ)) && !(nd_ds.w.omode_l & (1u << G_MDSFT_ZSRCSEL))); } /* NDR1C: zcol of nd_rect.c:236 */`.
   Core 0: `unsigned char nd_snap_live;` next to nd_snap_arm; nd_snapchk_run sets `nd_snap_live = 1;` next to `nd_snap_arm = 1;`. `const NdSnapBind* nd_snap_cur_b(void) { return sc_cur.b; }`.
   `void nd_snap_comb(void) { nd_comb_apply(&sc_cur.o, sc_cur.in.w0, sc_cur.in.w1, sc_cur.in.use_t1, sc_cur.in.env); }` (nd_snapchk_tri calls it for its last line).
   `void nd_snap_rlog(void)`: return unless pm_drec_now() and pm_frame_count() != sc_rfr (static, ~0u). Walker render w and mix wk, wc as in nd_snapchk_rnd. When memcmp(&sc_cur.rs, &w) or the mix differs or
   `sc_cur.role != nd_img_role(0)` or `nd_snap_bnew(NULL)`: `sc_rfr = pm_frame_count(); pm_log_d("rectsnap f%u", sc_rfr);`. nd_snap.h: declare nd_snap_rect, nd_snap_live (extern), nd_snap_cur_b, nd_snap_comb, nd_snap_rlog.
4. nd_rec.c. Before (:309, :311) `void nd_snap_mesh(void); /* NDR0E2: ... */` and `    if (PMDL("snapchk")) nd_snap_mesh(); /* NDR0E2: ... */`.
   After `void nd_snap_rect(void); /* NDR1C: strong in nd_snap.c */` and `    nd_snap_rect(); /* NDR1C: the snap of every texture rectangle (memo: one per new input) */`.
   Next to the weak nd_snap_mesh (:667): `__attribute__((weak)) void nd_snap_rect(void) {} /* NDR1C */`. Next to the weak nd_snap_arm (:669): `unsigned char nd_snap_live __attribute__((weak)); /* NDR1C */`.
   stream_run (:685): `nd_snap_arm = 0; }` becomes `nd_snap_arm = nd_snap_live = 0; }` (a command ends the live snap).
## Steps (output to build3ds/out/, read the last 20 lines; 1 build, 6 Azahar starts)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/ndr1c_host.txt 2>&1`: PASS. `grep -c "rc_render();" 3ds/src/nd_rect.c`: 4. Commit `NDR1C 1: texture rectangles take a snap; render, combine, binds and tile words from it`.
2. One call each: `python 3ds/tools/bench.py base <b>` and `sh 3ds/tools/rowloop.sh --row NDR1C --bench <b> --no-host` for b = nimgm_strip_drec, wscmb_pause_drec, wscmb_fsel_drec, spot_mac01_drec.
   For a DIFF bench: `python 3ds/tools/drecsum.py <b> --row NDR1C`, `grep -a "rectsnap\|bindsnap\|drec frame" build3ds/azt/logs/<b>.txt`. The frame of the first DIFF draw d: the last `drec frame <f> seq <s>` line with s <= d.
3. One call each: `python 3ds/tools/bench.py run <s> --row NDR1C --why "NDR1C snapchk"` for s = nimgm_strip_snapchk, wscmb_pause_snapchk. `grep -a snapchk` of both logs > build3ds/out/ndr1c_snapchk.txt.
Gate (exact). Pass: host PASS. snapchk bad 0 in both runs (the rnd compare now runs at live rectangles too). wst is reported next to `python 3ds/tools/bench.py base <s>` at the merge-base (not judged: rectangle compares add stale walker events).
Each drec bench byte-equal, or DIFF with a `rectsnap f<n>` or `bindsnap f<n>` line for the frame of its first DIFF draw.
Stop: a crash, bad above 0, a DIFF frame with no rectsnap or bindsnap line, a stream overflow or fatal line, a tool fault. Commit the outputs, send the paths to the oracle with SendMessage, wait.
Fix rights: the listed files, compile errors.
Pitfalls: `bench.py run` does not build. Run `PM_LANE=NDR1C sh 3ds/build.sh` after each code change, before step 3 (FACT NDR0G2: STALE BUILD exit 5). One host test at a time in a worktree (rowloop takes --no-host). PM_CACHE=off only as a prefix of one command, never exported. One bench per rowloop call. Each scene in its own call, no timeout option.
Keep each file's line endings (CRLF). A snap record is about 640 bytes (GUESS), the stream has 16 chunks of 32 KB per context (FACT nd_rec.c:30-31): the memo must stop a snap when the inputs stay the same.
Hand-back: last commit `NDR1C 2: gate. <bench equal or DIFF draw d frame f> each; snapchk bad <2>, wst <2>` deletes this page. One SubagentHandback: commit hash, per bench equal or (first DIFF draw, frame, rectsnap and bindsnap frames, drecsum line), snapchk bad, wst and recs per run.
