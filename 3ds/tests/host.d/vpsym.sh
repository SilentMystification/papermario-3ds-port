#!/bin/sh
# THEATVP: gen_gfx_syms.py maps a top-level `Vp NAME =` object (the address of gsSPViewport, e.g. src/curtains.c TheaterViewport)
# as GS_VP with its sizeof end. Seeded fault: VP_RULE off drops the entry.
R=$(pwd); W=/tmp/vpsym; rm -rf $W && mkdir -p $W/a $W/o $W/f
printf 'Vp vp_a = {\n    .vp = { { 640, 480, 511, 0 }, { 640, 480, 511, 0 } }\n};\nGfx dl_a[] = {\n    gsSPEndDisplayList(),\n};\n' > $W/a/x.c
(cd $W && python3 "$R/3ds/tools/gen_gfx_syms.py" o/gconv_in.c a/x.c > /dev/null) || { echo "vpsym FAIL gen"; exit 1; }
grep -q '{ "vp_a", GS_VP, (const void\*)&vp_a, (const void\*)((const char\*)&vp_a + sizeof vp_a) },' $W/o/gconv_tu_a_x_c.c || { echo "vpsym FAIL no vp_a entry"; exit 1; }
grep -q '"dl_a", GS_GFX' $W/o/gconv_tu_a_x_c.c || { echo "vpsym FAIL no dl_a entry"; exit 1; }
sed 's/^VP_RULE = True/VP_RULE = False/' "$R/3ds/tools/gen_gfx_syms.py" > $W/gen_fault.py
cmp -s $W/gen_fault.py "$R/3ds/tools/gen_gfx_syms.py" && { echo "vpsym FAIL seeded fault not applied"; exit 1; }
(cd $W && python3 gen_fault.py f/gconv_in.c a/x.c > /dev/null 2>&1)
grep -q '"vp_a", GS_VP' $W/f/gconv_tu_a_x_c.c 2>/dev/null && { echo "vpsym FAIL seeded fault not seen"; exit 1; }
echo "vpsym ok"
