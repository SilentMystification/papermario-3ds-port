#!/bin/sh
# seamlint.sh [--seed]: numeric path rule N1 to N3 (docs/handoff/plan_seams.md section 4). Exit 0 OK, 1 FAIL. --seed adds one site and one shader line in memory: it must FAIL.
# A new site is not an error by itself: an oracle reviews it, then updates WANT and the counts here in the same commit.
cd "$(dirname "$0")/../.." || exit 2
seed=0; [ "$1" = --seed ] && seed=1; fail=0
RE='f32tof24\(|Mtx_Multiply\(|nd_mtx_pcomp\(|nd_mtx_cmul\(|nd_mtx_mul\(|FLOATUNIFORM_CONFIG|C3D_FVUnif(Set|Mtx4x4)\('
WANT="3ds/include/nd.h=3 3ds/src/nd_mtx.c=8 3ds/src/nd_tmpl.c=3 3ds/src/pm_gpu.c=14 "
sites=$( { grep -nE "$RE" 3ds/src/*.c 3ds/src/*.h 3ds/include/*.h; [ $seed = 1 ] && echo "3ds/src/nd_emit.c:1:f32tof24(seed)"; } | cut -d: -f1 | LC_ALL=C sort | uniq -c | awk '{printf "%s=%s ", $2, $1}')
[ "$sites" = "$WANT" ] || { echo "seamlint FAIL sites: $sites(want $WANT)"; fail=1; }
up=$(grep -nE 'one\(o, 0x2C0|0x2C0u \| 0xF|FLOATUNIFORM_CONFIG' 3ds/src/*.c)
nu=$(printf '%s\n' "$up" | grep -c .); nf=$(printf '%s\n' "$up" | grep -c 0x80000000)
{ [ "$nu" = 3 ] && [ "$nf" = 3 ]; } || { echo "seamlint FAIL uniform writes $nu, f32 mode $nf (want 3 and 3)"; fail=1; }
blk() { tr -d '\r' < "$1" | sed -n '/mov r1.xyz, inpos/,/mov outpos.w, r5.w/p' | sed -e 's/;.*$//' -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' -e 's/pal5\[/pal[/g' -e 's/myc1\./myconst./g' | grep -v '^$'; }
b4=$(blk 3ds/shaders/pm4.v.pica); b5=$(blk 3ds/shaders/pm5.v.pica)
[ $seed = 1 ] && b5="$b5
dp4 r5.x, pal[a0.x], r1"
n4=$(printf '%s\n' "$b4" | grep -c .)
{ [ "$b4" = "$b5" ] && [ "$n4" = 23 ]; } || { echo "seamlint FAIL position block: pm4 $n4 lines, pm4 and pm5 differ or not 23"; fail=1; }
[ $fail = 0 ] && echo "seamlint OK sites $sites uniform f32 $nf block $n4"
exit $fail
