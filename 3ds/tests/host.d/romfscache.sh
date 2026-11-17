# LINKFAST: seeded faults of the romfs copy in 3ds/build.sh (the block "LINKFAST: the romfs in the build volume" .. "stage romfs").
# Cases: first copy, hit, one changed byte in the copy, a new shape key, PM_CACHE=off. Each miss copies again and removes the 3dsx.
T=/tmp/romfscache; rm -rf $T; mkdir -p $T/out/romfs/shape
printf 'a' > $T/out/romfs/shape/a.nds; printf 'b' > $T/out/romfs/shape/b.nds
awk '/LINKFAST: the romfs in the build volume/{f=1} f{print} /stage romfs/{exit}' 3ds/build.sh | sed -e "s|/work|$T/work|g" -e "s|/out/romfs|$T/out/romfs|g" > $T/frag.sh
grep -q "stage romfs" $T/frag.sh || { echo "romfscache FAIL: the romfs block is missing in 3ds/build.sh"; exit 1; }
run() { ( set -e -f; stage() { :; }; SHAPE_E=$1; PM_CACHE_V=$2; mkdir -p $T/work/build; touch $T/work/build/pm_3ds.3dsx; . $T/frag.sh ) || { echo "romfscache FAIL: the block failed (key $1, cache $2)"; exit 1; }; }
want() { # want <case> <3dsx kept 1|0>
    k=0; [ -f $T/work/build/pm_3ds.3dsx ] && k=1
    [ "$k" = "$2" ] || { echo "romfscache FAIL $1: 3dsx kept $k, want $2"; exit 1; }
    [ "$(cat $T/work/romfs/shape/a.nds)$(cat $T/work/romfs/shape/b.nds)" = ab ] || { echo "romfscache FAIL $1: the copy differs from the source"; exit 1; }
}
run k1 on; want first 0
run k1 on; want hit 1
printf X > $T/work/romfs/shape/b.nds; run k1 on; want byte 0
run k2 on; want key 0
run k2 off; want off 0
run k2 on; want hit2 1
rm -rf $T
echo "romfscache ok (first, hit, byte, key, off)"
