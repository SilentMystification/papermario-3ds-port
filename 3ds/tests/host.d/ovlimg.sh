#!/bin/sh
# OVLIMG: D_80156910 (src/screen_overlays.c) is the 160 x 120 RGBA16 image of D_8014EA48: an unsigned char array of 0x9600 bytes with an
# initializer, so gen_gfx_syms.py maps it as GS_IMG (RAW rule) and D_8014EA48 gets an itab and is gset ok (/tmp/gconv of nd_host_test.sh).
# Seeded fault: a copy of src/screen_overlays.c with the old `BSS s32 D_80156910;` gives no GS_IMG entry.
W=/tmp/ovlimg; rm -rf $W && mkdir -p $W/o $W/f $W/src
grep -q "^D_8014EA48: itab (" /tmp/gconv/ntab_inherit.txt || { echo "ovlimg FAIL: D_8014EA48 has no itab"; exit 1; }
grep -qx D_8014EA48 /tmp/gconv/gset_ok.txt || { echo "ovlimg FAIL: D_8014EA48 is not gset ok"; exit 1; }
E='{ "D_80156910", GS_IMG, (const void*)D_80156910, (const void*)((const char*)D_80156910 + sizeof D_80156910) },'
grep -q '^unsigned char D_80156910\[0x9600\] = { 0 };' src/screen_overlays.c || { echo "ovlimg FAIL: D_80156910 is not 0x9600 bytes"; exit 1; }
python3 3ds/tools/gen_gfx_syms.py $W/o/gconv_in.c src/screen_overlays.c > /dev/null 2>&1 || { echo "ovlimg FAIL gen"; exit 1; }
grep -qF "$E" $W/o/gconv_tu_src_screen_overlays_c.c || { echo "ovlimg FAIL: no GS_IMG entry"; exit 1; }
sed 's/^unsigned char D_80156910\[0x9600\] = { 0 };.*/BSS s32 D_80156910;/' src/screen_overlays.c > $W/src/screen_overlays.c
cmp -s $W/src/screen_overlays.c src/screen_overlays.c && { echo "ovlimg FAIL seeded fault not applied"; exit 1; }
python3 3ds/tools/gen_gfx_syms.py $W/f/gconv_in.c $W/src/screen_overlays.c > /dev/null 2>&1 || { echo "ovlimg FAIL gen of the seeded copy"; exit 1; }
cat $W/f/gconv_tu_*.c | grep -q '"vtx_stencil2", GS_VTX' || { echo "ovlimg FAIL seeded copy not scanned"; exit 1; }
cat $W/f/gconv_tu_*.c | grep -qF "$E" && { echo "ovlimg FAIL seeded fault not seen"; exit 1; }
echo "ovlimg ok"
