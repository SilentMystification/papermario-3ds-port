#!/bin/sh
# BLANKET: INCLUDE_IMG(.., N(x)) names NAMESPACE_x (IMGNS_RULE), and a twin also scans its first-line include when that is not its device source (TOPSCAN_RULE, dro_02/npc_merlee.c). Fixture in /tmp/blanket. Seeded faults: each rule off must drop its symbol.
R=$(pwd); W=/tmp/blanket; rm -rf $W && mkdir -p $W/t $W/3ds/tools/gfxin/t $W/o $W/f
printf '#define NAMESPACE tb\n' > $W/t/tb.h
printf '#include "tb.h"\n#include "part.c"\n#include "blk.inc.c"\n' > $W/t/top.c
printf '#include "tb.h"\nGfx part_gfx[] = {\n    gsSPEndDisplayList(),\n};\n' > $W/t/part.c
printf 'Vtx N(blk_vtx)[1] = {};\nINCLUDE_IMG("x.png", N(blk_img));\n' > $W/t/blk.inc.c
printf '#include "t/top.c"\n' > $W/3ds/tools/gfxin/t/part.c
(cd $W && python3 "$R/3ds/tools/gen_gfx_syms.py" o/gconv_in.c 3ds/tools/gfxin/t/part.c > /dev/null) || { echo "blanket FAIL gen"; exit 1; }
T=$W/o/gconv_tu_3ds_tools_gfxin_t_part_c.c
grep -q '"part_gfx", GS_GFX' $T || { echo "blanket FAIL part_gfx"; exit 1; }
grep -q '"tb_blk_img", GS_IMG' $T || { echo "blanket FAIL tb_blk_img"; exit 1; }
grep -q '"tb_blk_vtx", GS_VTX' $T || { echo "blanket FAIL tb_blk_vtx"; exit 1; }
# seeded fault IMGNS: IMGNS_RULE off drops tb_blk_img
sed 's/^IMGNS_RULE = True/IMGNS_RULE = False/' "$R/3ds/tools/gen_gfx_syms.py" > $W/gen_fault_img.py
cmp -s $W/gen_fault_img.py "$R/3ds/tools/gen_gfx_syms.py" && { echo "blanket FAIL IMGNS seeded fault not applied"; exit 1; }
(cd $W && python3 gen_fault_img.py f/gconv_in.c 3ds/tools/gfxin/t/part.c > /dev/null 2>&1)
T2=$W/f/gconv_tu_3ds_tools_gfxin_t_part_c.c
grep -q '"tb_blk_img", GS_IMG' $T2 2>/dev/null && { echo "blanket FAIL IMGNS seeded fault not seen"; exit 1; }
# seeded fault TOPSCAN: TOPSCAN_RULE off drops tb_blk_vtx (the blanket vtx of the first-line include)
sed 's/^TOPSCAN_RULE = True/TOPSCAN_RULE = False/' "$R/3ds/tools/gen_gfx_syms.py" > $W/gen_fault_top.py
cmp -s $W/gen_fault_top.py "$R/3ds/tools/gen_gfx_syms.py" && { echo "blanket FAIL TOPSCAN seeded fault not applied"; exit 1; }
(cd $W && python3 gen_fault_top.py f/gconv_in.c 3ds/tools/gfxin/t/part.c > /dev/null 2>&1)
T3=$W/f/gconv_tu_3ds_tools_gfxin_t_part_c.c
grep -q '"tb_blk_vtx", GS_VTX' $T3 2>/dev/null && { echo "blanket FAIL TOPSCAN seeded fault not seen"; exit 1; }
echo "blanket ok"
