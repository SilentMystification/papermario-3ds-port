# Row CONV2B: the N64 vertex slot resolver (VTX and TRI ops to absolute vertex indices), host C with a test

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main 5a53dd0a2, plan_endgame.md v3 section 6 phase B)
- An N64 list loads vertices into 32 slots (gSPVertex(v, n, v0): array v, n vertices into slots v0..v0+n-1) and draws triangles
  by slot (gSP1Triangle, gSP2Triangles). A later load overwrites slots. CONV2C turns this into absolute indices of the Vtx
  arrays. This row writes only the pure resolver. The op input is decoded already (plain slot numbers, not the x2 word form).
## Blocker
HOSTDIR merged. Start: `sh 3ds/tools/wt_new.sh CONV2B`. PM_ROW=CONV2B PM_LANE=CONV2B on every call.
## Touch only (places)
New files 3ds/tools/nd_vslot.h, 3ds/tools/nd_vslot.c, 3ds/tools/nd_vslot_test.c, 3ds/tests/host.d/vslot.sh. Nothing else.
## Exact edits
1. nd_vslot.h (guard ND_VSLOT_H, stdint.h):
```
enum { ND_VS_VTX = 1, ND_VS_TRI1 = 2, ND_VS_TRI2 = 3 };
typedef struct NdVsOp { uint8_t kind, n, v0, s[6]; uint32_t arr, first; } NdVsOp; /* VTX: arr id, first index, n, v0. TRI1: s[0..2]. TRI2: s[0..5] */
typedef struct NdVsDraw { uint32_t arr, idx0, nidx; } NdVsDraw;                    /* one draw: array id, first entry in idx, entry count */
int nd_vslot_resolve(const NdVsOp* ops, unsigned nops, uint16_t* idx, unsigned idx_max, NdVsDraw* dr, unsigned dr_max,
                     unsigned* ndr, unsigned* nidx);
```
2. nd_vslot.c: slot table of 32 entries {loaded, arr, index}, all unloaded at the start. For op i:
   - VTX: v0 + n > 32 -> return -(i + 1). Else slot v0 + j = {1, arr, first + j} for j 0..n-1.
   - TRI1 (one triangle s[0..2]) and TRI2 (two: s[0..2], then s[3..5]): per triangle, any slot > 31 or not loaded, or the 3
     slots of 2 arrays -> return -(i + 1). The triangle's array: if there is no open draw or its arr differs, open a new draw
     {arr, idx0 = *nidx, nidx 0} (more than dr_max draws -> return -(i + 1)). Append the 3 indices (more than idx_max -> return
     -(i + 1)) and add 3 to the draw's nidx.
   Return 0. *ndr and *nidx hold the counts.
3. nd_vslot_test.c, exact cases (VTX written as VTX(arr, first, n, v0)):
   - F1: VTX(7, 0, 4, 0), TRI1(0, 1, 2), TRI2(0, 2, 3, 3, 1, 0) -> 0, 1 draw {7, 0, 9}, idx 0 1 2 0 2 3 3 1 0.
   - F2: VTX(7, 0, 3, 0), TRI1(0, 1, 2), VTX(7, 10, 3, 0), TRI1(2, 1, 0) -> 0, 1 draw {7, 0, 6}, idx 0 1 2 12 11 10.
   - F3: VTX(1, 0, 3, 0), TRI1(0, 1, 2), VTX(2, 5, 3, 0), TRI1(0, 1, 2) -> 0, draws {1, 0, 3} {2, 3, 3}, idx 0 1 2 5 6 7.
   - E1: TRI1(5, 6, 7) alone -> -1. E2: VTX(1, 0, 2, 0), VTX(2, 0, 1, 2), TRI1(0, 1, 2) -> -3. E3: VTX(1, 0, 4, 30) -> -1.
   Print `vslot ok 6/6`, else `vslot FAIL <case>` with the values and exit 1.
4. host.d/vslot.sh (LF): `gcc -m32 -Wall -Wextra -Werror -I 3ds/tools -o /tmp/vslot 3ds/tools/nd_vslot_test.c 3ds/tools/nd_vslot.c && /tmp/vslot`.
5. Commit: `CONV2B: N64 vertex slot resolver nd_vslot.c, host test`.
## Gate (expected values)
1. Seeded fault: write the F2 expected idx as 0 1 2 10 11 12. `python 3ds/tools/bench.py run nd_host --row CONV2B --why "CONV2B
   seeded fault"`: expected `vslot FAIL F2`, nd_host FAIL.
2. Restore 12 11 10. Same command, --why "CONV2B gate": expected `vslot ok 6/6`, `ND HOST TESTS PASS`.
3. No device build (host files under 3ds/tools only). No Azahar start. Counter: none (part of CONV2, plan section 2).
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
