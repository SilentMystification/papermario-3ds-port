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
{ cat "$B.out"; for f in src/entity/model/M.c 3ds/tools/gfxin/src/entity/model/M.c src/own.c 3ds/tools/gfxin/src/own.c src/user.c src/gconv_user.c src/pal.c; do echo "== $f"; cat "$B/$f"; done; } | tr -d '\r' > "$B.txt"
tr -d '\r' < 3ds/tests/gset/expected_blk.txt > "$B.exp"
cmp -s "$B.txt" "$B.exp" || { echo "gset block test FAIL: output differs from expected_blk.txt (see $B.txt)"; diff "$B.txt" "$B.exp" | head -40; exit 1; }
BS="$T.blkseed"; rm -rf "$BS"; mkdir -p "$BS"; cp -r 3ds/tests/gset/case_blk/. "$BS/"
sed -e 's/^BLOCK_END = "};"/BLOCK_END = "}"/' -e 's/return ln.rstrip() == BLOCK_END/return ln.strip().startswith(BLOCK_END)/' 3ds/tools/gset.py > "$BS.py"
$PY "$BS.py" "$BS/list.txt" "$BS/gset_ok.txt" "$BS" > "$BS.out" 2>&1 || true
{ cat "$BS.out"; for f in src/entity/model/M.c 3ds/tools/gfxin/src/entity/model/M.c src/own.c 3ds/tools/gfxin/src/own.c src/user.c src/gconv_user.c src/pal.c; do echo "== $f"; cat "$BS/$f"; done; } | tr -d '\r' > "$BS.txt"
if cmp -s "$BS.txt" "$B.exp"; then echo "gset block seeded fault not seen"; exit 1; fi
# GSETB seeded fault: without the hold rule the own.c line with SetCustomGfx must print a keep line and differ
HS="$T.holdseed"; rm -rf "$HS"; mkdir -p "$HS"; cp -r 3ds/tests/gset/case_blk/. "$HS/"
sed 's/^HOLD_RULE = True/HOLD_RULE = False/' 3ds/tools/gset.py > "$HS.py"
$PY "$HS.py" "$HS/list.txt" "$HS/gset_ok.txt" "$HS" > "$HS.out" 2>&1 || true
{ cat "$HS.out"; for f in src/entity/model/M.c 3ds/tools/gfxin/src/entity/model/M.c src/own.c 3ds/tools/gfxin/src/own.c src/user.c src/gconv_user.c src/pal.c; do echo "== $f"; cat "$HS/$f"; done; } | tr -d '\r' > "$HS.txt"
cmp -s "$HS.txt" "$B.exp" && { echo "gset hold seeded fault not seen"; exit 1; }
# ISTAR own seeded fault: without the own rule the pal.c fixture keeps no D: the output must differ
O="$T.own"; rm -rf "$O"; mkdir -p "$O"; cp -r 3ds/tests/gset/case_blk/. "$O/"
sed 's/^OWN_RULE = True/OWN_RULE = False/' 3ds/tools/gset.py > "$O.py"
$PY "$O.py" "$O/list.txt" "$O/gset_ok.txt" "$O" > "$O.out" 2>&1 || true
{ cat "$O.out"; for f in src/entity/model/M.c 3ds/tools/gfxin/src/entity/model/M.c src/own.c 3ds/tools/gfxin/src/own.c src/user.c src/gconv_user.c src/pal.c; do echo "== $f"; cat "$O/$f"; done; } | tr -d '' > "$O.txt"
cmp -s "$O.txt" "$B.exp" && { echo "gset own seeded fault not seen"; exit 1; }
# GSETW: N(x) names (fixture case_ns: a.c and m1/gfx.c with m1.h and skel.inc.c; b.c is another namespace and must not change).
N="$T.ns"; rm -rf "$N"; mkdir -p "$N"; cp -r 3ds/tests/gset/case_ns/. "$N/"
rc=0; $PY 3ds/tools/gset.py "$N/list.txt" "$N/gset_ok.txt" "$N" > "$N.out" 2>&1 || rc=$?
[ "$rc" = 0 ] || { echo "gset ns test: exit $rc, want 0"; exit 1; }
tr -d '\r' < "$N.out" > "$N.o2"
for want in "gset moved 3 files 3 keep 0" "gset ok"; do grep -qxF "$want" "$N.o2" || { echo "gset ns test FAIL: missing line: $want"; cat "$N.o2"; exit 1; }; done
chk() { tr -d '\r' < "$N/$1" | grep -qxF "$2" || { echo "gset ns test FAIL: $1 lacks: $2"; exit 1; }; }
chk src/battle/item/a.c 'NDN_DECL(N(dl));'
chk src/battle/item/a.c 'EntityModelScript N(ems) = STANDARD_ENTITY_MODEL_SCRIPT(NDN(N(dl)), 1);'
chk src/world/m1/m1.h 'NDN_DECL(N(g2));'
chk src/world/m1/skel.inc.c '    .displayList = NDN(N(g1)),'
chk src/world/m1/gfx.c 'NDN_DECL(N(g1));'
chk 3ds/tools/gfxin/src/world/m1/gfx.c 'Gfx N(g2)[] = {'
cmp -s "$N/src/battle/item/b.c" "3ds/tests/gset/case_ns/src/battle/item/b.c" || { echo "gset ns test FAIL: b.c (other namespace) changed"; exit 1; }
# seeded fault: without the namespace rule the other namespace's b.c must change
NS2="$T.nsseed"; rm -rf "$NS2"; mkdir -p "$NS2"; cp -r 3ds/tests/gset/case_ns/. "$NS2/"
sed 's/^NS_RULE = True/NS_RULE = False/' 3ds/tools/gset.py > "$NS2.py"
$PY "$NS2.py" "$NS2/list.txt" "$NS2/gset_ok.txt" "$NS2" > "$NS2.out" 2>&1 || true
cmp -s "$NS2/src/battle/item/b.c" "3ds/tests/gset/case_ns/src/battle/item/b.c" && { echo "gset ns seeded fault not seen"; exit 1; }
# VSLOT2: include hold (fixture case_inc: w.c includes C and D, its kept line gsSPBranchList(C) names C): C stays, D moves. Seeded fault: INC_HOLD off moves C too.
I="$T.inc"; rm -rf "$I"; mkdir -p "$I"; cp -r 3ds/tests/gset/case_inc/. "$I/"
rc=0; $PY 3ds/tools/gset.py "$I/list.txt" "$I/gset_ok.txt" "$I" > "$I.out" 2>&1 || rc=$?
[ "$rc" = 0 ] || { echo "gset inc test: exit $rc, want 0"; exit 1; }
tr -d '\r' < "$I.out" | grep -q "^gset moved 1 files" || { echo "gset inc test FAIL: no moved line"; cat "$I.out"; exit 1; }
tr -d '\r' < "$I/src/effects/gfx/w.c" | grep -qxF '#include "effects/gfx/C.gfx.inc.c"' || { echo "gset inc test FAIL: w.c lost the C include"; exit 1; }
IS="$T.incseed"; rm -rf "$IS"; mkdir -p "$IS"; cp -r 3ds/tests/gset/case_inc/. "$IS/"
sed 's/^INC_HOLD = True/INC_HOLD = False/' 3ds/tools/gset.py > "$IS.py"
$PY "$IS.py" "$IS/list.txt" "$IS/gset_ok.txt" "$IS" > "$IS.out" 2>&1 || true
tr -d '\r' < "$IS/src/effects/gfx/w.c" | grep -qxF '#include "effects/gfx/C.gfx.inc.c"' && { echo "gset inc seeded fault not seen"; exit 1; }
# CHCONV: TYPE_RULE. Fixture case_typ: t.c names Gfx, has no common.h, and its moved include goes: line 1 becomes common.h. Seeded fault: TYPE_RULE off, no common.h line.
Y="$T.typ"; rm -rf "$Y"; mkdir -p "$Y"; cp -r 3ds/tests/gset/case_typ/. "$Y/"
rc=0; $PY 3ds/tools/gset.py "$Y/list.txt" "$Y/gset_ok.txt" "$Y" > "$Y.out" 2>&1 || rc=$?
[ "$rc" = 0 ] || { echo "gset typ test: exit $rc, want 0"; exit 1; }
tr -d '' < "$Y/src/effects/gfx/t.c" | head -1 | grep -qxF '#include "common.h"' || { echo "gset typ test FAIL: t.c line 1 is not common.h"; exit 1; }
YS="$T.typseed"; rm -rf "$YS"; mkdir -p "$YS"; cp -r 3ds/tests/gset/case_typ/. "$YS/"
sed 's/^TYPE_RULE = True/TYPE_RULE = False/' 3ds/tools/gset.py > "$YS.py"
$PY "$YS.py" "$YS/list.txt" "$YS/gset_ok.txt" "$YS" > "$YS.out" 2>&1 || true
tr -d '' < "$YS/src/effects/gfx/t.c" | grep -qxF '#include "common.h"' && { echo "gset typ seeded fault not seen"; exit 1; }
# GSETI blkinc: block source b.c (case_blkinc) includes asset E.gfx.inc.c (all lists ok). The include moves: b.c keeps NDN_DECL(E_gfx) and NDN(E_gfx) uses, the twin gets the include.
K="$T.blkinc"; rm -rf "$K"; mkdir -p "$K"; cp -r 3ds/tests/gset/case_blkinc/. "$K/"
rc=0; $PY 3ds/tools/gset.py "$K/list.txt" "$K/gset_ok.txt" "$K" > "$K.out" 2>&1 || rc=$?
[ "$rc" = 0 ] || { echo "gset blkinc test: exit $rc, want 0"; cat "$K.out"; exit 1; }
tr -d '\r' < "$K.out" | grep -q "^gset moved 1 files" || { echo "gset blkinc test FAIL: no moved line"; cat "$K.out"; exit 1; }
tr -d '\r' < "$K/src/b.c" | grep -qxF 'NDN_DECL(E_gfx);' || { echo "gset blkinc test FAIL: b.c lacks NDN_DECL(E_gfx);"; exit 1; }
tr -d '\r' < "$K/src/b.c" | grep -qxF '    gSPDisplayList(gMainGfxPos++, NDN(E_gfx));' || { echo "gset blkinc test FAIL: b.c lacks the NDN(E_gfx) use"; exit 1; }
tr -d '\r' < "$K/3ds/tools/gfxin/src/b.c" | grep -qxF '#include "ast/E.gfx.inc.c"' || { echo "gset blkinc test FAIL: twin lacks the include"; exit 1; }
KS="$T.blkincseed"; rm -rf "$KS"; mkdir -p "$KS"; cp -r 3ds/tests/gset/case_blkinc/. "$KS/"
sed 's/^INC_BLOCK = True/INC_BLOCK = False/' 3ds/tools/gset.py > "$KS.py"
$PY "$KS.py" "$KS/list.txt" "$KS/gset_ok.txt" "$KS" > "$KS.out" 2>&1 || true
tr -d '\r' < "$KS/src/b.c" | grep -qxF '#include "ast/E.gfx.inc.c"' || { echo "gset blkinc seeded fault not seen"; exit 1; }
# GSETI incns: block source c.c (namespace ns) includes asset F.gfx.inc.c, list ns_F_gfx; its N(F_gfx) use must become NDN(N(F_gfx)) (rewrite_ns). Seeded fault: INC_NS off leaves the N use.
M="$T.incns"; rm -rf "$M"; mkdir -p "$M"; cp -r 3ds/tests/gset/case_incns/. "$M/"
rc=0; $PY 3ds/tools/gset.py "$M/list.txt" "$M/gset_ok.txt" "$M" > "$M.out" 2>&1 || rc=$?
[ "$rc" = 0 ] || { echo "gset incns test: exit $rc, want 0"; cat "$M.out"; exit 1; }
tr -d '\r' < "$M/src/c.c" | grep -qxF 'NDN_DECL(N(F_gfx));' || { echo "gset incns test FAIL: c.c lacks NDN_DECL(N(F_gfx));"; exit 1; }
tr -d '\r' < "$M/src/c.c" | grep -qxF '    gSPDisplayList(gMainGfxPos++, NDN(N(F_gfx)));' || { echo "gset incns test FAIL: c.c lacks the NDN(N(F_gfx)) use"; exit 1; }
MS="$T.incnsseed"; rm -rf "$MS"; mkdir -p "$MS"; cp -r 3ds/tests/gset/case_incns/. "$MS/"
sed 's/^INC_NS = True/INC_NS = False/' 3ds/tools/gset.py > "$MS.py"
$PY "$MS.py" "$MS/list.txt" "$MS/gset_ok.txt" "$MS" > "$MS.out" 2>&1 || true
tr -d '\r' < "$MS/src/c.c" | grep -qxF '    gSPDisplayList(gMainGfxPos++, NDN(N(F_gfx)));' && { echo "gset incns seeded fault not seen"; exit 1; }
echo "gset test ok"
