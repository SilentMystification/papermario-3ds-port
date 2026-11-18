# ws_NDR0B (Haiku): the texture descriptors td[0], td[1] on core 1, compared by dschk (stage S4; counter: dschk bad 0 with td)
Designer: oracle FIN. WAIT: branch from branch NST2F at its gate commit. Worktree build3ds/wt/NDR0B, branch NDR0B. PM_ROW=NDR0B PM_LANE=NDR0B.
Edit only: 3ds/src/nd_ds.c, 3ds/src/nd_dschk.c, 3ds/tools/nd_ds_test.c, 3ds/tests/host.d/nst1.sh, 3ds/tools/fxrest.d/dschk_base.txt.
## Facts (code read, main 59a0ffc265 and branch NST2F 2f8d939b64)
- Core 0 writes td only in nd_tex_set.c: set_tilesize tile 0 (:153 valid 0, :181-182, :190-191, :203-213), tex_tile1 (:468-481), tlut_v20 (:487-493).
  The 3 tile 0 branches write the same fields. Valid: src, w > 0, h > 0, and h > 1 or timg outside nuGfxZBuffer (:201). GUESS: the glyph branch (:175)
  never sees h 1 in the z-buffer. tex_shaded0 returns before the window (:156): td[0].valid stays 0.
- tex_win0 and tex_win1 (nd_texwin.c:11, :83) are pure. nd_ds.c has the same inputs: s->w.tst1, s->tm, s->w.timg.
- dschk zeroes td before the compare (nd_ds.c ds_zero_draw). The ep rule reads the change bit (SET): td writes must not use SET.
## Edits
1. nd_ds.c above ds_tilesize (plain stores, no SET):
   ```
   static void ds_td0(NdDs* s, int uls, int ult, int lrs, int lrt) {   /* set_tilesize tile 0 (nd_tex_set.c:158-213) */
       NdTst* t = &s->w.tst1; NdTd* d = &s->w.td[0]; NdWin wn;
       tex_win0(t, &s->tm, s->w.timg, uls, ult, lrs, lrt, &wn);
       if (!wn.src || wn.w <= 0 || wn.h <= 0) return;
       if (wn.h == 1 && (unsigned)(uintptr_t)s->w.timg - (unsigned)(uintptr_t)nuGfxZBuffer < 320u * 240u * 2u) return;
       d->valid = 1; d->src = wn.src; d->fmt = t->tile_fmt[0]; d->siz = t->tile_siz[0]; d->w = wn.w; d->h = wn.h; d->stride = wn.stride;
       d->x0 = uls >> 2; d->y0 = ult >> 2; d->pal_n = s->w.tlut_n;
       d->pal = s->tm.tlut_pal[t->tile_pal[0] & 15] ? s->tm.tlut_pal[t->tile_pal[0] & 15] : s->w.tlut; }
   static void ds_td1(NdDs* s, int uls, int ult, int lrs, int lrt) {   /* tex_tile1 (nd_tex_set.c:464-482) */
       NdTst* t = &s->w.tst1; NdTd* d = &s->w.td[1]; NdWin wn;
       tex_win1(t, &s->tm, s->w.timg, uls, ult, lrs, lrt, &wn);
       d->valid = 0;
       if (!wn.src || wn.w <= 0 || wn.h <= 0) return;
       d->valid = 1; d->src = wn.src; d->fmt = t->tile_fmt[1]; d->siz = t->tile_siz[1]; d->w = wn.w; d->h = wn.h; d->stride = wn.stride;
       d->x0 = wn.x0; d->y0 = ult >> 2; d->pal_n = s->w.tlut_n;
       d->pal = s->tm.tlut_pal[t->tile_pal[1] & 15] ? s->tm.tlut_pal[t->tile_pal[1] & 15] : s->w.tlut; }
   ```
2. ds_tilesize: after the tex_tile1 m1 line add `if (tile == 1) ds_td1(s, uls, ult, lrs, lrt);`. In the `if (tile == 0) {` block: first line `s->w.td[0].valid = 0;`;
   keep the shaded test result in `const int sh = ...` (the same condition), then `if (sh) SET(s->dual_on, 1);`, `s->m1v = 0;`, `if (!sh) ds_td0(s, uls, ult, lrs, lrt);`.
3. ds_loadtlut inside `if (slot < 16)` after the tlut_pal SET (tlut_v20, nd_tex_set.c:487-493): for n 0 and 1, when td[n].valid, td[n].fmt == 2,
   (tile_pal[n] & 15) == slot and td[n].pal != s->w.timg: td[n].pal = s->w.timg, td[n].pal_n = s->w.tlut_n.
4. nd_ds.c: add `DSW(td)` to ds_fields. ds_zero_draw: replace the td memset by `for (int n = 0; n < 2; n++) if (!s->w.td[n].valid) memset(&s->w.td[n], 0, sizeof s->w.td[n]);`.
   nuGfxZBuffer and NdWin come from nd_int.h:18, :194 (nd_ds.h includes it).
5. nd_dschk.c: hit_adopt and dir_adopt copy td: `memcpy(nd_ws.td, c->w.td, sizeof nd_ws.td);`.
6. nd_ds_test.c: define `unsigned short* nuGfxZBuffer;`. Case tlut: td[0] valid, fmt 2, pal A, tile_pal[0] 0, tile_tmem 256 at the load tile; TIMG B; LOADTLUT:
   td[0].pal == B. Case tile 1: TIMG, TILE 1, LOADTILE, TILESIZE 1: td[1] equals a direct tex_win1 call on the same s. Print `ndr0b cases 2 bad <b>`.
   `--fault` sets tile_pal[0] 1 in case tlut. nst1.sh: add 3ds/src/nd_texwin.c to the gcc line.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 6 Azahar starts; the 4 dschk scenes of azt.d/NST3.json)
1. Edits 1-6. Host PASS with `ndr0b cases 2 bad 0` and the fault seen. Commit `NDR0B 1: td on core 1, dschk compares td`.
2. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NDR0B --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run the 4 dschk scenes; grep the dschk lines > build3ds/out/ndr0b_dschk.txt. Pass: bad 0 and epbad 0 in the last line of each. Write the grep into dschk_base.txt.
## Fix rights: the listed files; compile errors. bad above 0 (a `dschk bad` line names td), a crash, a DIFF: stop, send the output path to the oracle. Wait.
  Stop: last commit `NDR0B 2: gate. dschk bad 0 epbad 0 (x4) with td; adopt <4> adiff <4>` deletes this page. One SubagentHandback.
