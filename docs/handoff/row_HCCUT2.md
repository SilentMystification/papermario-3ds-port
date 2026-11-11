# Row HCCUT2: rare bodies of three class E and G functions cold (byte-equal)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (main map build3ds/pm_3ds.map 2026-11-11, hot_order.ld counts, oracle code read)
- nd_tmpl_build (3ds/src/nd_tmpl.c:411-457, class E, 1008 B, 22.3 calls per frame in hot_order.ld:55): the hot part is the memo test and
  the hash probe (:412-424); the miss part (:425-456: victim, pm_comb_build, nd_tev_build, build_words, tmpl_index) runs only for a new
  template key. build_words (:330-389, 1072 B), tmpl_index (:394-401) and tmpl_victim (:403-410) are called only from that miss part.
- box_pattern_rect (3ds/src/nd_rect.c:105, class G, 1328 B, `noinline`): called only from nd_texrect (:237) behind the inline guard
  `box_pat && nd_ws.td[0].valid && nd_ws.td[1].valid` (draw_box windows; not in hot_order.ld).
- rect_to_image (nd_rect.c:60-96, class G, 640 B, 4.3 calls per frame, hot_order.ld:142): the first line (:61) refuses almost every call;
  the drawing body (:62-95) is the shaded-sprite TLUT write into an off-screen image.
## Start
`sh 3ds/tools/wt_new.sh HCCUT2`; PM_ROW=HCCUT2 PM_LANE=HCCUT2 on every call.
Other rows: HC6/HC6B (pm_gpu.c, nd_recs.c, nd_model.c, nd_emit.c, nd_rec.c), WALLSHADE (nd_geom.c, nd_walk.c, pm_gpu.c), UB1, HC9B
(nd_rec.c). None edits nd_tmpl.c or nd_rect.c. `git merge main` before the completing commit.
## Touch only
3ds/src/nd_tmpl.c: nd_tmpl_build, build_words, tmpl_index, tmpl_victim (definition lines and the miss part). 3ds/src/nd_rect.c:
box_pattern_rect (definition line), rect_to_image (:60-96). No other line. The function names in hot_order.ld stay (no rename of them).
## Exact edits
1. nd_tmpl.c: `static void build_words(` -> `static __attribute__((cold)) void build_words(`; the same `__attribute__((cold))` on
   tmpl_index and tmpl_victim.
2. nd_tmpl.c: add above nd_tmpl_build a new function
   `static __attribute__((cold, noinline)) int tmpl_miss(const NdTmplKey* kkp, unsigned h) { NdTmplKey kk = *kkp; <lines 425-456 moved
   here unchanged> }` (the body from `/* FB-2: a full table evicts ...` to `return (int)tmpl_n++;`). In nd_tmpl_build replace those
   lines with `return tmpl_miss(&kk, h); /* HCCUT2: the miss part is cold */`. `h` after the probe loop is the free index slot (the
   moved line `tmpl_idx[h] = ...` uses it).
3. nd_rect.c:105: `__attribute__((noinline)) int box_pattern_rect(` -> `__attribute__((cold, noinline)) int box_pattern_rect(`.
4. nd_rect.c rect_to_image: rename the function at :60 to `static __attribute__((cold, noinline)) int rect_to_image_draw(int xl, int yl,
   int xh, int yh, unsigned tile, u32 hs, u32 hd)` and delete its first line (:61). Directly below it add:
   `__attribute__((noinline)) int rect_to_image(int xl, int yl, int xh, int yh, unsigned tile, u32 hs, u32 hd) {`
   `    if (!nd_ws.cimg || pm_is_cfb(nd_ws.cimg) || tile >= 8 || nd_ws.tst1.tile_tmem[tile] < 256) return 0;`
   `    return rect_to_image_draw(xl, yl, xh, yh, tile, hs, hd); /* HCCUT2: the drawing body is cold */ }`
## Gate (CLAUDE.md 2)
- `PM_ROW=HCCUT2 PM_LANE=HCCUT2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `PM_LANE=HCCUT2 python 3ds/tools/bench.py run hotset_size --row HCCUT2 --lane HCCUT2 --why "HCCUT2 sizes"`: rc 0 (0 stale), M1hot E
  and G lower than main's line (write both before and after; expected E about -1.8 KB, G about -1.9 KB). No lower value: stop, hand back.
- `python 3ds/tools/bench.py run nd_host --row HCCUT2 --why "HCCUT2"`: PASS.
- Byte compare (one list): spot_mac01_det and msgbox_talk_nowide (a draw_box window): `bench.py base` each, one `run a,b`, `cmp` each:
  all shots byte-equal.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
