#!/bin/sh
# NMDL0: the model record writer and the texture bind lists of src/model.c are built without gbi macros. (1) Each O form added for model.c
# equals its gbi macro. (2) make_texture_gfx and SolidCombineModes, extracted from src/model.c (the real code), give the same units as the frozen
# gbi-macro form (3ds/tests/nmdl0/old_mtg.c) for every format, size, palette, aux and combine case. Prints `nmdl0 ok <n>`. Seeded fault: one
# changed unit field in the extracted copy must print FAIL.
FP="-msse2 -mfpmath=sse -ffp-contract=off"
FL="-m32 -w -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -I src -I assets/us -DVERSION=us"
RO=/tmp # HOSTD2: read-only inputs that nd_host_test.sh makes before the host.d loop; the batch rule does not count $RO paths
LIBS="3ds/tools/nd_xf_link_stub.c 3ds/src/nd_xf.c 3ds/src/nd_rec.c 3ds/tools/nd_gx_host.c 3ds/src/nd_ds.c 3ds/src/nd_model.c 3ds/tools/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c 3ds/src/nd_texwin.c 3ds/tools/nd_host_stub.c $RO/ndcomb.o $RO/ndcombs.c $RO/ndlogs.c -lm"
N=/tmp/nmdl0; rm -rf $N; mkdir -p $N
tr -d '\r' < src/model.c > $N/model.txt
{
  printf '#include "ultra64.h"\n#include "variables.h"\n#include "pm_port.h"\n#include "gbi_custom.h"\n#include "model.h"\n#include "macros.h"\n#include "nd_gbi.h"\n#include "nd_at.h"\n#include "nd_tx.h"\n'
  awk '/supported values for auxCombineType/ {on=1} on {print} on && /TINT_COMBINE_REMAP/ {t=1} on && t && /^};/ {exit}' $N/model.txt
  awk '/^NdCmbW SolidCombineModes/ {on=1} on {print} on && /^};/ {exit}' $N/model.txt
  awk '/^void make_texture_gfx\(TextureHeader\* header/ {on=1} on {print} on && /^}/ {exit}' $N/model.txt
} > $N/new_mtg.c
grep -q "NDTX_LOAD_BLOCK" $N/new_mtg.c && grep -q "^void make_texture_gfx" $N/new_mtg.c && grep -q "^NdCmbW SolidCombineModes" $N/new_mtg.c || { echo "nmdl0 FAIL extract"; exit 1; }
build() { # $1 = new_mtg.c, $2 = output
  gcc $FL $FP -I 3ds/tools -o $2 3ds/tools/nd_nmdl0_test.c $1 3ds/tests/nmdl0/old_mtg.c $LIBS
}
# seeded fault: the texture level of the mipmap texture unit (the G_TEXTURE unit b field) is changed in the extracted copy
sed 's/0xFFFF, 0xFFFF, lod - 1, G_TX_RENDERTILE, G_ON/0xFFFF, 0xFFFE, lod - 1, G_TX_RENDERTILE, G_ON/' $N/new_mtg.c > $N/seed.c
cmp -s $N/seed.c $N/new_mtg.c && { echo "nmdl0 seeded fault not made"; exit 1; }
build $N/seed.c $N/seed_t || { echo "nmdl0 FAIL seed build"; exit 1; }
got=$($N/seed_t | head -1)
case "$got" in "nmdl0 FAIL mtg"*) ;; *) echo "nmdl0 seeded fault not seen: [$got]"; exit 1;; esac
build $N/new_mtg.c $N/t || exit 1
$N/t || exit 1
