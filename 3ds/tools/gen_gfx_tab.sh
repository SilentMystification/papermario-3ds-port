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
# GFXINC: the TU files go to one fixed folder (/tmp/gct, the container's own /tmp), so every caller compiles the same paths and
# shares the object cache entries (the key holds the source path). $T gets the joined object gconv_in.o as before.
TD=/tmp/gct; rm -rf "$TD"; mkdir -p "$TD"
TUS=$(python3 3ds/tools/gen_gfx_syms.py "$TD/gconv_in.c" $(for g in $GCONV_SRC; do case $g in *.c) p=$g;; *) p=src/effects/gfx/$g.c;; esac; [ -f "3ds/tools/gfxin/$p" ] && p="3ds/tools/gfxin/$p"; echo "$p"; done))
# GCONVW: the shared file and each own-TU file are separate objects; ld -r joins them (gconv_sym section). GCONVSPLIT: every
# source is an own-TU file (gconv_tu_<path>.c); gconv_in.c holds no source.
# GFXINC: 3ds/tools/gconv_objcache.py compiles them (16 at once): a hit copies the stored object (GCONV_OBJ, the docker volume
# pm3ds-gconvobj-<host image id>; unset: no cache). The objects and the link order are the ones of the old loop.
python3 3ds/tools/gconv_objcache.py --flags "-m32 -w -ffunction-sections -fdata-sections $DEF -DND_GCONV_IN $INC" "$TD/gconv_in.c" $TUS
TUOBJ=""
for f in $TUS; do TUOBJ="$TUOBJ ${f%.c}.o"; done
gcc -m32 -r -nostdlib -o "$T/gconv_in.o" "$TD/gconv_in.o" $TUOBJ
gcc -m32 -Wall -Wextra -Werror -DND_SCOMP_HOST $DEF $TINC -o "$T/nd_gfx_conv" 3ds/tools/nd_gfx_conv.c 3ds/src/nd_scomp.c 3ds/src/nd_texwin.c 3ds/tools/nd_vslot.c "$T/gconv_in.o" -Wl,--gc-sections
python3 3ds/tools/gen_ntab_entries.py src/effects $(cat $(ls 3ds/tools/ntab_ent.d/*.txt | LC_ALL=C sort) | sed 's/#.*//' | tr -s ' \t\r\n' ' ') > "$T/ntab_entries.txt" # ENTSRC: plus the files of ntab_ent.d
# MODELRM: the lists of nd_rmdef.h also get a static NdSDef (ndsd_<list>)
sed -n 's/^ *X(\(Gfx_RM[A-Za-z0-9_]*\)).*/\1/p' 3ds/include/nd_rmdef.h > "$T/sdef_names.txt"
"$T/nd_gfx_conv" "$T/nd_ntab_descs.c" "$T/ntab_inherit.txt" "$T/ntab_unsup.txt" "$T/nd_ntab_idx.c" "$T/ntab_tables.txt" "$T/ntab_entries.txt" "$T/ntab_pairs.txt" "$T/gset_ok.txt" "$T/sdef_names.txt" > "$T/nd_gfx_tab.c"
[ "$T/nd_gfx_tab.c" -ef "$OUTC" ] || cp "$T/nd_gfx_tab.c" "$OUTC"
# CONV2C: the pso descriptions next to the table file (gen_pso_tab.py scans them), the logs of the lists without a v2 table in build3ds/out
OUTD=$(dirname "$OUTC")
[ "$T/nd_ntab_descs.c" -ef "$OUTD/nd_ntab_descs.c" ] || cp "$T/nd_ntab_descs.c" "$OUTD/nd_ntab_descs.c"
for lg in ntab_inherit.txt ntab_unsup.txt ntab_tables.txt ntab_entries.txt ntab_pairs.txt gset_ok.txt; do [ -d /w/build3ds/out ] && cp "$T/$lg" /w/build3ds/out/$lg || true; done
