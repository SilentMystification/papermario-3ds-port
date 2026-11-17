# GENKEY: seeded self-test of the read trace (3ds/tools/readtrace.c). Each reader kind that a traced generator or host test uses
# must log the file it read: sh (script and "."), cat, sed, awk, grep, tar, python (open, import, os.walk + open), gcc (cc1 header,
# as .incbin), a 32-bit program (fopen). A file that no step reads must not be logged. A miss here would give a key without an input.
T=/tmp/readtrace; rm -rf $T; mkdir -p $T/r/sub $T/so
cc_so() { gcc $1 -O2 -shared -fPIC -o $2 3ds/tools/readtrace.c -ldl || { echo "readtrace FAIL: shim compile ($1)"; exit 1; }; }
cc_so -m64 $T/so/rt64.so; cc_so -m32 $T/so/rt32.so
for p in x86_64 i386 i486 i586 i686; do mkdir -p $T/so/$p; done
cp $T/so/rt64.so $T/so/x86_64/rt.so; for p in i386 i486 i586 i686; do cp $T/so/rt32.so $T/so/$p/rt.so; done
for f in s1 s2 cat sed awk grep tar py mod walk hdr inc fo32 unread; do printf '%s\n' "$f" > $T/r/sub/$f.txt; done
printf 'x=1\n' > $T/r/sub/mod.py; printf '#define HDR 1\n' > $T/r/sub/hdr.h
printf '#include "hdr.h"\nint v = HDR;\n' > $T/r/sub/c1.c
printf '.section .rodata\n.incbin "sub/inc.txt"\n' > $T/r/a1.s
printf '. sub/s2.txt 2>/dev/null; true\n' > $T/r/sub/s1.sh
printf '#include <stdio.h>\nint main(int c, char** v) { FILE* f = fopen(v[1], "r"); return f ? 0 : 1; }\n' > $T/fo.c
gcc -m32 -o $T/fo32 $T/fo.c || { echo "readtrace FAIL: 32-bit test program"; exit 1; }
(
    cd $T/r || exit 1
    export RT_LOG=$T/log.txt RT_ROOT=$T/r/ LD_PRELOAD="$T/so/\$PLATFORM/rt.so"
    sh sub/s1.sh; cat sub/cat.txt >/dev/null; sed -n 1p sub/sed.txt >/dev/null; awk 1 sub/awk.txt >/dev/null
    grep -q x sub/grep.txt; tar -cf /tmp/readtrace/t.tar sub/tar.txt
    python3 -c 'import sys, os; sys.path.insert(0, "sub"); open("sub/py.txt").read(); import mod
for r, _, ns in os.walk("sub"):
    [open(os.path.join(r, n)).read() for n in ns if n == "walk.txt"]'
    gcc -c -o /tmp/readtrace/c1.o -I sub sub/c1.c; gcc -c -o /tmp/readtrace/a1.o a1.s
    $T/fo32 sub/fo32.txt
) || { echo "readtrace FAIL: a traced reader failed"; exit 1; }
miss=""
for f in sub/s1.sh sub/s2.txt sub/cat.txt sub/sed.txt sub/awk.txt sub/grep.txt sub/tar.txt sub/py.txt sub/mod.py sub/walk.txt sub/hdr.h sub/c1.c a1.s sub/inc.txt sub/fo32.txt; do
    cut -f2 $T/log.txt | grep -qx "$f" || miss="$miss $f"
done
[ -z "$miss" ] || { echo "readtrace FAIL: not logged:$miss"; exit 1; }
cut -f2 $T/log.txt | grep -qx sub/unread.txt && { echo "readtrace FAIL: an unread file was logged"; exit 1; }
grep -q "^fo32	sub/fo32.txt$" $T/log.txt || { echo "readtrace FAIL: the 32-bit reader has no exe name"; exit 1; }
rm -rf $T
echo "readtrace ok (sh, cat, sed, awk, grep, tar, python, gcc, as, 32-bit fopen)"
