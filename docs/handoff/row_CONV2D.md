# Row CONV2D (part 1 of 2): each v2 table carries the typed units of its list (NDN_UNIT), so the runner (CONV2D2) is exact by construction

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Rewritten by ORACLE8 on 2026-11-13: the design is fixed below.)
## Design decisions (ORACLE8 code read, main 0250d5221). Do not change them.
- No pso apply in CONV2D. The NdPsoDesc of a v2 table is not a full state. The converter tracks omh, oml and geom from 0 (V2St,
  nd_gfx_conv.c:181-189). Only comb, cycle and texon must be set (v2_inherit_check :356-366). NDO_TEXTURE drops the scale (:392).
  NdBindDesc has no tile shift and no TLUT slot. An apply of the desc (nd_direct_bind :94-140 setters) would overwrite the render
  mode and geometry bits of the call and lose the texture scale, so the drec would differ. The native emit from NdPsoOut waits for
  a form with set masks (a later row).
- The runner runs the list's own typed units through nd_walk_state, the executor that gt_ops (nd_walk.c:516-545) uses today. So
  there is no walker handover: nd_ws after the table is what the v1 path leaves, and the next walker draw sees the same state.
- The units are a copy of the top-level units of the list (depth 0). An NDO_CALL unit stays a unit and is not inlined, so the walker
  takes its G_DL arms (ent_gdl, dlc_try), as gt_call does. NDN_PSO, NDN_TEX, NDN_XF, NDN_COL and NDN_DRAW stay for the host
  compare and the later native emit. The runner skips them.
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
## Gate (expected values)
1. Seeded fault first (a value change): in v2_unit_add store `w1 ^ 1u` for NDO_GEOM. nd_host: `ntab ... bad` above 0 with
   "unit stream" lines. Undo it.
2. nd_host PASS: `ntab tables 923 draws 6245 bad 0` and `ntab inherit 426 unsup 23 descs 105` (unchanged). Build, memcheck OK
   (hand back the memcheck static and margin numbers: the copy adds 8 bytes per unit).
3. No Azahar (the device does not read NDN_UNIT before CONV2D2). Counter: none moves (runner row).
## Stop and end
A table count other than 923, unsup other than 23, bad above 0 with the real edit, memcheck FAIL, or a tool fault: stop, commit,
hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
