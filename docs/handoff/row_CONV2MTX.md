# Row CONV2MTX: matrix symbols (Mtx objects) in the converter's symbol map, so 21 lists with a matrix get v2 tables

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main bb9d5483e, CONV2C hand-back)
- 21 of the 121 unsupported lists stop at 3ds/tools/nd_gfx_conv.c:383 `if ((w1 >> 24) >= 16u) { v2_unsup("matrix address outside the
  symbol map", k); break; }`: the G_MTX address is a host address of an entity Mtx object, not a segment address.
- 3ds/tools/gen_gfx_syms.py:10 `DEF = re.compile(r"^(Gfx|Vtx)\s+(\w+)\s*\[[^\]]*\]\s*=", re.M)` scans only Gfx and Vtx arrays,
  and :51 writes GS_GFX or GS_VTX (GS_IMG for images). sym_of(addr, kind, &off) (nd_gfx_conv.c:83, :90) maps an address to a
  symbol of a kind. An Mtx object is `Mtx NAME = {...}` (not always an array).
- build.sh gen_gfx_tab stamp list GT_IN (:86) does not list 3ds/tools/nd_vslot.c (the converter links it since CONV2C).
Start: `sh 3ds/tools/wt_new.sh CONV2MTX`. PM_ROW=CONV2MTX PM_LANE=CONV2MTX on every call. Waits for CONV2WIN1 (same file).
## Touch only (places)
3ds/tools/gen_gfx_syms.py (:10 and :49-52), 3ds/tools/nd_gconv.h (the GS_ kind enum), 3ds/tools/nd_gfx_conv.c :383 (the
matrix branch), 3ds/build.sh the GT_IN line (:86).
## Exact edits
1. gen_gfx_syms.py: a second regex `MTX = re.compile(r"^Mtx\s+(\w+)\s*(\[[^\]]*\])?\s*=", re.M)`, its names added with kind
   "Mtx" and written as GS_MTX entries (address and end by `sizeof`). nd_gconv.h: GS_MTX next to GS_VTX.
2. nd_gfx_conv.c :383: when `(w1 >> 24) >= 16u`, look the address up with sym_of(w1, GS_MTX, &off). Found: NDN_XF with p = the
   symbol address + off (the table names the Mtx object). Not found: the v2_unsup line as today.
3. build.sh :86: add 3ds/tools/nd_vslot.c to GT_IN.
4. Commit: `CONV2MTX: Mtx symbols for the converter, matrix ops of entity lists, GT_IN lists nd_vslot.c`.
## Gate (expected values)
1. nd_host PASS with `ntab ... unsup` 21 lower than before this row and `bad 0`. Seeded fault first (a value change): add 64 to
   the matrix symbol address in edit 2: bad above 0. Then restore.
2. Build, memcheck OK. A second build with nd_vslot.c touched rebuilds nd_gfx_tab.c (the stamp line changes). No Azahar.
## Stop and end
A matrix address that no Mtx symbol holds (hand back the list), bad above 0, or a tool fault: stop, commit, hand back the exact
output (CLAUDE.md 4-5). Delete this row file in the completing commit.
