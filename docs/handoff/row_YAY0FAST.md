# Row YAY0FAST: a faster Yay0 decoder (run copies instead of per-byte checks), same output; the sprite loads are its largest user

(Model: Haiku. Base: main. Edits 3ds/src/pm_map.c yay0_decode_raw (:98-144) -> a new pure-C file 3ds/src/nd_yay0.c, 3ds/CMakeLists.txt
(one source line), a new host test 3ds/tools/yay0_test.c and its build line in 3ds/build.sh (next to gen_shapes). Disjoint from
AZKEEP, FILLA0R, PMUPHFIX2, NAPI0. Reads this file and CLAUDE.md.)

## Facts (DPCEN2, Azahar per map load: sprite loads 15 in 138.5 ms on mac_01, 35 in 73.8 ms on the xfer route; list compiles 4.6-8.4 ms)
- spr_load_sprite (src/101b90_len_8f0.c:185-281): nuPiReadRom of the compressed sprite (a memcpy from the resident ROM image),
  pm_yay0_decode into the sprite heap (up to 256 KB), spr_host_endian (byte order of the anim tables), pointer swizzles. The decode is
  the bulk; the sprite heap is rebuilt per map, so the same NPC sprites decode again at each map load.
- yay0_decode_raw (pm_map.c:98-144) copies a back reference one byte per loop pass with two bound tests per byte (`dst_pos >= dec_size
  || from == 0`, :134) and reads one literal per pass. It also serves the map assets (decode_yay0), the core-1 prefetch (pm_prefetch.c:44)
  and the sprite loads (pm_yay0_decode).
- A host copy of the same decoder runs in gen_shapes (3ds/tools/nd_shape_conv.c:323-380) against the ROM: the equality test can decode
  every asset of the asset table with the old and the new decoder in the same docker step.

## Start
`sh 3ds/tools/wt_new.sh YAY0FAST`. PM_ROW=YAY0FAST PM_LANE=YAY0FAST on every build and bench call.

## Exact edits
1. New 3ds/src/nd_yay0.c (pure C, stdint and string only): `int nd_yay0_raw(const uint8_t* src, uint32_t src_len, uint8_t* dst,
   uint32_t dst_len)`, the body of yay0_decode_raw with the back-reference branch replaced by: after `if (dist > dst_pos) return 0;`
   `if (dist == dst_pos) return 0;` (the old `from == 0` test at the first byte), `if (n > dec_size - dst_pos) return 0;`, then
   `{ uint8_t* d = dst + dst_pos; const uint8_t* s = d - dist - 1; if (dist + 1 >= n) memcpy(d, s, n); else for (uint32_t k = 0; k < n; k++) d[k] = s[k]; dst_pos += n; }`
   (non-overlapping runs copy at once; an overlapping run repeats its pattern byte by byte, as before). Literals and the control words
   unchanged. Declare it in 3ds/include/nd.h (or a new nd_yay0.h).
2. pm_map.c: yay0_decode_raw's body becomes `return nd_yay0_raw(src, src_len, dst, dst_len);` (keep the function and its callers).
3. 3ds/CMakeLists.txt: `src/nd_yay0.c` after `src/nd_recs.c`.
4. 3ds/tools/yay0_test.c: the old decoder copied from nd_shape_conv.c:324 (as yay0_old) and nd_yay0.c; for every asset of the ROM asset
   table (nd_shape_conv's find/iterate code) decode with both into two buffers and memcmp; print `yay0_test: assets %u equal %u` and exit 1
   on any difference. build.sh: run it in the gen_shapes docker step (gcc -m32 ... yay0_test.c /w/3ds/src/nd_yay0.c && /tmp/yay0_test /ar).

## Gate (CLAUDE.md 2; Azahar budget 5 of 8)
- Build: the build log shows `yay0_test: assets N equal N`; memcheck OK; nd_host PASS.
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`; one list `bench.py run spot_mac01_det,mdl5_iwa10_det,dpcen_xfer --row YAY0FAST
  --why "same decode, faster"`; both maps 6/6 byte-equal; the dpcen_xfer dpres line of the load window: sprus below the DPCEN2 value
  (73.8 ms; Azahar time is direction only: the console decides).

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line, the yay0_test line, the dpres line.
