#!/bin/sh
# RAWIMG: gen_gfx_syms.py maps a top-level `unsigned char NAME[] =` array (a texture or palette written as C bytes, e.g.
# src/entity/model/HeartBlockContent.c) as GS_IMG with its sizeof end. Seeded fault: RAW_RULE off drops the entry.
R=$(pwd); W=/tmp/rawimg; rm -rf $W && mkdir -p $W/a $W/o $W/f
printf 'unsigned char img_a[] = {\n    1, 2, 3, 4,\n};\nGfx dl_a[] = {\n    gsSPEndDisplayList(),\n};\n' > $W/a/x.c
(cd $W && python3 "$R/3ds/tools/gen_gfx_syms.py" o/gconv_in.c a/x.c > /dev/null) || { echo "rawimg FAIL gen"; exit 1; }
grep -q '{ "img_a", GS_IMG, (const void\*)img_a, (const void\*)((const char\*)img_a + sizeof img_a) },' $W/o/gconv_tu_a_x_c.c || { echo "rawimg FAIL no img_a entry"; exit 1; }
grep -q '"dl_a", GS_GFX' $W/o/gconv_tu_a_x_c.c || { echo "rawimg FAIL no dl_a entry"; exit 1; }
sed 's/^RAW_RULE = True/RAW_RULE = False/' "$R/3ds/tools/gen_gfx_syms.py" > $W/gen_fault.py
cmp -s $W/gen_fault.py "$R/3ds/tools/gen_gfx_syms.py" && { echo "rawimg FAIL seeded fault not applied"; exit 1; }
(cd $W && python3 gen_fault.py f/gconv_in.c a/x.c > /dev/null 2>&1)
grep -q '"img_a", GS_IMG' $W/f/gconv_tu_a_x_c.c 2>/dev/null && { echo "rawimg FAIL seeded fault not seen"; exit 1; }
echo "rawimg ok"
