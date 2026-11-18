# LINKFAST/BUILDO: seeded faults of the romfs copy in 3ds/build.sh (the block "LINKFAST: the romfs in the build volume" .. "stage romfs").
# The source is a shape entry /sh/<key> (key.txt, out.sha1, f/). Cases: first copy, hit, a changed byte in the workspace copy,
# a new key, PM_CACHE=off, a missing entry (exit 8), a changed byte in the entry (exit 8).
T=/tmp/romfscache; rm -rf $T; mkdir -p $T
awk '/LINKFAST: the romfs in the build volume/{f=1} f{print} /stage romfs/{exit}' 3ds/build.sh | sed -e "s|/work|$T/work|g" -e "s|/sh/|$T/sh/|g" > $T/frag.sh
grep -q "stage romfs" $T/frag.sh || { echo "romfscache FAIL: the romfs block is missing in 3ds/build.sh"; exit 1; }
mkent() { # mkent <name> <a> <b>: an entry; prints its key
    printf 'key %s' "$1" > $T/k.txt; e=$(sha1sum < $T/k.txt | cut -c1-40); mkdir -p $T/sh/$e/f
    cp $T/k.txt $T/sh/$e/key.txt; printf '%s' "$2" > $T/sh/$e/f/a.nds; printf '%s' "$3" > $T/sh/$e/f/b.nds
    (cd $T/sh/$e && sha1sum f/* > out.sha1); echo $e
}
k1=$(mkent one a b); k2=$(mkent two a b)
run() { ( set -e -f; stage() { :; }; SHAPE_E=$1; PM_CACHE_V=$2; mkdir -p $T/work/build; touch $T/work/build/pm_3ds.3dsx; . $T/frag.sh ) >/dev/null 2>&1; }
want() { # want <case> <3dsx kept 1|0>
    k=0; [ -f $T/work/build/pm_3ds.3dsx ] && k=1
    [ "$k" = "$2" ] || { echo "romfscache FAIL $1: 3dsx kept $k, want $2"; exit 1; }
    [ "$(cat $T/work/romfs/shape/a.nds)$(cat $T/work/romfs/shape/b.nds)" = ab ] || { echo "romfscache FAIL $1: the copy differs from the source"; exit 1; }
}
run $k1 on || { echo "romfscache FAIL first: exit $?"; exit 1; }; want first 0
run $k1 on; want hit 1
printf X > $T/work/romfs/shape/b.nds; run $k1 on; want byte 0
run $k2 on; want key 0
run $k2 off; want off 0
run $k2 on; want hit2 1
run 0000000000000000000000000000000000000000 on; rc=$?
[ "$rc" = 8 ] || { echo "romfscache FAIL missing entry: exit $rc, want 8"; exit 1; }
printf X >> $T/sh/$k1/f/a.nds; run $k1 on; rc=$?
[ "$rc" = 8 ] || { echo "romfscache FAIL changed entry byte: exit $rc, want 8"; exit 1; }
rm -rf $T
echo "romfscache ok (first, hit, byte, key, off, missing entry, entry byte)"
