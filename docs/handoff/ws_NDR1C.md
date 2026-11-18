# ws_NDR1C (Haiku): every texture rectangle takes a snap; its render state and binds come from the snap (stage S4; counter: none of C1-C12)
Designer: oracle FIN. WAIT: branch from main after the NDR0G2 and REN8 merges (nd_snap_render of NDR0F2, nd_bsrc of NDR0G2, nd_rec.c of REN8).
Worktree build3ds/wt/NDR1C, branch NDR1C. PM_ROW=NDR1C PM_LANE=NDR1C. Edit only: 3ds/src/nd_rec.c (3 lines), 3ds/src/nd_snap.c, 3ds/include/nd_snap.h, 3ds/src/nd_rect.c.
## Facts (code read, main 5fc6cf135b; find each line by its text)
- nd_rec_texrect takes the snap only with the snapchk switch (nd_rec.c:309). So in a drec run no rectangle is armed: the NDR1B combine apply (nd_rect.c:241) never ran in a drec gate.
- nd_texrect renders with sync_render at :237, :238, :242 and in nd_texrect_zprim :205; it binds at :245-248 (rtile, ensure_tile, dual_bind).
- The walker role of a rectangle reads zcol (nd_tgt.h:38-41, set at nd_rect.c:236). Core 1 takes the role with zc 0 (nd_snap.c nd_snap_mesh). zcol is a pure rule
  of the target and oml: `siz == 2 && (kind & (NDK_ZERO | NDK_INZ)) && !(oml & (1u << G_MDSFT_ZSRCSEL))` (nd_rect.c:236).
- The early returns DEPTHQ (:232) and rect_to_image (:233) draw no GPU primitive: they drop the snap (the next draw after a command takes a new snap, nd_rec.c:648 counts NDC_TEXRECT).
## Edits
1. nd_snap.c: rename `void nd_snap_mesh(void)` to `static void snap_take(int zc)`; in its nd_tgt_role call the last argument `0` becomes `zc`. Add below it:
   `void nd_snap_mesh(void) { snap_take(0); }` and
   `void nd_snap_rect(void) { snap_take(sn_tgt.siz == 2 && (sn_tgt.kind & (NDK_ZERO | NDK_INZ)) && !(nd_ds.w.omode_l & (1u << G_MDSFT_ZSRCSEL))); } /* NDR1C: zcol of nd_rect.c:236 */`
   Add `void nd_snap_bsrc(int on) { nd_bsrc = on ? sc_cur.b : NULL; } /* NDR1C: the bind source of a rectangle */` at the end. nd_snap.h: declare nd_snap_rect and nd_snap_bsrc.
2. nd_rec.c. Before (:307-309): `void nd_snap_mesh(void); /* NDR0E2: ... */` and `    if (PMDL("snapchk")) nd_snap_mesh(); /* NDR0E2: ... */`
   After: `void nd_snap_rect(void); /* NDR1C: strong in nd_snap.c */` and `    nd_snap_rect(); /* NDR1C: the snap of every texture rectangle */`. Next to the weak nd_snap_mesh (:654): `__attribute__((weak)) void nd_snap_rect(void) {}`.
   nd_rec_cmds (:648): delete `ndc_n[NDC_TEXRECT] + ` (a rectangle after a rectangle with the same inputs takes no new snap; GUESS 640 bytes per snap record).
3. nd_rect.c above nd_texrect_zprim:
   `static unsigned char rc_sn; /* NDR1C: the rectangle in progress has an armed snap */`
   `static void rc_render(void) { if (rc_sn) nd_snap_render(); else sync_render(); } /* NDR1C */`
   In nd_texrect after its first line: `rc_sn = sn_armed;`. Replace `sync_render();` by `rc_render();` at :205, :237, :238 and :242 (4 sites; nd_fillrect keeps its own).
4. nd_rect.c binds. Before (:245): `    int rtile = (nd_ws.use_t1 || (((w1 >> 24) & 7) == 1 && nd_ws.td[1].valid)) ? 1 : 0;`
   After: `    nd_snap_bsrc(rc_sn); /* NDR1C: the binds of the snap */` and `    int rtile = (nd_ws.use_t1 || (((w1 >> 24) & 7) == 1 && ND_BTD(1)->valid)) ? 1 : 0;`.
   After `dual_bind();` (:248): `    nd_bsrc = NULL; /* NDR1C */`. No return lies between the 2 new lines.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 2 Azahar starts)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh` PASS. `grep -c "rc_render();" 3ds/src/nd_rect.c`: 4. Commit `NDR1C 1: texture rectangles take a snap, render and binds from it`.
2. `python 3ds/tools/bench.py base nimgm_strip_drec,wscmb_pause_drec,wscmb_fsel_drec,spot_mac01_drec` (merge-base, not charged).
   `sh 3ds/tools/rowloop.sh --row NDR1C --bench nimgm_strip_drec,wscmb_pause_drec,wscmb_fsel_drec,spot_mac01_drec`: LOOP PASS, 4 byte-equal.
   Expected: byte-equal. GUESS: this is the first drec gate of the NDR1B rectangle combine apply.
3. `python 3ds/tools/bench.py run nimgm_strip_snapchk,wscmb_pause_snapchk --row NDR1C --why "NDR1C snapchk"`; `grep -a snapchk` of both logs > build3ds/out/ndr1c_snapchk.txt. Pass: bad 0.
Fix rights: the listed files; compile errors. A build after a partial shape cache: `PM_CACHE=off sh 3ds/build.sh`.
A DIFF, a crash, a bad above 0: stop, commit the outputs, send the paths to the oracle with SendMessage. Wait.
Stop: last commit `NDR1C 2: gate. 4 benches byte-equal; snapchk bad <2>` deletes this page. One SubagentHandback.
