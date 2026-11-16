#!/bin/sh
# GFXINC: self-test of the gconv object cache 3ds/tools/gconv_objcache.py (cache safety rule 4). Seeded faults: one changed
# byte of an #include file, of an .incbin file and of a stored object; a key.txt of another key; PM_CACHE=off; a cachecheck of a
# changed entry. Each must give a miss (or CACHECHECK FAIL). A hit must give the bytes of a plain compile. Prints "gfxinc ok".
set -e
R=$(pwd); W=/tmp/gfxinc; rm -rf $W; mkdir -p $W/c $W/p; cd $W
fail() { echo "gfxinc FAIL $1"; exit 1; }
printf '#define H 1\n' > h.h; printf 'abc' > d.bin
printf '#include "h.h"\n__asm__(".pushsection .data\\n.incbin \\"d.bin\\"\\n.popsection\\n");\nint x = H;\n' > a.c
F="-m32 -w -ffunction-sections -fdata-sections -I ."
oc() { python3 "$R/3ds/tools/gconv_objcache.py" --flags "$F" a.c 2> $W/log.txt; }
export GCONV_OBJ=$W/c PM_CACHE=on
oc || fail "first compile"; grep -q "hit 0 miss 1" $W/log.txt || fail "first run is not a miss"
oc; grep -q "hit 1 miss 0" $W/log.txt || fail "second run is not a hit"
cp a.o $W/p/hit.o; gcc $F -c -o $W/p/plain.o a.c; cmp -s $W/p/hit.o $W/p/plain.o || fail "a hit gave other bytes than a plain compile"
printf '#define H 2\n' > h.h; oc; grep -q "miss 1" $W/log.txt || fail "a changed include byte gave a hit"
printf 'abd' > d.bin; oc; grep -q "miss 1" $W/log.txt || fail "a changed incbin byte gave a hit"
e=$(ls -d $W/c/*/ | head -1); printf 'x' >> ${e}obj.o; oc; grep -q "failed its check" $W/log.txt || fail "a changed stored byte gave a hit"
e=$(ls -d $W/c/*/ | head -1); printf 'other' > ${e}key.txt; oc; grep -q "miss 1" $W/log.txt || fail "a key.txt of another key gave a hit"
PM_CACHE=off oc; grep -q "hit 0 miss 1" $W/log.txt || fail "PM_CACHE=off gave a hit"
PM_CACHE=check oc; grep -q "cachecheck gconvobj ok 1" $W/log.txt || fail "cachecheck of an equal entry"
e=$(ls -d $W/c/*/ | head -1); printf 'x' >> ${e}obj.o; sha1sum ${e}obj.o | cut -d' ' -f1 > ${e}out.sha1
if PM_CACHE=check oc; then fail "cachecheck missed a changed entry"; fi
grep -q "CACHECHECK FAIL gconvobj" $W/log.txt || fail "no CACHECHECK FAIL line"
cd "$R"; rm -rf $W
echo "gfxinc ok"
