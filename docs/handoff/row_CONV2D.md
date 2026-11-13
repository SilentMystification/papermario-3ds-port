# Row CONV2D (part 1 of 2): each v2 table carries the typed units of its list (NDN_UNIT), so the runner (CONV2D2) is exact by construction

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Rewritten by ORACLE8 on 2026-11-13: the design is fixed below.)
## Design decisions (ORACLE8 code read, main 0250d5221). Do not change them.
- No pso apply: the table's NdPsoDesc is not a full state (omh, oml, geom tracked from 0, nd_gfx_conv.c:181-189; no texture
  scale, :392). The runner runs the list's own typed units through nd_walk_state, as gt_ops (nd_walk.c:516-545) does, so there
  is no walker handover. The units are the depth-0 units (a call stays a unit, so the walker takes its G_DL arms). NDN_PSO, TEX,
  XF, COL and DRAW stay for the host compare and the later native emit. The runner skips them.
## Facts
- nd_gtab.h:76-83 enum (NDN_END .. NDN_CALL). nd_gfx_conv.c: V2_MAX defines :174-179, V2C struct :190-201, v2_walk :380 (NOP skip
  `if (w0 == ND_NOP_A) continue;` :387, `depth` is its argument), v2_emit_table :510-530 (ops array printed at :524-528),
  v2_sym_expr :222 gives `(const char*)NAME + off`. Address w1 kinds: NDO_VTX (GS_VTX), NDO_TIMG (GS_IMG), NDO_CALL (GS_GFX),
  NDO_MTX (GS_MTX). Each is an address only when `w1 && (w1 >> 24) >= 16u`. Else w1 is a number (segment address or 0).
- nd_gtab_test.c v2_compare :471 (m_walk :482, v_replay :483). The test binary links gconv_in.o (the lists) and nd_gfx_tab.o, so
  a list address and its `&NAME` expression are equal there.
Start: `sh 3ds/tools/wt_new.sh CONV2D`. PM_ROW=CONV2D PM_LANE=CONV2D on every call.
## Touch only (places)
3ds/include/nd_gtab.h (the enum), 3ds/tools/nd_gfx_conv.c (V2C struct, v2_walk top, v2_emit_table), 3ds/tools/nd_gtab_test.c
(v2_compare). CONV2MIX (resolve block :270-295) is parked.
## Exact edits
1. nd_gtab.h: after NDN_CALL add `NDN_UNIT` with the comment `p = const uint32_t* the depth-0 typed units of the list (word
   pairs, NOPs and the end left out, a call stays a unit), b = their count; the first op of every table; CONV2D2 runs them with
   nd_walk_state`.
2. nd_gfx_conv.c V2C: `uint32_t uw[2u * V2_MAXOPS]; char ue[V2_MAXOPS][160]; unsigned nu;` (ue[i][0] == 0: no address).
3. v2_walk, after the NOP skip: `if (depth == 0u && id != NDO_END) v2_unit_add(k, id, w0, w1);`. v2_unit_add (new static, before
   v2_walk): unsup "too many units" when nu == V2_MAXOPS. It stores w0 and w1. For the 4 address kinds it calls v2_sym_expr with
   the kind (for NDO_MTX it writes `(const char*)&NAME + off` itself, as :404-405 do: an Mtx is not an array), puts the
   text in ue[nu] and sets uw[2nu+1] = 0. On a miss it calls `v2_unsup("unit address outside the symbol map", k)`.
4. v2_emit_table, before the ops array: `static const uint32_t ndnu_<n>[<nu or 1>] = { ... };`. Each unit prints as
   `0x%xu, 0x%xu,`, or as `0x%xu, (uint32_t)(uintptr_t)(<ue>),` when ue is set. The ops array then starts with
   `{ NDN_UNIT, 0, <nu>, 0, ndnu_<n>, 0 },` before the other ops (op count + 1 in the NdNTab line).
5. nd_gtab_test.c v2_compare, after v_replay: the expected stream is the list words from t->list at k = 0. Skip ND_NOP_A. Stop
   before NDO_END. Stop after an NDO_CALL with `(w0 & 1u)` (a branch). t->ops[0] must be NDN_UNIT with b equal to the count, and
   every pair must be equal (`v2_fail("unit stream", name, i)` on the first difference).
6. Commit: `CONV2D: v2 tables carry the depth-0 typed units of their list (NDN_UNIT), host unit stream compare`.
## Correction 1 (ORACLE8, 2026-11-13, after the stop at 41718cf81). Resume from 41718cf81: edits 1-5 are there.
- The TIMG miss is right, and the counts change. Entity_HeartBlockContent_LoadPalette (src/entity/model/HeartBlockContent.c:27-36)
  unit 1 loads D_0A000600_E4A440, an `unsigned char[]` that the GS_IMG map does not hold (gen_gfx_syms.py scans only
  INCLUDE_IMG/PAL/RAW). A unit copy has no form for an address without a symbol, so the list has no v2 table. It costs nothing:
  its 8 units are all state ops (gt_kind, nd_gtab.c:50-56), so nd_rec_dl flags it state-only and nd_gtab_run runs it in the
  state-only arm, which never reaches gt_ops or nd_ntab_run. New expected values: tables 922, unsup 24, descs 105,
  triangles 6245. D_09005090_3EAFC0 keeps its unsup with a new first reason (same count).
- The stop rule. gt_ops (nd_walk.c:536-538) ends a list only at its own branch (`if (o->a & 1u) return;`). A callee's branch
  does not end the caller. v2_walk instead returns at `(a & 1u) || term`, so units emitted inside v2_walk stop early when a
  callee branches. Edit 7: delete the line `if (depth == 0u && id != NDO_END) v2_unit_add(k, id, w0, w1);` from v2_walk. Add
  `static void v2_units_top(const GSym* g)` before v2_walk. It loops k from 0 over the units of g, skips ND_NOP_A, stops at
  NDO_END, calls v2_unit_add(k, id, w0, w1), and stops after an NDO_CALL with `(w0 & 1u)`. This is the rule of v2_units in
  nd_gtab_test.c. Call `v2_units_top(g);` on the line before `v2_walk(g, 0, 0);` (:580). (Finding for the native emit row: the
  `term` rule of v2_walk and m_walk differs from gt_ops. Do not change it in this row.)
- Externs: the unit texts use only names that nd_gfx_tab.c already declares. Add no extern line (the coder's revert is right).
- Commit edit 7 as `CONV2D: depth-0 units from their own loop (the gt_ops stop rule)`. Then the gate, from gate 1 (the seed).
## Gate (expected values)
1. Seeded fault first (a value change): in v2_unit_add store `w1 ^ 1u` for NDO_GEOM. nd_host: `ntab ... bad` above 0 with
   "unit stream" lines. Undo it.
2. nd_host PASS: `ntab tables 922 draws 6245 bad 0` and `ntab inherit 426 unsup 24 descs 105` (Correction 1). Build, memcheck OK
   (hand back the memcheck static and margin numbers: the copy adds 8 bytes per unit).
3. No Azahar (the device does not read NDN_UNIT before CONV2D2). Counter: none moves (runner row).
## Stop and end
A table count other than 922, unsup other than 24, bad above 0 with the real edit, memcheck FAIL, or a tool fault: stop, commit,
hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
