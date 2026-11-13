# Row HOSTCORE1: the draw record format of the golden masters (nd_drec.h, a text writer, a host test)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main 5a53dd0a2, plan_endgame.md section 5 "Golden master")
- A golden file holds one text block per draw. Addresses are never written: their content is (FNV-1a 64 of the bytes).
  HOSTCORE2 fills one NdDrec per draw at the emit boundary. This row writes only the format, the writer and its test.
- FNV-1a 64: h = 0xcbf29ce484222325, per byte h ^= b, h *= 0x100000001b3 (mod 2^64).
## Blocker
HOSTDIR merged. Start: `sh 3ds/tools/wt_new.sh HOSTCORE1`. PM_ROW=HOSTCORE1 PM_LANE=HOSTCORE1 on every call.
## Touch only (places)
New files 3ds/include/nd_drec.h, 3ds/tools/nd_drec.c, 3ds/tools/nd_drec_test.c, 3ds/tests/host.d/drec.sh. Nothing else.
## Exact edits
1. nd_drec.h (guard ND_DREC_H, includes stdint.h and stdio.h):
```
typedef struct NdDrecReg { uint16_t reg; uint8_t mask; uint32_t val; } NdDrecReg; /* a state register write; an address register carries 0 */
typedef struct NdDrecBlob { const void* p; uint32_t n; } NdDrecBlob;          /* bytes the draw reads (n 0: none) */
typedef struct NdDrec {
    uint32_t seq, kind, n;                 /* draw number from 0; 1 indexed triangles, 2 arrays; index or vertex count */
    const NdDrecReg* regs; uint32_t nregs; /* in emit order */
    NdDrecBlob tex[3]; uint32_t tex_type[3], tex_dim[3]; /* level 0 texels of units 0-2 */
    NdDrecBlob vtx, idx;
    const float (*mtx)[16]; uint32_t nmtx; /* palette matrices, memory order */
} NdDrec;
uint64_t nd_drec_fnv(const void* p, uint32_t n);
void nd_drec_write(FILE* f, const NdDrec* r);
```
2. nd_drec.c: nd_drec_fnv as in the Facts. nd_drec_write prints, in this order: `draw %u kind %u n %u`; per reg `r %03x %x %08x`;
   per unit u 0..2 with tex[u].n > 0: `t %u type %08x dim %08x bytes %u h %016llx`; `v bytes %u h %016llx`; `i bytes %u h %016llx`
   (always, n 0 gives h cbf29ce484222325); per matrix k: `m %u` then 16 times ` %08x` of the float bits (memcpy to uint32_t);
   `end`. Every line ends with "\n". No other output.
3. nd_drec_test.c: record A: seq 0, kind 1, n 3, regs {0x0C0, 0xF, 0x000F0F0F} and {0x104, 0x3, 0x00000001}, tex[0] = the 16
   bytes 0..15, tex_type[0] 0x0000000D, tex_dim[0] 0x00040004, units 1 and 2 n 0, vtx = 48 bytes with byte i = (i * 3) & 0xFF,
   idx = uint16_t {0, 1, 2} (6 bytes), one identity matrix. Record B: A with seq 1 and vtx byte 5 = 0xFF. Write A then B to
   tmpfile(), read it back, compare with this exact text (16 lines), print `drec ok 2` or `drec FAIL` + the text, exit 1:
```
draw 0 kind 1 n 3
r 0c0 f 000f0f0f
r 104 3 00000001
t 0 type 0000000d dim 00040004 bytes 16 h 7c84dc9477851775
v bytes 48 h b9709bed602225a5
i bytes 6 h 37f11cf1d36d7afe
m 0 3f800000 00000000 00000000 00000000 00000000 3f800000 00000000 00000000 00000000 00000000 3f800000 00000000 00000000 00000000 00000000 3f800000
end
```
   then the same 8 lines for draw 1 with `draw 1 kind 1 n 3` and `v bytes 48 h 9bc4836d4ef03135`.
4. host.d/drec.sh (LF): `gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/drec 3ds/tools/nd_drec_test.c 3ds/tools/nd_drec.c && /tmp/drec`.
5. Commit: `HOSTCORE1: draw record format nd_drec.h, text writer, host test`.
## Gate (expected values)
1. Seeded fault: in nd_drec_test.c write the draw 0 v hash as b9709bed602225a6. `python 3ds/tools/bench.py run nd_host --row
   HOSTCORE1 --why "HOSTCORE1 seeded fault"`: expected `drec FAIL`, nd_host FAIL.
2. Put a5 back. Same command, --why "HOSTCORE1 gate": expected `drec ok 2`, `ND HOST TESTS PASS`. No build needed (host-only
   files under 3ds/tools and a header no device file includes). No Azahar start. Counter: none (harness row).
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
