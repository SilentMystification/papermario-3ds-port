# Row FRAMEHASH2: the frame hash feeds the bytes a draw reads (vertices, indices, texels) and the target role, not addresses

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main b8b314758, libctru 3ds/gpu/registers.h, FRAMEHASH1 row)
- nd_fhash.c (FRAMEHASH1): nd_fh_words feeds (reg, mask, nd_fh_keep(reg, v)) per written register. Address registers feed 0.
- Registers the draw content needs (values as written): 0x200 ATTRIBBUFFERS_LOC (physical >> 3), 0x203 + 3 i OFFSET (bytes from
  the base), 0x205 + 3 i CONFIG2 (bits 16-23 stride in bytes, bits 28-31 component count, 0 = buffer off), 0x227 INDEXBUFFER_CONFIG
  (bits 0-27 offset from the base, bit 31: 1 = u16 indices, 0 = u8), 0x228 NUMVERTICES, 0x22A VERTEX_OFFSET, 0x22E DRAWARRAYS,
  0x22F DRAWELEMENTS (a write = one draw). Texture: 0x080 bits 0-2 unit enable, unit 0 dim 0x082 (bits 16-26 width, 0-10 height),
  addr 0x085 (physical >> 3), type 0x08E (bits 0-3 format); unit 1: 0x092, 0x095, 0x096; unit 2: 0x09A, 0x09D, 0x09E. Bits per
  texel by format 0..13: 32 24 16 16 16 16 16 8 8 8 4 4 4 8. Colour and depth buffer: 0x11D and 0x11C (physical >> 3).
## Blocker
FRAMEHASH1 merged. Start: `sh 3ds/tools/wt_new.sh FRAMEHASH2`. PM_ROW=FRAMEHASH2 PM_LANE=FRAMEHASH2 on every call.
## Touch only (places)
3ds/include/nd_fhash.h (new lines after the nd_fh_words declaration), 3ds/src/nd_fhash.c (new functions after nd_fh_words),
3ds/tools/nd_fhash_test.c (new cases at the end of main, before the result line). Nothing else. nd_fh_words stays as it is.
## Exact edits
1. nd_fhash.h: `typedef struct NdFhEnv { const void* (*map)(uint32_t pa, uint32_t n, void* ctx); uint32_t (*role)(uint32_t pa,
   void* ctx); void* ctx; } NdFhEnv;` (map: bytes at physical pa, NULL when not readable. role: a small id of a target address)
   and `int nd_fh_frame(uint64_t* h, const uint32_t* w, uint32_t n, const NdFhEnv* e);`.
2. nd_fhash.c nd_fh_frame: the same walk as nd_fh_words (same 7-byte feed per register), and it keeps the last value of every
   register of the Facts in a table of 0x300 words (static, zero at the call start). Extra feeds, each as one uint32_t in 4 little
   endian bytes before the data:
   - a write of 0x11C or 0x11D: feed 0xD3000000 | (reg & 0xFF), then role(value << 3).
   - a write of 0x22E or 0x22F (one draw): count = reg 0x228. Indexed (0x22F): idx = base + (reg 0x227 & 0x0FFFFFFF), size 2 if
     bit 31 else 1. Feed 0xD1000000, then the count * size index bytes. lo, hi = min and max index. Arrays (0x22E): lo = reg 0x22A,
     hi = lo + count - 1. base = reg 0x200 << 3. Per buffer i 0..11 with CONFIG2 bits 28-31 not 0: stride = bits 16-23, feed
     0xD0000000 | i, then the bytes [base + OFFSET_i + lo * stride, base + OFFSET_i + (hi + 1) * stride). Per unit u with bit u of
     reg 0x080: w, ht, fmt from its registers, feed 0xD2000000 | u, w, ht, fmt, then w * ht * bits / 8 bytes at its address << 3.
   - bytes that map gives NULL for: feed 0xDEADBEEF and the length instead. No division (bits / 8 is a shift).
   - Correction 1 (ORACLE8, spec of the coder's choices in bce564bb3): an indexed draw whose index bytes map gives NULL feeds
     0xD1000000, 0xDEADBEEF and count * size, and no vertex feed (the range is not known). A texel format above 13 feeds w, ht
     and fmt and 0 texel bytes. 0x11D is the colour buffer and 0x11C the depth buffer: both feed only their role.
3. nd_fhash_test.c new cases, one map callback over a 64 KB fixture array at physical 0x20000000 (pa outside: NULL), role =
   0 for 0x20008000 and 0x20009000, else 1. Command words: base 0x20000000 >> 3, buffer 0 offset 0x100 stride 16 count 3, index
   u16 at offset 0x400 {0, 1, 2}, NUMVERTICES 3, unit 0 on, 8 x 8 RGBA8 at 0x20001000, colour buffer 0x20008000, DRAWELEMENTS.
   Expected (exact relations): S1 same words and bytes, second run -> equal. S2 vertex byte at 0x20000110 changed -> differs.
   S3 byte at 0x20000140 (vertex 4, not drawn) changed -> equal. S4 texel byte at 0x20001004 changed -> differs. S5 colour buffer
   0x20009000 (same role 0) -> equal. S6 the vertex, index and texel bytes and their address words moved by +0x2000
   (colour buffer word unchanged) -> equal.
   Print `fhframe ok 6/6` or `fhframe FAIL S<k>` and exit 1. The old line `fhash ok 6/6` stays.
4. Commit: `FRAMEHASH2: nd_fh_frame feeds vertex, index and texel bytes and the target role, host test`.
## Gate (expected values)
1. Seeded fault (correction 2: a value change, every variable stays used): in fh_tex change the length argument of
   `h = fh_data(h, e, fh_tab[addrr[u]] << 3, n);` from `n` to `(n > 4u ? 4u : n)` (only the first 4 texel bytes are fed).
   Code read of the cases: only S4 changes a texel byte, at offset 4 of the 8 x 8 RGBA8 texture (0x20001004 - 0x20001000), which
   the seed no longer feeds. S1, S3, S5, S6 expect equal and stay equal. S2 changes a vertex byte and still differs. So the
   expected output is exactly one FAIL line, `fhframe FAIL S4`, and nd_host FAIL. `python 3ds/tools/bench.py run nd_host
   --row FRAMEHASH2 --why "FRAMEHASH2 seeded fault"`.
2. Restore `n`. Same command, --why "FRAMEHASH2 gate": expected `fhash ok 6/6`, `fhframe ok 6/6`, `ND HOST TESTS PASS`.
3. `PM_ROW=FRAMEHASH2 PM_LANE=FRAMEHASH2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. No Azahar. Counter: none.
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
