#!/bin/sh
# FXVROM: a vertex load past its array end splits in ROM order (nd_gfx_conv.c ROM_SPLIT). D_09002CE8_3C0688 (snowman_doll.c: a 30-vertex
# load on the 9-vertex D_090015E0_3BEF80) has a v2 table. Seeded fault: the converter built with -DROM_SPLIT=0 on the same objects
# (/tmp/gconv of nd_host_test.sh) reports the list unsup again. Prints `vrom ok` or fails.
T=/tmp/gconv; V=/tmp/vrom; rm -rf "$V"; mkdir -p "$V"
grep -qx D_09002CE8_3C0688 "$T/ntab_tables.txt" || { echo "vrom FAIL: D_09002CE8_3C0688 has no v2 table"; exit 1; }
DEF="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -DVERSION=us -D_MIPS_SZLONG=32 -Dnullptr=NULL"
gcc -m32 -Wall -Wextra -Werror -DROM_SPLIT=0 -DND_SCOMP_HOST $DEF -I 3ds/tools -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include \
    -o "$V/conv" 3ds/tools/nd_gfx_conv.c 3ds/src/nd_scomp.c 3ds/src/nd_texwin.c 3ds/tools/nd_vslot.c "$T/gconv_in.o" -Wl,--gc-sections
"$V/conv" "$V/d.c" "$V/i.txt" "$V/u.txt" "$V/x.c" "$V/t.txt" "$T/ntab_entries.txt" "$V/p.txt" "$V/g.txt" > "$V/tab.c" 2> "$V/err.txt" || true
grep -q "^D_09002CE8_3C0688: vertex ref outside the set" "$V/u.txt" || { echo "vrom seeded fault not seen"; exit 1; }
echo "vrom ok"
