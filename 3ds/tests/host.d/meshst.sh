#!/bin/sh
# MESHST: nd_mesh_build (nd_mesh.h) copies an animator mesh draw like the old per-frame copy (animator.c:94-107). The seed (no base offset per load) must fail case 0.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I src -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
mkdir -p /tmp/meshst_seed
sed 's/(vso + (int)vi) \* 6u/(vso) * 6u/' 3ds/include/nd_mesh.h > /tmp/meshst_seed/nd_mesh.h
gcc -m32 -w -I /tmp/meshst_seed $F -o /tmp/meshst_seed/t 3ds/tools/nd_mesh_test.c || exit 1
got=$(/tmp/meshst_seed/t); if [ "$got" != "mesh FAIL case 0" ]; then echo "meshst seed FAIL: got [$got]"; exit 1; fi
gcc -m32 -Wall -Wextra $F -o /tmp/meshst_t 3ds/tools/nd_mesh_test.c && /tmp/meshst_t || exit 1
