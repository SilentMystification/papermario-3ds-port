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
the tex_win0 note at :195), 3ds/tools/nd_gfx_conv.c: the tile 1 branch at :332 (and the tile tracker it calls for tile 0, :330-345).
## Exact edits
1. Cut tex_win1 (:451 comment to the closing brace :514) out of nd_tex_set.c and append it unchanged to nd_texwin.c without
   `static`. Add `void tex_win1(const NdTst* t, const NdTexM* m, const void* timg, int uls, int ult, int lrs, int lrt, NdWin* o);`
   to nd_int.h next to the tex_win0 note.
2. nd_gfx_conv.c :332: in place of the v2_unsup line, compute the tile 1 window with tex_win1 the same way the tile 0 branch uses
   tex_win0 (the next lines of the tile 0 path, :333-345), and record it as an NdBindDesc for tile 1 (NDN_TEX with a = 1).
3. Commit: `CONV2WIN1: tex_win1 in the pure part (nd_texwin.c), the converter takes tile 1 windows`.
## Gate (expected values)
1. Device unchanged: `git diff --color-moved=plain` of nd_tex_set.c and nd_texwin.c shows the function as moved text (only
   `static` and the new prototype differ). Build, memcheck OK.
2. nd_host PASS with `ntab ... unsup 44` (121 - 77) and `bad 0`, tables 902 (825 + 77). Seeded fault first (a value change):
   in the converter pass `lrs + 4` to tex_win1: bad above 0. Then restore.
3. No Azahar (no runtime file reads v2). Counter: none yet (GSET rows move C5, C6).
## Stop and end
A list of the 77 that still stops (hand back its unsup line), a bad above 0 with the real edit, or a tool fault: stop, commit,
hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
