# ws_NST2I (Haiku): dschk measures every core 0 batch cut (gv_epoch), core 1 sets ep where core 0 always cuts (stage S3 end; counter: epbad by key)
Designer: oracle FIN. WAIT: branch from main after the NST2H merge. Run before NDR0E2 (both edit nd_rec.c; REN8 after both). Worktree build3ds/wt/NST2I, branch NST2I.
PM_ROW=NST2I PM_LANE=NST2I. Edit only: 3ds/src/nd_rec.c (3 places), 3ds/src/nd_dschk.c (the compare), 3ds/tools/fxrest.d/dschk_base.txt.
## Facts (code read; line numbers of NST2H a06c9a7b4c). Unit ids: 0x20 + the order of nd_fs.h (TIMG 0x30, TEXTURE 0x2a, MW 0x2b, OMH 0x20, OML 0x21, COMB 0x2d, MISC 0x3e)
- 10130, 1012b, 1033e (core 0 cut, core 1 not): nd_op_same never skips TIMG, TILE, TILESIZE, LOADBLOCK, LOADTILE, LOADTLUT, FILL, PRIMZ, SCISSOR, RARE (default arm,
  nd_set.c:122-147), nor MW unless LIGHTCOL (:142-144). op_ctl cuts at every RECT and MISC but MODVTX (nd_walk.c:357). Core 1 cuts there only on a field change (rule 1).
- 20130, 20120, 20121 (core 1 change, no core 0 count): the 6 direct-draw feeds (nd_ds_feed after the direct record, e.g. screen_overlays.c:170-171) change nd_ds,
  and core 0 cuts in the direct runner (nd_direct.c:187, :220, :379) with gv_epoch only. The same holds for about 20 other gv_epoch sites (list start nd_walk.c:434,
  :689, particles :462 :510, texrect nd_tex_set.c:231, target nd_target.c:105, model paths nd_recs.c:28 :76 :241 :328, shapes, scomp, ent): nd_st_epoch counts only 4.
- 2022d, 20220, 1012a (sc_lost rule): core 1 sets ds_sclost at every command (nd_rec.c:204); core 0 clears sc_ok only at some (F6). This row records them only.
## Edits
1. nd_dschk.c: the core 0 side is any gv_epoch move since the last compared record. Add `static unsigned dk_gv = 1; /* NST2I: gv_epoch at the last compare */`
   and `extern unsigned char nd_cmd_last;` (nd_rec.c). Replace :137 by:
   ```
   {   const int e0 = gv_epoch != dk_gv;   /* NST2I: every core 0 batch cut, not only the 4 nd_st_epoch sites */
       dk_gv = gv_epoch;
       if (e0 != (ep != 0)) {
           dk_epbad++;
           if (e0) ep_note(0x10000u | (nd_st_epoch ? (nd_st_why & 0xffffu) : (0xff00u | nd_cmd_last)));   /* site ff: the command op that ran */
           else ep_note(0x20000u | (((epw >> 16) & 0xffu) == 9u ? (0x900u | nd_cmd_last) : ((epw >> 8) & 0xffffu)));
       }
   }
   ```
   Keep :138. Update the file comment (:1-4): the core 0 side is the gv_epoch move.
2. nd_rec.c: global `unsigned char nd_cmd_last; /* NST2I: the op of the last command core 0 ran (not a state run, not the dschk record) */`. In stream_run after :658:
   `{ const unsigned op = h >> 24; if (op != NDC_JUMP && !(op == NDC_NAPI && (p[1] == (uint32_t)(uintptr_t)nd_nr_run || p[1] == (uint32_t)(uintptr_t)nd_dschk_run))) nd_cmd_last = (uint8_t)op; }`.
3. nd_rec.c rec_take: after `ds_epw = 0;` (:202): `DS_EP(9, 0); /* NST2I: core 0 runs every command with a gv_epoch cut; the key names the quiet ones */`.
4. nd_rec.c nd_ds_unit after :257:
   ```
   if ((id >= NDO_TIMG && id <= NDO_LOADTLUT) || id == NDO_FILL || id == NDO_PRIMZ || id == NDO_SCISSOR || id == NDO_RARE) DS_EP(6, id);   /* nd_op_same never skips */
   if (id == NDO_MW && ((w0 & 0xffffffu) >> 16) != NDO_MW_LIGHTCOL) DS_EP(7, id);                                                       /* nd_set.c:142-144 */
   if (id == NDO_RECT || (id == NDO_MISC && w0 != ND_NOP_A && ((w0 & 0xffffffu) >> 16) != NDO_MISC_MODVTX)) DS_EP(8, id);               /* op_ctl nd_walk.c:357 */
   ```
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 6 Azahar starts; the 4 dschk scenes of azt.d/NST3.json)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh` PASS. Commit `NST2I 1: dschk gv_epoch side, core 1 ep rules 6-9`.
2. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NST2I --bench the same 2: LOOP PASS, byte-equal (all edits feed dschk only).
3. bench.py run the 4 dschk scenes; `grep -a "dschk"` > build3ds/out/nst2i_dschk.txt; write it into dschk_base.txt (replace). Pass: bad 0 in the last counter line
   of each; the epsrc counts plus `other` sum to epbad. epbad is data (GUESS: the 10130, 1012b, 1033e and direct-draw keys go; 2022d, 20220, 1012a and rule 9
   keys of quiet commands stay). Send the epsrc lines in the hand-back.
Fix rights: the listed files; compile errors. A DIFF, a crash, a sum that differs: stop, send the output path to the oracle. Wait.
Stop: last commit `NST2I 2: gate. dschk bad <4> epbad <4>; top key per scene <4>` deletes this page. One SubagentHandback with the epsrc lines of the 4 scenes.
