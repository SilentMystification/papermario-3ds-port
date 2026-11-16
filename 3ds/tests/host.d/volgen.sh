#!/bin/sh
# VOLGEN: self-test of the generator cache 3ds/tools/gencache.sh (cache safety rule 4, plan_buildspeed.md). Seeded faults: one
# changed input byte, one changed stored byte, a key.txt of another key, one changed byte in a dir entry, PM_CACHE=off, and a
# cachecheck compare. Each fault must give a miss (or a CACHECHECK FAIL). Prints "volgen gencache ok", exit 1 on a failure.
set -e
T=/tmp/volgen; rm -rf $T; mkdir -p $T/out
OUT=$T/out; GENCACHE_DIR=$T/store; MAIN_ROOT=$T; PM_CACHE=on
. 3ds/tools/gencache.sh
fail() { echo "volgen FAIL $1"; exit 1; }
key() { printf 'gen t v1\nin %s' "$(sha1sum < $T/in1 | cut -c1-40)"; }
printf 'a\n' > $T/in1; printf 'x1\n' > $T/o1.txt
k1=$(key)
gc_get t "$k1" o1.txt:$T/r1.txt && fail "hit on an empty store"
gc_put t "$k1" o1.txt:$T/o1.txt
gc_get t "$k1" o1.txt:$T/r1.txt || fail "miss after put"
cmp -s $T/r1.txt $T/o1.txt || fail "a hit copied other bytes"
printf 'b\n' > $T/in1; k2=$(key)
gc_get t "$k2" o1.txt:$T/r1.txt && fail "one changed input byte gave a hit"
d=$(gc_dir t "$k1"); printf 'x2\n' > $d/o1.txt
gc_get t "$k1" o1.txt:$T/r1.txt 2>/dev/null && fail "a changed stored byte gave a hit"
[ -d "$d" ] && fail "a failed entry was kept"
gc_put t "$k1" o1.txt:$T/o1.txt; printf '%s' "$k2" > $d/key.txt
gc_get t "$k1" o1.txt:$T/r1.txt 2>/dev/null && fail "a key.txt of another key gave a hit"
mkdir -p $T/sd; for i in 1 2 3; do printf "s$i\n" > $T/sd/f$i.nds; done
gc_put_dir s "$k1" $T/sd $T/m0
gc_get_dir s "$k1" $T/ld $T/m1 || fail "dir miss after put"
diff -r -q $T/sd $T/ld >/dev/null || fail "a dir hit copied other bytes"
dd=$(gc_dir s "$k1"); rm -f $dd/f/f2.nds; printf 'zz\n' > $dd/f/f2.nds
gc_get_dir s "$k1" $T/ld $T/m1 2>/dev/null && fail "a changed byte in a dir entry gave a hit"
PM_CACHE=off; gc_put t "$k2" o1.txt:$T/o1.txt; PM_CACHE=on
gc_get t "$k2" o1.txt:$T/r1.txt && fail "PM_CACHE=off stored an entry"
gc_put t "$k2" o1.txt:$T/o1.txt
PM_CACHE=check; gc_get t "$k2" o1.txt:$T/r1.txt && fail "PM_CACHE=check gave a hit"
gc_put t "$k2" o1.txt:$T/o1.txt 2>/dev/null; [ "$GC_FAIL" = 0 ] || fail "cachecheck of equal bytes failed"
printf 'x3\n' > $T/o1.txt; gc_put t "$k2" o1.txt:$T/o1.txt 2>/dev/null; [ "$GC_FAIL" = 1 ] || fail "cachecheck missed a changed output"
PM_CACHE=on
echo "volgen gencache ok"
