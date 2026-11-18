#!/bin/sh
# IMGFX: the build-time anim units (gen_imgfx_tab.py) as a host table. The seed (END as TRI1) must fail case 0.
F="-I 3ds/include"
T=/tmp/imgfxtab
mkdir -p $T/seed
python3 3ds/tools/gen_imgfx_tab.py $T/nd_imgfx_tab.c assets/us/imgfx || { echo "imgfxtab FAIL gen"; exit 1; }
sed 's/ND_FS_W0(ND_FS_END, 0x000000)/ND_FS_W0(ND_FS_TRI1, 0x000000)/' $T/nd_imgfx_tab.c > $T/seed/nd_imgfx_tab.c
gcc -m32 -w $F -o $T/seed/t $T/seed/nd_imgfx_tab.c 3ds/tools/nd_imgfx_tab_test.c || exit 1
got=$($T/seed/t); if [ "$got" != "imgfxtab FAIL case 0" ]; then echo "imgfxtab seed FAIL: got [$got]"; exit 1; fi
gcc -m32 -Wall -Wextra -Werror $F -o $T/t $T/nd_imgfx_tab.c 3ds/tools/nd_imgfx_tab_test.c || exit 1
$T/t || exit 1
