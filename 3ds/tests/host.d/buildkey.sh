#!/bin/sh
# BUILDO: seeded faults of the key phase (3ds/tools/build_key.py, inc_scan.py, asset_ids.py stat cache).
# 1 build_key.rk_get finds an entry that the shell (gencache.sh gc_put, readkey.sh rk_full) stored: same key text; a changed
#   read id misses; a changed id of an unread file hits; a changed stored byte misses and removes the entry; PM_CACHE=check misses.
# 2 inc_scan: an include in an #if 0 branch counts; every -I folder counts (a shadowing header); an include by macro exits 3.
# 3 asset stat cache: a reused id needs size and mtime equal; a racy file (mtime within 2 s of the scan) is hashed again.
T=/tmp/buildkey; rm -rf $T; mkdir -p $T/gc $T/out $T/w/a
R=$(pwd); fail() { echo "buildkey FAIL: $1"; rm -rf $T; exit 1; }
OUT=$T/out; MAIN_ROOT=$T; GENCACHE_DIR=$T/gc
. 3ds/tools/gencache.sh
. 3ds/tools/readkey.sh
printf 'a/r1.c\tb:1111\na/r2.h\tb:2222\na/u1.c\tb:3333\n' > $T/ids.txt
printf 'a/r1.c\na/r2.h\n' > $T/reads.txt
printf 'out1\n' > $T/w/o1.txt
K0="t v1
cmd x"
gc_put "tg/$(printf '%s' "$K0" | sha1sum | cut -c1-16)" "$(rk_full "$K0" $T/ids.txt $T/reads.txt)" o.txt:$T/w/o1.txt reads.txt:$T/reads.txt || fail "shell put"
pyget() { ( cd $T/w && GENCACHE_DIR=$T/gc OUT=$T/out PM_CACHE=${1:-on} python3 -c "
import sys; sys.path.insert(0, '$R/3ds/tools'); import build_key as b
ids = b.idmap([l for l in open('$T/ids.txt').read().splitlines() if l])
hit, d = b.rk_get('tg', open('$T/k0.txt').read(), ids, [('o.txt', '$T/w/r.txt')])
sys.exit(0 if hit else 1)" ); }
printf '%s' "$K0" > $T/k0.txt
pyget || fail "python missed an entry the shell stored"
[ "$(cat $T/w/r.txt)" = out1 ] || fail "a hit copied other bytes"
printf 'a/r1.c\tb:1111\na/r2.h\tb:2222\na/u1.c\tb:9999\n' > $T/ids.txt
pyget || fail "an unread file edit missed"
pyget check && fail "PM_CACHE=check hit"
printf 'a/r1.c\tb:1112\na/r2.h\tb:2222\na/u1.c\tb:9999\n' > $T/ids.txt
pyget && fail "a read file edit hit"
printf 'a/r1.c\tb:1111\na/r2.h\tb:2222\na/u1.c\tb:9999\n' > $T/ids.txt
e=$(ls -d $T/gc/tg/*/*/ | head -1); printf X >> $e/o.txt
pyget 2>/dev/null && fail "a changed stored byte hit"
[ -d "$e" ] && fail "a failed entry was kept"
# 2 inc_scan
mkdir -p $T/s/i1 $T/s/i2 && cd $T/s
printf '#if 0\n#include "h0.h"\n#endif\n#include <h1.h>\n' > m.c; printf '' > h0.h; printf '#include "h2.h"\n' > i2/h1.h; printf '' > i2/h2.h
python3 $R/3ds/tools/inc_scan.py $T/l1.txt "-I i1,i2 m.c" || fail "inc_scan exit $?"
[ "$(tr '\n' ' ' < $T/l1.txt)" = "h0.h i2/h1.h i2/h2.h m.c " ] || fail "inc_scan list: $(tr '\n' ' ' < $T/l1.txt)"
printf '' > i1/h1.h; python3 $R/3ds/tools/inc_scan.py $T/l2.txt "-I i1,i2 m.c"
grep -qx i1/h1.h $T/l2.txt || fail "a shadowing header is not listed"
printf '#define H "h0.h"\n#include H\n' > m2.c; python3 $R/3ds/tools/inc_scan.py $T/l3.txt "-I i1 m2.c" 2>/dev/null; rc=$?
[ "$rc" = 3 ] || fail "an include by macro gave exit $rc, want 3"
cd $R
# 3 asset stat cache
mkdir -p $T/as/d && printf 'aaaa' > $T/as/d/f1 && printf 'bbbb' > $T/as/d/f2
old=$(( $(date +%s) - 100 )); touch -d "@$old" $T/as/d/f1 $T/as/d/f2
ids() { PM_ASSET_STAT=$T/st.txt python3 3ds/tools/asset_ids.py $T/as $T/p.txt $T/n.txt d && cut -f2 $T/n.txt | tr '\n' ' '; }
i1=$(ids); i1b=$(ids); [ "$i1" = "$i1b" ] || fail "stat cache: two runs differ"
printf 'cccc' > $T/as/d/f1; touch -d "@$old" $T/as/d/f1
[ "$(ids)" = "$i1" ] && true # an old mtime and the same size reuse the id (the git index rule; not a fault case)
printf 'dddddd' > $T/as/d/f2; touch -d "@$old" $T/as/d/f2
[ "$(ids)" = "$i1" ] && fail "stat cache: a new size kept the id"
printf 'eeee' > $T/as/d/f1; touch -r $T/as/d/f1 $T/ref; i2=$(ids) # f1 is racy now (mtime within 2 s of the scan)
printf 'ffff' > $T/as/d/f1; touch -r $T/ref $T/as/d/f1 # same size, same mtime: only the racy rule finds the change
[ "$(ids)" = "$i2" ] && fail "stat cache: a racy file kept its id"
rm -rf $T
echo "buildkey ok (shell entry hit, unread hit, read miss, byte miss, check, inc_scan if0 shadow macro, stat size racy)"
