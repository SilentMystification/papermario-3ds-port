# GENKEY: seeded faults of the read-set keys (3ds/tools/readkey.sh with gencache.sh gc_get/gc_put). Cases: first put and hit;
# a changed id of a read file misses; a changed id of a listed file that was not read hits; a changed stored output misses and
# its entry goes; PM_CACHE=off and check never hit; a trace without a must file stops (exit 1); rk_paths adds a GC_EXCL output.
T=/tmp/genkey; rm -rf $T; mkdir -p $T/gc $T/w/a $T/out
OUT=$T/out; MAIN_ROOT=$T; GENCACHE_DIR=$T/gc
. 3ds/tools/gencache.sh
. 3ds/tools/readkey.sh
GC_EXCL="a/gen.c"
fail() { echo "genkey FAIL: $1"; rm -rf $T; exit 1; }
printf 'a/r1.c\tb:1111\na/r2.h\tb:2222\na/u1.c\tb:3333\n' > $T/ids.txt
printf 'out1\n' > $T/w/o1.txt
cd $T/w || exit 1
printf 'gen\n' > a/gen.c
rk_paths $T/ids.txt a > $T/paths.txt
[ "$(cat $T/paths.txt | tr '\n' ' ')" = "a/gen.c a/r1.c a/r2.h a/u1.c " ] || fail "rk_paths: $(cat $T/paths.txt | tr '\n' ' ')"
printf 'cc1\ta/r1.c\nsh\ta/r2.h\nsh\tnew/made.o\npython3\ta/gen.c\n' > $T/log.txt
rk_reads $T/log.txt $T/paths.txt $T/reads.txt a/r1.c
[ "$(cat $T/reads.txt | tr '\n' ' ')" = "a/gen.c a/r1.c a/r2.h " ] || fail "rk_reads: $(cat $T/reads.txt | tr '\n' ' ')"
( rk_reads $T/log.txt $T/paths.txt $T/r2.txt a/u1.c ) 2>/dev/null && fail "a trace without a must file passed"
K0="t v1"
rk_get tg "$K0" $T/ids.txt o.txt:$T/w/o1.txt && fail "hit on an empty store"
gc_put "$RK_DIR" "$(rk_full "$K0" $T/ids.txt $T/reads.txt)" o.txt:$T/w/o1.txt reads.txt:$T/reads.txt || fail "put"
rm -f $T/w/o1.txt
rk_get tg "$K0" $T/ids.txt o.txt:$T/w/o1.txt || fail "no hit after put"
[ "$(cat $T/w/o1.txt)" = out1 ] || fail "hit gave other bytes"
printf 'a/r1.c\tb:1111\na/r2.h\tb:2222\na/u1.c\tb:9999\n' > $T/ids.txt
rk_get tg "$K0" $T/ids.txt o.txt:$T/w/o1.txt || fail "an unread file edit missed"
printf 'a/r1.c\tb:1112\na/r2.h\tb:2222\na/u1.c\tb:9999\n' > $T/ids.txt
rk_get tg "$K0" $T/ids.txt o.txt:$T/w/o1.txt && fail "a read file edit hit"
printf 'a/r1.c\tb:1111\na/r2.h\tb:2222\na/u1.c\tb:3333\n' > $T/ids.txt
printf 'gen2\n' > a/gen.c
rk_get tg "$K0" $T/ids.txt o.txt:$T/w/o1.txt && fail "a GC_EXCL output edit (hashed from the disk) hit"
printf 'gen\n' > a/gen.c
rk_get tg "$K0" $T/ids.txt o.txt:$T/w/o1.txt || fail "no hit after the revert"
rk_get tg "K0 other" $T/ids.txt o.txt:$T/w/o1.txt && fail "another K0 hit"
PM_CACHE=off rk_get tg "$K0" $T/ids.txt o.txt:$T/w/o1.txt && fail "PM_CACHE=off hit"
PM_CACHE=check rk_get tg "$K0" $T/ids.txt o.txt:$T/w/o1.txt && fail "PM_CACHE=check hit"
e=$(ls -d $T/gc/$RK_DIR/*/ | head -1); printf X >> $e/o.txt
rk_get tg "$K0" $T/ids.txt o.txt:$T/w/o1.txt 2>/dev/null && fail "a changed stored byte hit"
[ -d "$e" ] && fail "the failed entry stayed"
cd / && rm -rf $T
echo "genkey ok (paths, reads, must, hit, unread hit, read miss, excl miss, K0, off, check, byte)"
