#!/bin/sh
# WCLIP1: nd_sci_x (one scissor x rule, plan_widewin.md 4). The table and grid must pass; 4 seeds must each print FAIL.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
gcc -m32 -Wall -Wextra -Werror $F -o /tmp/wclip1_t 3ds/tools/nd_sci_test.c 3ds/src/nd_sci.c || exit 1
/tmp/wclip1_t || exit 1
mkdir -p /tmp/wclip1_seed
seed() {
    sed "$1" 3ds/src/nd_sci.c > /tmp/wclip1_seed/nd_sci.c
    cp 3ds/include/nd_sci.h /tmp/wclip1_seed/nd_sci.h
    gcc -m32 -w $F -I /tmp/wclip1_seed -o /tmp/wclip1_seed/t 3ds/tools/nd_sci_test.c /tmp/wclip1_seed/nd_sci.c || exit 1
    out=$(/tmp/wclip1_seed/t 2>&1)
    case "$out" in *FAIL*) ;; *) echo "wclip1 seed PASS (no FAIL): $1"; exit 1 ;; esac
}
seed 's#(x1 == 319 || x1 == 320)#x1 >= 320#' || exit 1
seed 's#x0 == 0 ? 0#x0 <= 0 ? 0#' || exit 1
seed 's#x0 == 12 && y0 == 20#x0 == 13 \&\& y0 == 20#' || exit 1
seed 's#s ? ci(40 + x0#s ? ci(x0#' || exit 1
seed 's#const int s = ui;#const int s = mode == 1 \&\& ui;#' || exit 1
echo "wclip1 seeds: 5 FAIL"
