#!/bin/sh
# nd_n64_same.sh <file> [git-ref]: the N64 side of a file is unchanged. Preprocess the file at git-ref (default HEAD) and in the
# working tree with -UTARGET_3DS, and compare. Exit 0 same, 1 different. Uses the host gcc of Git Bash (-E only, no 32-bit needed).
cd "$(dirname "$0")/../.." || exit 2
f=$1; ref=${2:-HEAD}
[ -n "$f" ] || { echo "usage: nd_n64_same.sh <file> [git-ref]"; exit 2; }
export PATH="/c/msys64/mingw64/bin:$PATH"
INC="-I. -Iinclude -Isrc -I3ds/include -Iver/us/build/include -Iinclude/world -I./assets/us"
tmp=${TMPDIR:-/tmp}/nd_n64_$$; mkdir -p "$tmp"
git show "$ref:$f" > "$tmp/before.c" 2>/dev/null || { echo "no $f at $ref"; exit 2; }
cp "$f" "$tmp/after.c"
for side in before after; do
    if ! gcc -E -x c -UTARGET_3DS $INC - < "$tmp/$side.c" > "$tmp/$side.i" 2>"$tmp/err"; then echo "preprocess failed ($side)"; head -3 "$tmp/err"; rm -rf "$tmp"; exit 2; fi
done
if cmp -s "$tmp/before.i" "$tmp/after.i"; then echo "SAME N64 preprocess $f ($ref)"; rc=0; else echo "DIFF N64 preprocess $f ($ref)"; diff "$tmp/before.i" "$tmp/after.i" | head -20; rc=1; fi
rm -rf "$tmp"; exit $rc
