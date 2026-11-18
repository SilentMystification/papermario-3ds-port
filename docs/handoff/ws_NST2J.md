# ws_NST2J (Haiku): core 1 follows the combine sync of core 0 (sc_ok, sc_sig); dschk compares it at every command (stage S3; counters: dschk sclbad, sigbad; reader -5)
Designer: oracle FIN. WAIT: branch from main after the NST2I merge. NDR0E2 may run at the same time (its nd_rec.c and nd_api.h hunks are 20+ lines away). Worktree build3ds/wt/NST2J, branch NST2J. PM_ROW=NST2J PM_LANE=NST2J.
Edit only: 3ds/include/nd_ds.h, 3ds/src/nd_ds.c, 3ds/src/nd_rec.c, 3ds/src/nd_gtab.c, 3ds/src/nd_dschk.c, 3ds/include/nd_api.h (1 line), 3ds/src/nd_direct.c, src/hud_element.c, src/screen_overlays.c (5 calls), 3ds/tools/nd_dschk_test.c, 3ds/tools/fxrest.d/dschk_base.txt.
## Facts (code read; nd_rec.c, nd_dschk.c lines of NST2I 339c372f03; other files main 4abca1ff8a)
- Core 0 skips PRIM, ENV, OMH only with sc_ok 1; TEXTURE, COMB also need sc_sig == sc_sig_now() (nd_set.c:125-141). A handler that runs calls sync_combine (sc_ok 1, sc_sig new, nd_comb.c:338-356): PRIM, ENV, TEXTURE, COMB, RARE, OMH with the cycle type (nd_set.c:95), TILESIZE tile 1 or tile 0 with dual_on (nd_tex_set.c:151, :155, :509).
- sc_ok stays 0 after a model record (nd_recs.c:244, :293, :330) and a direct draw (nd_direct.c:184). A parts run ends with sync_combine (nd_walk.c:475, :523). Core 1 sets ds_sclost at every command (nd_rec.c:206), clears it at a list with triangles (:189). Rule 9 (:204) is the first ep rule of every interval: the ep word does not change.
## Edits (core 1 and dschk only: drec stays byte-equal)
1. nd_ds.h NdDs after `m1v` (:24): `uint8_t sc_lost, sc_dirty; /* NST2J: core 1 view of !sc_ok and sc_sig != sc_sig_now() (nd_comb.c:342); not in ds_fields */`. nd_ds.c initializer (:21): `.sc_lost = 1,`.
2. nd_ds.c: rename `unsigned nd_ds_apply(` (:341) to `static unsigned ds_apply(`. After it:
   ```
   static void ds_sync(NdDs* s, uint32_t k, uint32_t v, unsigned ch, uint32_t h0, int du0) {   /* NST2J: h0, du0 = omode_h, dual_on before the unit */
       int sync = 0; switch (k >> 24) {
       case ND_FS_PRIM: case ND_FS_ENV: sync = (ch & 1u) || s->sc_lost; break;                     /* nd_set.c:132, :135 */
       case ND_FS_TEXTURE: case ND_FS_COMB: sync = (ch & 1u) || s->sc_lost || s->sc_dirty; break; /* nd_set.c:138-141 */
       case ND_FS_RARE: sync = 1; break;                                                          /* nd_set.c:157 */
       case ND_FS_OMH: if (s->w.omode_h == h0 && !s->sc_lost) break;                              /* skipped (nd_set.c:125) */
           if ((k & 0xffu) <= G_MDSFT_CYCLETYPE && (k & 0xffu) + ((k >> 8) & 0xffu) > G_MDSFT_CYCLETYPE) sync = 1; /* nd_set.c:95 */
           else if (s->w.omode_h != h0) s->sc_dirty = 1; break;                                    /* omode_h is a sc_sig input */
       case ND_FS_TILESIZE: sync = ((v >> 24) & 7u) == 1u || (((v >> 24) & 7u) == 0u && (du0 || s->dual_on)); break;
       default: break; }
       if (sync) { s->sc_lost = 0; s->sc_dirty = 0; } }
   unsigned nd_ds_apply(NdDs* s, uint32_t k, uint32_t v) {   /* NST2J: the old body is ds_apply */
       const uint32_t h0 = s->w.omode_h; const int du0 = s->dual_on; const unsigned ch = ds_apply(s, k, v); ds_sync(s, k, v, ch, h0, du0); return ch; }
   ```
   nd_ds_model before `if (q[23])` (:448): `s->sc_lost = 1; /* NST2J: the model runner leaves sc_ok 0 before the post range */`.
