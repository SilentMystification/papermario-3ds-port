# Row CONV2MIX: CLOSED by ORACLE8 on 2026-11-13. Do not merge branch CONV2MIX (621963a37). The orchestrator deletes this file and the branch.

## Decision and reason
- The CONV2D design (row_CONV2D.md) runs the NDN_UNIT copy of a table through nd_walk_state. It does not read NDN_DRAW. A
  generated vertex array only serves NDN_DRAW. It also needs a compare change in nd_gtab_test.c (the vertex base check and the
  model's two-array refusal), and its seed proved only an NDN_COL value. That costs work and buys nothing for the runner now.
- Exceptions: the 16 lists of ntab_unsup.txt with the reason `vertex slots (nd_vslot_resolve)` keep no v2 table. They run on v1
  (gt_ops), because nd_ntab_find gives NULL for them. 11 of them also load past the declared size of the source Vtx symbol
  (D_09002650_3BFFF0 is Vtx[1] and the list loads 30): D_09002860_3C0200 D_090029A0_3C0340 D_09002B68_3C0508
  D_09002CE8_3C0688 D_09003148_3C0AE8 D_09003398_3C0D38 D_09003510_3C0EB0 D_09003730_3C10D0 D_09003778_3C1118
  D_09003790_3C1130 D_09003818_3C11B8.
- Counters they leave: their 16 Gfx arrays and the Vtx arrays they read stay in listcen data_arrays and in elfgate data.
  The native emit row (after the GOLD rows) takes them, together with a Vtx symbol merge in the gen_gfx_syms map (the split
  decomp symbols become one array).

## The old row text (for the record)
Row CONV2MIX: a triangle that mixes vertex arrays gets one generated vertex array, so 16 lists get v2 tables

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main bb9d5483e, CONV2C hand-back)
- 16 of the 121 unsupported lists stop at 3ds/tools/nd_gfx_conv.c:285 `if (r) { v2_unsup("vertex slots (nd_vslot_resolve)", ...)`:
  nd_vslot_resolve (CONV2B) refuses a triangle whose 3 slots hold vertices of two arrays (example D_09002860_3C0200).
- NDN_DRAW holds one vertex base (p) and an index range: one draw reads one array.
Start: `sh 3ds/tools/wt_new.sh CONV2MIX`. PM_ROW=CONV2MIX PM_LANE=CONV2MIX on every call. Waits for CONV2MTX (same file).
## Touch only (places)
3ds/tools/nd_gfx_conv.c: the resolve block (:270-295). Not nd_vslot.c (its refusal stays: the converter handles the case).
## Exact edits
1. When nd_vslot_resolve refuses with a mixed triangle, the converter makes one generated array for that list: `static const Vtx
   ndv_<list>[]` in nd_gfx_tab.c, the copies of every vertex the list loads, in load order (the Vtx bytes of the source arrays,
   copied at generation time). It resolves the slots again against that one array (each VTX op becomes a load from the generated
   array at its new first index) and writes the NDN_DRAW with p = ndv_<list>. Only the 16 refused lists use this.
2. Commit: `CONV2MIX: mixed-array triangles draw from one generated vertex array per list`.
## Gate (expected values)
1. nd_host PASS with `ntab ... unsup` 16 lower than before this row and `bad 0` (the compare checks the triangles as vertex
   triples, so the copied array gives the same positions). Seeded fault first (a value change): copy one vertex with x + 1: bad
   above 0. Then restore.
2. Build, memcheck OK (the generated arrays add data: hand back the memcheck margin). No Azahar.
## Stop and end
A refused list that is not a mixed triangle (hand back its reason), bad above 0, or a tool fault: stop, commit, hand back the
exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
