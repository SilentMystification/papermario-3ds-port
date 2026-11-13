# Row FRAMEHASH1: nd_fhash.c hashes PICA command words with the address registers masked (device and host C)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main 5a53dd0a2)
- PICA command (format of 3ds/src/nd_emit.c:9-15 and 3ds/include/nd.h:240-242): word 0 = first value, word 1 = header: bits 0-15
  register, 16-19 byte mask, 20-27 extra count, bit 31 consecutive. Extra values follow the header. One pad word follows when the
  extra count is odd. Length = 2 + extra + (extra & 1). Consecutive: value k (k >= 1) goes to register + k, else to register.
- Address registers (libctru 3ds/gpu/registers.h): 0x085-0x08A (TEXUNIT0_ADDR1-6), 0x095 (TEXUNIT1_ADDR), 0x09D (TEXUNIT2_ADDR),
  0x11C (DEPTHBUFFER_LOC), 0x11D (COLORBUFFER_LOC), 0x200 (ATTRIBBUFFERS_LOC), 0x203 + 3 i for i 0..11 (ATTRIBBUFFERi_OFFSET),
  0x23A, 0x23B (CMDBUF_ADDR0, 1). 0x227 (INDEXBUFFER_CONFIG): bit 31 is the index format (kept), bits 0-30 the offset (masked).
- FNV-1a 64 as row HOSTCORE1: h = 0xcbf29ce484222325, per byte h ^= b, h *= 0x100000001b3.
## Blocker
HOSTDIR merged. Start: `sh 3ds/tools/wt_new.sh FRAMEHASH1`. PM_ROW=FRAMEHASH1 PM_LANE=FRAMEHASH1 on every call.
## Touch only (places)
New files 3ds/include/nd_fhash.h, 3ds/src/nd_fhash.c, 3ds/tools/nd_fhash_test.c, 3ds/tests/host.d/fhash.sh. Nothing else.
## Exact edits
1. nd_fhash.h (guard ND_FHASH_H, stdint.h): `#define ND_FH_INIT 0xcbf29ce484222325ull`,
   `uint64_t nd_fh_bytes(uint64_t h, const void* p, uint32_t n);` `uint32_t nd_fh_keep(unsigned reg, uint32_t v);`
   `int nd_fh_words(uint64_t* h, const uint32_t* w, uint32_t n);` with one comment line each.
2. nd_fhash.c (pure C, no 3ds.h, no division): nd_fh_bytes = FNV-1a over n bytes. nd_fh_keep returns v & 0x80000000 for 0x227,
   0 for every address register of the Facts, else v. nd_fh_words walks the commands from word 0: for each written register
   (reg, mask, value) it feeds 7 bytes { reg & 0xFF, reg >> 8, mask, k & 0xFF, k >> 8 & 0xFF, k >> 16 & 0xFF, k >> 24 } with
   k = nd_fh_keep(reg, value). A command whose length passes n: stop, return 0. Return 1 when the walk ends exactly at n.
3. nd_fhash_test.c: words A (12) = { 0x00012340, 0x000F0085, 0x00400040, 0x801F0082, 0x00000006, 0x00000000, 0x80001000, 0x000F0227,
   0x00000040, 0x000F0203, 0x00000001, 0x000F022F }. B = A with A[0] 0x00055550, A[6] 0x80002000, A[8] 0x00000080 (only masked
   bits). C = A with A[4] 0x00000007. C2 = A with A[6] 0x00001000. Run nd_fh_words from ND_FH_INIT. Expected, exact:
   A e1f5449bd8d7a601 ret 1, B e1f5449bd8d7a601 ret 1, C 821500b103d744e2 ret 1, C2 ef582e03fd998881 ret 1, A with n 11 ret 0,
   nd_fh_bytes(ND_FH_INIT, "abc", 3) = e71fa2190541574b. Print `fhash ok 6/6`, else `fhash FAIL <case> <hash>` and exit 1.
4. host.d/fhash.sh (LF): `gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/fhash 3ds/tools/nd_fhash_test.c 3ds/src/nd_fhash.c && /tmp/fhash`.
5. Commit: `FRAMEHASH1: nd_fhash.c, PICA command hash with masked address registers, host test`.
## Gate (expected values)
1. Seeded fault: in nd_fh_keep return v for 0x203 (drop the mask of ATTRIBBUFFER0_OFFSET). `python 3ds/tools/bench.py run nd_host
   --row FRAMEHASH1 --why "FRAMEHASH1 seeded fault"`: expected `fhash FAIL B ...`, nd_host FAIL.
2. Put the mask back. Same command, --why "FRAMEHASH1 gate": expected `fhash ok 6/6`, `ND HOST TESTS PASS`.
3. `PM_ROW=FRAMEHASH1 PM_LANE=FRAMEHASH1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: build OK, memcheck OK (nd_fhash.c is
   in the 3ds/src glob). No Azahar start. Counter: none (harness row).
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
