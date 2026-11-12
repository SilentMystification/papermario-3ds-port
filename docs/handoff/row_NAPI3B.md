# Row NAPI3B: message glyphs drawn by a direct executor (NATIVEAPI A2)

Model: Sonnet (user decides). Why not Haiku: the glyph bind has three arms (atlas cell hit, tsz_last memo, new load with the
glyph offsets) and the palette-per-glyph flush rule (MSGBOX 3), and the direct path must take the same arm the walker takes, or text
shifts by a cell. Why not Opus: the arms exist in nd_tex_set.c; this row calls them through the A1 helpers.

## Blocker and base
NAPI2B merged (nd_direct.c, nd_tex_bind_desc, nd_direct_end). `sh 3ds/tools/wt_new.sh NAPI3B` from main after that merge;
PM_ROW=NAPI3B PM_LANE=NAPI3B. Base for the byte compare: `git merge-base HEAD main`.

## Readers and writers of the record (grep)
- Writer: msg_draw_char (src/msg_draw.c, `nd_rec_napi(msg_glyph_run, &rec, sizeof rec / 8u)` at about :1910). The only writer.
- Reader: msg_glyph_run (src/msg_draw.c:1784), the core 0 runner. The only reader. NdGlyphRec stays 48 bytes (6 units, the
  static assert at :1782).
- State the record does not carry: combine, render mode, cycle type, filter and the TLUT mode come from the typed ops that msg_draw
  records before the glyphs (the printer setup). They stay walker ops in this row (S5/S6 move them).

## Design
msg_glyph_run, instead of building a local list and calling nd_walk_state:
1. Texture: the CI4 raster r->raster, r->texW x r->texH, palette r->pal when r->tlut (16 entries). The walker today:
   set_tilesize's glyph arm (nd_tex_set.c:205-232: pm_tex_glyph_same with fmt 2, siz 0, gives the cell origin ggx, ggy;
   tile_x0 = x0 - ggx, no flush, no load), the PG1 tsz_last memo arm (:233-240), else ensure_tile_slow's glyph atlas arm (:416,
   glyph_atlas_bound) or the new load (pm_tex_load, then the origin from pm_tex_origin and pm_tex_glyph_ofs). Build an NdTexDesc
   for the raster, then call the same arms in the same order through a new A1-style helper `nd_tex_bind_glyph(const NdTexDesc*)`
   in nd_tex_set.c. It sets nd_ws.td[0], tile_x0/y0, tile_fx/fy as the walker does. Keep the MSGBOX 3 rule: a palette change with
   pm_tex_cell_bound() false flushes (nd_tex_set.c:581).
2. State: call sync_combine() and sync_render() (the walker's current state, as nd_texrect does before a rect), then
   pm_gpu_tex_rect with x from ulx..lrx, y from uly..lry, s = texOffsetX / 32 + tile origin, t likewise, scale dsdx / 1024 and
   dtdy / 1024 (the nd_texrect arithmetic for a 1-cycle rect, nd_rect.c:219 onward; copy it, do not call nd_texrect).
3. nd_direct_end().
A glyph the direct path cannot state (a texW that is not CI4-shaped, a missing raster): run the old list for that glyph (keep
the current body as the fallback) and count it (`gfb` in a census line every 300 frames).

## Risks
- Rainbow and shaking text: the palette per glyph and per-frame offsets; the byte gate below covers a talk box.
- tsz_last (PG1) and ens_* are walker globals: the helper must update them exactly as the walker does, or the next walker draw
  skips a load it needs. nd_direct_end clears ens_*; check that it also invalidates tsz_last (set tsz_last.ok = 0).
- fps60: glyphs have no anchors. Interpolation is not touched.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- New scenario `msgbox_talk_det` = `msgbox_talk_nowide` + `det` (insert after `msgbox_talk_nowide` in both JSON files).
- `bench.py base msgbox_talk_det --commit <merge-base>`, `bench.py base text_menu --commit <merge-base>`; one list
  `bench.py run msgbox_talk_det,text_menu --row NAPI3B --why "A2: glyphs direct"`; `bench.py cmp` each: every shot byte-equal.
- Census: fncov on `napicen_talk`-style frame: new scenario `glyph_fncov_talk` = msgbox_talk_det without shots, with `fncov=330`,
  until `fncov f\d+ done`, cap 45. PM_FNCOV build, base and head. Expected: pm_gpu_tex_rect calls equal; nd_walk_state calls lower
  by the glyph count; gfb 0.
- Budget 8 Azahar starts. Console: CONSOLE C1 after this merge (one prof push mac_01 0 0 200 and the talk spot).

## Stop
A byte difference or gfb above 0 in the talk scene: stop, commit, hand back the counts and the first differing shot (2 tries per
named cause). Delete this row file in the completing commit.
