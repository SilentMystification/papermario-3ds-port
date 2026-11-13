#!/bin/sh
# gen_gfx_tab.sh <out.c> [work dir]: the gfx table step (S7CONV, row design note 1). Runs inside docker pm3ds-host:1 with the repo layout of
# nd_host_test.sh at the cwd (3ds, include, src, assets/us, ver/us/build). Scans the converted gfx sources (gen_gfx_syms.py), compiles them
# for the host with the device defines, builds the tool nd_gfx_conv.c with the pure part of nd_scomp.c and writes the tables to <out.c>.
# CONV2C: also writes nd_ntab_descs.c (next to <out.c>), ntab_inherit.txt and ntab_unsup.txt (build3ds/out), nd_ntab_idx.c (work dir, host compare). The work dir keeps gconv_in.o for the host compare (nd_gtab_test.c). GCONV_SRC: the effect_gfx_<g> sources of the set; S7 rows extend it.
set -e
# a row adds its sources as 3ds/tools/gconv_src.d/<row>.txt; no shared line
GCONV_SRC=$(cat $(ls 3ds/tools/gconv_src.d/*.txt | LC_ALL=C sort) | sed 's/#.*//' | tr -s ' \t\r\n' ' ')
OUTC=$1
T=${2:-/tmp/gconv}
mkdir -p "$T"
DEF="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -DVERSION=us -D_MIPS_SZLONG=32 -Dnullptr=NULL"
TINC="-I 3ds/tools -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include" # the nd_host order
INC="-I 3ds/tools/gconv_inc -I 3ds/tools -I 3ds/include -I 3ds/stub_include -I include -I src -I assets/us -I ver/us/build/include -I ."
python3 3ds/tools/gen_gfx_syms.py "$T/gconv_in.c" $(for g in $GCONV_SRC; do case $g in *.c) echo "$g";; *) echo "src/effects/gfx/$g.c";; esac; done)
gcc -m32 -w $DEF $INC -c -o "$T/gconv_in.o" "$T/gconv_in.c"
gcc -m32 -Wall -Wextra -Werror -DND_SCOMP_HOST $DEF $TINC -o "$T/nd_gfx_conv" 3ds/tools/nd_gfx_conv.c 3ds/src/nd_scomp.c 3ds/src/nd_texwin.c 3ds/tools/nd_vslot.c "$T/gconv_in.o"
"$T/nd_gfx_conv" "$T/nd_ntab_descs.c" "$T/ntab_inherit.txt" "$T/ntab_unsup.txt" "$T/nd_ntab_idx.c" > "$T/nd_gfx_tab.c"
[ "$T/nd_gfx_tab.c" -ef "$OUTC" ] || cp "$T/nd_gfx_tab.c" "$OUTC"
# CONV2C: the pso descriptions next to the table file (gen_pso_tab.py scans them), the logs of the lists without a v2 table in build3ds/out
OUTD=$(dirname "$OUTC")
[ "$T/nd_ntab_descs.c" -ef "$OUTD/nd_ntab_descs.c" ] || cp "$T/nd_ntab_descs.c" "$OUTD/nd_ntab_descs.c"
for lg in ntab_inherit.txt ntab_unsup.txt; do [ -d /w/build3ds/out ] && cp "$T/$lg" /w/build3ds/out/$lg || true; done
