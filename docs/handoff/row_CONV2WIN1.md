# Row CONV2WIN1: the tile 1 window (tex_win1) moves to the pure part, so 77 lists with a tile 1 SetTileSize get v2 tables

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main bb9d5483e, CONV2C hand-back)
- CONV2C: `ntab tables 825 draws 5253 bad 0`, `ntab inherit 426 unsup 121 descs 104`. 77 of the 121 stop at
  3ds/tools/nd_gfx_conv.c:332 `if (tile == 1) { v2_unsup("tile 1 size (the tile 1 window is not in the pure part)", k); return; }`.
- tex_win1 is `static` in 3ds/src/nd_tex_set.c:452-514 (pure: its inputs are NdTst, NdTexM, timg and the window, output NdWin; the
  comment at :451 says "pure, as tex_win0"). S7TEX1 moved tex_win0 the same way into 3ds/src/nd_texwin.c (:11), with the
  pointer at nd_int.h:195. The device call stays at nd_tex_set.c:530.
Start: `sh 3ds/tools/wt_new.sh CONV2WIN1`. PM_ROW=CONV2WIN1 PM_LANE=CONV2WIN1 on every call.
## Touch only (places)
3ds/src/nd_tex_set.c:451-514 (the moved function), 3ds/src/nd_texwin.c (append it), 3ds/include/nd_int.h (one prototype next to
the tex_win0 note at :195), 3ds/tools/nd_gfx_conv.c: the state struct (:187), the tile tracker (:330-345), the draw handler
(:437-451), v2_make_desc (:228-236) and the place where emitted_bind is reset; 3ds/tools/nd_gtab_test.c: TriEv (:221), MSt (:233), m_desc, m_tilesize, m_tri, v_replay, v2_compare.
## Exact edits
1. Cut tex_win1 (:451 comment to the closing brace :514) out of nd_tex_set.c and append it unchanged to nd_texwin.c without
   `static`. Add `void tex_win1(const NdTst* t, const NdTexM* m, const void* timg, int uls, int ult, int lrs, int lrt, NdWin* o);`
   to nd_int.h next to the tex_win0 note.
2. Correction 1 (ORACLE8): the form has tile 1 already (CONV2A: NDN_TEX `a = tile (0 or 1)`, one NdBindDesc per op). No new
   native form is needed. nd_gfx_conv.c:
   a. state struct (:187, next to `int td_valid; NdBindDesc td;`): `int td1_valid; NdBindDesc td1;`. The cx state next to
      emitted_bind: `int emitted_bind1;`, reset to -1 where emitted_bind is reset.
   b. Correction 2 (ORACLE8, device rule nd_tex_set.c:528-545: td[1].valid = 0, then 1 when wn.src && wn.w > 0 && wn.h > 0,
      x0 = wn.x0, y0 = ult >> 2, fmt/siz/pal from tile 1, wrap and sz from tst tile 1 at bind, nd_comb.c:364/394). Replace :332 with:
      `if (tile == 1) { s->td1_valid = 0; if (!s->timg_ok) { v2_unsup("texture image outside the set", k); return; }`
      `NdWin wn; tex_win1(&s->t, &s->m, s->timg, uls, ult, lrs, lrt, &wn);`
      `if (wn.src && wn.w > 0 && wn.h > 0) { const unsigned pi = s->t.tile_pal[1] & 15u; NdBindDesc* d = &s->td1; memset(d, 0, sizeof *d);`
      `d->src = wn.src; d->pal = s->m.tlut_pal[pi] ? s->m.tlut_pal[pi] : s->tlut; d->pal_n = s->tlut_n; d->fmt = s->t.tile_fmt[1];`
      `d->siz = s->t.tile_siz[1]; d->stride = wn.stride; d->x0 = wn.x0; d->y0 = ult >> 2; d->w = wn.w; d->h = wn.h;`
      `for (int i = 0; i < 4; i++) { d->wrap[i] = s->t.tile_wrap[1][i]; d->sz[i] = s->t.tile_sz[1][i]; } s->td1_valid = 1; } return; }`
   c. draw handler, right after the tile 0 block (after :451): the same block for tile 1 under `if (s->td1_valid && s->tex_on)`:
      find or add s->td1 in cx->bind (the same list), a name `&ndnb_%s_%u`, and when the index differs from cx->emitted_bind1:
      v2_flush, `v2_push(NDN_TEX, 1, 0, 0, p, 0)`, cx->emitted_bind1 = index.
   d. v2_make_desc (:228-236), after the memset: `d.t1_valid = (uint8_t)s->td1_valid;` (t1_valid is a pipeline input:
      nd_api.h:30, read by nd_decide.c:76 and :247).
   nd_gtab_test.c (the v1 model has no tile 1 state today: m_tilesize :274 returns for tile != 0):
   e. MSt (:233): add `int td1_valid; NdBindDesc td1;`. TriEv (:221): add `int tex1; NdBindDesc b1;` at the end.
   f. m_tilesize (:274): replace `if (tile != 0) return;` with the block of edit b, written for the model: `ms.` for `s->`,
      `&ms.t, &ms.m, ms.timg` as the inputs, ms.td1 and ms.td1_valid, no timg_ok test and no v2_unsup. Then `if (tile != 0) return;`.
   g. m_desc (:248): add `d.t1_valid = (uint8_t)s->td1_valid;`. m_tri (:361): add `e->tex1 = ms.td1_valid && ms.tex_on;`
      and `if (e->tex1) e->b1 = ms.td1;`.
   h. v_replay: `const NdBindDesc* bind1 = NULL;` under bind (:402). :408 becomes
      `case NDN_TEX: if (o->a) bind1 = (const NdBindDesc*)o->p; else bind = (const NdBindDesc*)o->p; break;`. In NDN_DRAW after
      :427: `e->tex1 = bind1 != NULL; if (bind1) e->b1 = *bind1;`.
   i. v2_compare after :473: `if (m->tex1 && (!v->tex1 || memcmp(&m->b1, &v->b1, sizeof m->b1))) v2_fail("tile 1 bind descriptor", name, i);`
3. Commit: `CONV2WIN1: tex_win1 in the pure part (nd_texwin.c), the converter takes tile 1 windows`.
## Gate (expected values)
1. Device unchanged: `git diff --color-moved=plain` of nd_tex_set.c and nd_texwin.c shows the function as moved text (only
   `static` and the new prototype differ). Build, memcheck OK.
2. nd_host PASS with `ntab ... unsup 44` (121 - 77) and `bad 0`, tables 902 (825 + 77), descs 104 or more (expected to grow:
   a desc with t1_valid 1 is a new pipeline id; the pso compare with t1_valid in both models proves the ids). Seeded fault first (a value change):
   in the converter pass `lrs + 4` to tex_win1: bad above 0 (the tile 1 bind compare sees it). Then restore.
   Step 1 is done (dcea6b999): resume at edit 2.
3. No Azahar (no runtime file reads v2). Counter: none yet (GSET rows move C5, C6).
## Stop and end
A list of the 77 that still stops (hand back its unsup line), a bad above 0 with the real edit, or a tool fault: stop, commit,
hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
