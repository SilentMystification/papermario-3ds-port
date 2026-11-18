#!/bin/sh
# FLTGT: a colour image unit (gsDPSetColorImage) of an image symbol is a unit only (nd_gfx_conv.c CIMG_UNIT), so the flame lists
# D_090009E0_354590 and D_09000800_3543B0 (3ds/tools/gfxin/src/effects/gfx/flame.c) have v2 tables and are gset ok. Seeded fault: the
# converter built with -DCIMG_UNIT=0 on the same objects (/tmp/gconv of nd_host_test.sh) reports D_090009E0_354590 unsup again.
T=/tmp/gconv; V=/tmp/fltgt; rm -rf "$V"; mkdir -p "$V"
for l in D_090009E0_354590 D_09000800_3543B0; do
  grep -qx "$l" "$T/gset_ok.txt" || { echo "fltgt FAIL: $l is not gset ok"; exit 1; }
  grep -qx "$l" "$T/ntab_tables.txt" || { echo "fltgt FAIL: $l has no v2 table"; exit 1; }
done
DEF="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -DVERSION=us -D_MIPS_SZLONG=32 -Dnullptr=NULL"
gcc -m32 -Wall -Wextra -Werror -DCIMG_UNIT=0 -DND_SCOMP_HOST $DEF -I 3ds/tools -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include \
    -o "$V/conv" 3ds/tools/nd_gfx_conv.c 3ds/src/nd_scomp.c 3ds/src/nd_texwin.c 3ds/tools/nd_vslot.c "$T/gconv_in.o" -Wl,--gc-sections
"$V/conv" "$V/d.c" "$V/i.txt" "$V/u.txt" "$V/x.c" "$V/t.txt" "$T/ntab_entries.txt" "$V/p.txt" "$V/g.txt" "$T/sdef_names.txt" > "$V/tab.c" 2> "$V/err.txt" || true
grep -q "^D_090009E0_354590: MISC (unit 0 of D_090009E0_354590)" "$V/u.txt" || { echo "fltgt seeded fault not seen"; exit 1; }
echo "fltgt ok"
