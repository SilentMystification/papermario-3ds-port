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
# GSET3: block mode on the fixture 3ds/tests/gset/case_blk (entity model file M.c, a user file, a gconv_src.d user file). Same rules:
# the output must equal 3ds/tests/gset/expected_blk.txt. The seeded fault: the block end is a line that starts with `}` after strip (`    }`).
B="$T.blk"; rm -rf "$B"; mkdir -p "$B"; cp -r 3ds/tests/gset/case_blk/. "$B/"
rc=0; $PY 3ds/tools/gset.py "$B/list.txt" "$B/gset_ok.txt" "$B" > "$B.out" 2>&1 || rc=$?
[ "$rc" = 0 ] || { echo "gset block test: exit $rc, want 0"; exit 1; }
{ cat "$B.out"; for f in src/entity/model/M.c 3ds/tools/gfxin/src/entity/model/M.c src/own.c 3ds/tools/gfxin/src/own.c src/user.c src/gconv_user.c; do echo "== $f"; cat "$B/$f"; done; } | tr -d '\r' > "$B.txt"
tr -d '\r' < 3ds/tests/gset/expected_blk.txt > "$B.exp"
cmp -s "$B.txt" "$B.exp" || { echo "gset block test FAIL: output differs from expected_blk.txt (see $B.txt)"; diff "$B.txt" "$B.exp" | head -40; exit 1; }
BS="$T.blkseed"; rm -rf "$BS"; mkdir -p "$BS"; cp -r 3ds/tests/gset/case_blk/. "$BS/"
sed -e 's/^BLOCK_END = "};"/BLOCK_END = "}"/' -e 's/return ln.rstrip() == BLOCK_END/return ln.strip().startswith(BLOCK_END)/' 3ds/tools/gset.py > "$BS.py"
$PY "$BS.py" "$BS/list.txt" "$BS/gset_ok.txt" "$BS" > "$BS.out" 2>&1 || true
{ cat "$BS.out"; for f in src/entity/model/M.c 3ds/tools/gfxin/src/entity/model/M.c src/own.c 3ds/tools/gfxin/src/own.c src/user.c src/gconv_user.c; do echo "== $f"; cat "$BS/$f"; done; } | tr -d '\r' > "$BS.txt"
if cmp -s "$BS.txt" "$B.exp"; then echo "gset block seeded fault not seen"; exit 1; fi
# GSETB seeded fault: without the hold rule the own.c line with SetCustomGfx must print a keep line and differ
HS="$T.holdseed"; rm -rf "$HS"; mkdir -p "$HS"; cp -r 3ds/tests/gset/case_blk/. "$HS/"
sed 's/^HOLD_RULE = True/HOLD_RULE = False/' 3ds/tools/gset.py > "$HS.py"
$PY "$HS.py" "$HS/list.txt" "$HS/gset_ok.txt" "$HS" > "$HS.out" 2>&1 || true
{ cat "$HS.out"; for f in src/entity/model/M.c 3ds/tools/gfxin/src/entity/model/M.c src/own.c 3ds/tools/gfxin/src/own.c src/user.c src/gconv_user.c; do echo "== $f"; cat "$HS/$f"; done; } | tr -d '\r' > "$HS.txt"
cmp -s "$HS.txt" "$B.exp" && { echo "gset hold seeded fault not seen"; exit 1; }
echo "gset test ok"
