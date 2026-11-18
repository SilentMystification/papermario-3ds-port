# ws_NST2F (Haiku): dschk end of S3: adopt after direct draws, box fold, shaded sprite dual_on (counter: dschk fbad lower, bad not higher)
Designer: oracle NSTO. WAIT: branch from branch NST2E at its gate commit (plan_finish.md section 4; facts checked on NST2D b1975eb0c6). Worktree build3ds/wt/NST2F, branch NST2F. PM_ROW=NST2F PM_LANE=NST2F.
Edit only: 3ds/src/nd_ds.c, 3ds/include/nd_ds.h, 3ds/src/nd_dschk.c, 3ds/src/nd_direct.c (2 lines), 3ds/tools/nd_ds_test.c,
3ds/tests/host.d/nst1.sh, 3ds/tools/fxrest.d/dschk_base.txt.
## Facts (code read, main db26d2894d)
- A direct draw writes core 0 state at draw time (nd_direct_bind nd_direct.c:103-116: omode, om_k, combine, dual_on 0, prim, env) and binds
  its texture without the tile and load ops. Its fallback runs the units (fed on core 1 since NST2E). Paths end at nd_direct_end (:179) or
  nd_glyph_draw (:212). Core 1 cannot know which path core 0 takes (nd_direct_ready :85 reads core 0 target and texture state).
- box_fold_sync (nd_comb.c:374-390) folds the combine fields and use_t1 at draw time. The unfold (:385-387) gives the core 1 values.
- The shaded sprite: tex_tile1 sets m1.valid (nd_tex_set.c:453-463), set_tilesize tile 0 runs tex_shaded0 (:156, :497-512): dual_on = 1.
## Edits
1. nd_ds.h: NdDs gets `int m1v, m1w, m1h;`. nd_ds_cmp zeroes them (draw-time list). In ds_tilesize after the dual_on line add:
   ```
   if (tile == 1 && t->tile_fmt[1] == 2 && t->tile_siz[1] == 0 && s->tm.ld_valid && s->w.timg) {        /* tex_tile1 :453 */
       s->m1v = 1; s->m1w = ((lrs - uls) >> 2) + 1; s->m1h = ((lrt - ult) >> 2) + 1; }
   if (tile == 0) {                                                                                   /* tex_shaded0 :497 */
       if (s->m1v && t->tile_fmt[0] == 2 && t->tile_siz[0] == 0 && t->tile_pal[0] != t->tile_pal[1] && t->comb_a == 1 && t->comb_c == 2 &&
           s->m1w > 0 && s->m1h > 0) SET(s->dual_on, 1);  /* GUESS: load_shaded_sprite succeeds */
       s->m1v = 0; }
   ```
2. nd_direct.c: `extern unsigned char nd_dschk_dir;` and `nd_dschk_dir = 1;` as the first line of nd_direct_end and of nd_glyph_draw.
3. nd_dschk.c: define `unsigned char nd_dschk_dir;`. In nd_dschk_run after the hit adopt: `if (nd_dschk_dir) { dir_adopt(c); nd_dschk_dir = 0; }`.
   dir_adopt copies from c into core 0: nd_ws.omode_h, omode_l, om_kh, om_kl, comb_w0, comb_w1, tst1 (whole), use_t1, prim, prim_lod, envc,
   timg, tlut, tlut_n, nd_tm, comb_set, box_prod, comb_alpha_shade, dual_on, and sets `box_folded = 0;`.
   Before each adopt (hit or dir): fill core 0, count the differing fields with nd_ds_cmp into `adiff`, count `adopt`, then adopt and fill again.
4. nd_dschk.c core 0 fill: when `box_folded` is 1, write the unfold values into the filled copy: tst1.comb_a..comb_d = box_save[0..3],
   comb_aa = comb_ab = comb_ac = 7, comb_ad = 2, use_t1 = 0 (nd_comb.c:385-387).
5. The 100-frame line and the end line get ` adopt <a> adiff <d>` at their end.
6. nd_ds_test.c: TIMG 4b, TILE 1 (CI, 4b, pal 1), LOADTILE, TILESIZE 1, TILE 0 (CI, 4b, pal 0), COMB with comb_a 1 and comb_c 2,
   TILESIZE 0: dual_on 1 and m1v 0. Print `nst2f shaded bad <b>`. Fault: TILE 0 with pal 1 (dual_on stays 0).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 6 Azahar starts; the 4 dschk scenes of azt.d/NST3.json)
1. Edits 1-6. Host PASS with `nst2f shaded bad 0` and the fault seen. Commit `NST2F 1: dschk adopt after direct draws, box fold, shaded dual_on`.
2. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NST2F --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run the 4 dschk scenes; grep their dschk lines > build3ds/out/nst2f_dschk.txt. Pass (oracle round 1, gate corrected): in spot_mac01_dschk
   and fxchk_a_dschk the last fbad is lower and bad is not higher than dschk_base.txt (NST2E). epbad and the 2 scenes without a base are recorded,
   not judged: dir_adopt sets box_folded 0 (nd_dschk.c), so sc_sig_now (nd_comb.c:191-193) changes and core 0 runs TEXTURE and COMB units it
   would skip (nd_set.c:139-141): nd_st_epoch rises under dschk only. bad 0 and epbad 0 move to row NST2G (on main with FALLBK 91d6172eb3).
   Write the grep into dschk_base.txt.
## Fix rights: the listed files; compile errors. bad or epbad above 0, a crash, a DIFF: stop, send the output path (with the `dschk bad` name
  lines) to oracle NSTO. Wait. Stop: last commit `NST2F 2: gate. dschk fbad <2> (base <2>), bad <2> (base <2>); epbad <4>; adopt <4> adiff <4>` deletes this page. One SubagentHandback.