3. nd_rec.c: :178 drop `ds_sclost, `. Delete :188-189 (nd_rec_scl) and :206 `ds_sclost = 1;`. In :176 write `nd_ds.sc_lost` for `ds_sclost`. nd_ds_unit first line: `const int lost = s->sc_lost, dirty = s->sc_dirty; /* NST2J: before the unit */`. Replace :258 by
   `if ((lost && (id == ND_FS_OMH || id == ND_FS_PRIM || id == ND_FS_ENV)) || ((lost || dirty) && (id == ND_FS_TEXTURE || id == ND_FS_COMB))) DS_EP(2, id);`
   nd_parts_end after :389: `nd_ds.sc_lost = 0; nd_ds.sc_dirty = 0; /* NST2J: the parts runner ends with sync_combine */`. After nd_parts_end:
   `void nd_ds_feed_dir(const uint32_t* u, unsigned n) { nd_ds_feed(u, n); nd_ds.sc_lost = 1; } /* NST2J: a direct draw ends with sc_ok 0 */`. nd_api.h after :101: declare it.
4. nd_gtab.c: delete :109, :138 and the call `nd_rec_scl(...);` in :134. Before: `nd_ds_list(&nd_ds, list, 0); nd_rec_scl(v2 ? ...); return; }` After: `nd_ds_list(&nd_ds, list, 0); return; }`.
5. `nd_ds_feed(` -> `nd_ds_feed_dir(` at nd_direct.c:285, :367, hud_element.c:564, :596, screen_overlays.c:131 (the runners with nd_direct_end). Not msg_draw.c, not sprite.c.
6. nd_dschk.c after :32: `static unsigned dk_sclbad, dk_sigbad, dk_sclog; /* NST2J */`. After :148: `{ const int l0 = !sc_ok, d0 = sc_ok && sc_sig != sc_sig_now(), l1 = theirs->sc_lost != 0, d1 = theirs->sc_dirty != 0;`
   `if (l0 != l1 || (!l0 && d0 != d1)) { if (l0 != l1) dk_sclbad++; else dk_sigbad++; if (dk_sclog < 16u) { dk_sclog++; pm_log_evt("dschk scl f%u op %x c0 %d %d c1 %d %d", pm_frame_n, (unsigned)nd_cmd_last, l0, d0, l1, d1); } } }`.
   Append ` sclbad %u sigbad %u` (dk_sclbad, dk_sigbad) to the lines :152 and :160. 7. nd_dschk_test.c main, before the last printf, 6 cases on `NdDs s = nd_ds;` with nd_ds_apply (setup, unit a, b -> expect): 1 `s.sc_lost = 1`, PRIM 0, s.w.prim -> lost 0 dirty 0; 2 OMH `2u << 8 | 12u`, `2u << 12` -> lost 0 dirty 1; 3 PRIM as 1 -> dirty 1;
   4 TEXTURE s.w.tex_on, `(uint32_t)s.w.tex_ss << 16 | s.w.tex_ts` -> dirty 0; 5 `s.sc_lost = 1`, OMH as 2 -> lost 1; 6 OMH `2u << 8 | 20u`, `1u << 20` -> lost 0 dirty 0. Print `nst2j sync ok <k>/6`; return 1 when k is not 6 (cases 3 and 5 fail an always-sync model: the seed).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 6 Azahar starts)
1. Edits 1-7. `sh 3ds/tools/nd_host_test.sh` PASS with `nst2j sync ok 6/6`. Commit `NST2J 1: core 1 combine sync state, dschk sclbad and sigbad`.
2. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NST2J --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run the 4 dschk scenes (azt.d/NST3.json); `grep -a "dschk"` > build3ds/out/nst2j_dschk.txt; replace dschk_base.txt with it. Pass: bad 0; cmds and epbad equal to the NST2I lines of dschk_base.txt. sclbad, sigbad are data (GUESS: above 0 from ranges, glyphs, box patterns).
Fix rights: the listed files; compile errors. A DIFF, a crash, an epbad change: stop, send the output path to the oracle. Wait. Stop: last commit `NST2J 2: gate. dschk bad <4> epbad <4>; sclbad <4> sigbad <4>` deletes this page. One SubagentHandback with the 16 `dschk scl` lines of spot_mac01_dschk.
