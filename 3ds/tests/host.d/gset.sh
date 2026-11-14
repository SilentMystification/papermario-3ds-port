#!/bin/sh
# GSET1: gset.py on the fixture 3ds/tests/gset/case (a fake repo). The output (the tool's lines and the three files it writes or changes)
# must equal 3ds/tests/gset/expected.txt (LF; CR dropped on both sides: the Windows python writes CRLF). The seeded fault: a copy with the
# keep rule off must differ. Prints `gset test ok` or fails.
T=${GSET_T:-/tmp/gsetcase}
PY=${PY:-python3}
rm -rf "$T"; mkdir -p "$T"; cp -r 3ds/tests/gset/case/. "$T/"
rc=0; $PY 3ds/tools/gset.py "$T/list.txt" "$T/gset_ok.txt" "$T" > "$T.out" 2>&1 || rc=$?
[ "$rc" = 1 ] || { echo "gset test: exit $rc, want 1 (keep lines)"; exit 1; }
{ cat "$T.out"; for f in src/effects/gfx/fake.c 3ds/tools/gfxin/src/effects/gfx/fake.c src/user.c; do echo "== $f"; cat "$T/$f"; done; } | tr -d '\r' > "$T.txt"
tr -d '\r' < 3ds/tests/gset/expected.txt > "$T.exp"
cmp -s "$T.txt" "$T.exp" || { echo "gset test FAIL: output differs from expected.txt (see $T.txt)"; diff "$T.txt" "$T.exp" | head -40; exit 1; }
# seeded fault: without the keep rule the same fixture must differ
S="$T.nokeep"; rm -rf "$S"; mkdir -p "$S"; cp -r 3ds/tests/gset/case/. "$S/"
sed 's/^KEEP_RULE = True/KEEP_RULE = False/' 3ds/tools/gset.py > "$S.py"
$PY "$S.py" "$S/list.txt" "$S/gset_ok.txt" "$S" > "$S.out" 2>&1 || true
{ cat "$S.out"; for f in src/effects/gfx/fake.c 3ds/tools/gfxin/src/effects/gfx/fake.c src/user.c; do echo "== $f"; cat "$S/$f"; done; } | tr -d '\r' > "$S.txt"
if cmp -s "$S.txt" "$T.exp"; then echo "gset seeded keep fault not seen"; exit 1; fi
echo "gset test ok"
