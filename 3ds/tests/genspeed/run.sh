#!/bin/sh
# GENSPEED self-test (needs docker; not a host.d test): the asset volume check GEN_AV of 3ds/build.sh. Control: the lane asset
# volume passes (rc 0). Seeded fault: a copy of it with one appended byte fails with rc 7. Usage, from the repo root after a
# build of the lane: sh 3ds/tests/genspeed/run.sh build3ds/lane-<lane>. Prints "genspeed ok" or "genspeed FAIL <why>".
O=${1:?usage: run.sh <lane out dir>}
[ -s "$O/asset_now.txt" ] || { echo "genspeed FAIL no $O/asset_now.txt"; exit 1; }
AV=pm3ds-assets-$(sha1sum < "$O/asset_now.txt" | cut -c1-16)
SV=pm3ds-assets-gsseed$$
GEN_AV=; eval "$(grep '^GEN_AV=' 3ds/build.sh)"
[ -n "$GEN_AV" ] || { echo "genspeed FAIL no GEN_AV line in 3ds/build.sh"; exit 1; }
run() { tar -cf - 3ds/tools/gen_gfx_tab.sh | MSYS_NO_PATHCONV=1 docker run --rm -i -v "$1:/a:ro" pm3ds-host:1 sh -c "set -e; $GEN_AV; test -f /work/3ds/tools/gen_gfx_tab.sh; test -d /work/assets/us; test -d /work/ver/us/build"; }
run "$AV"; rc=$?
[ "$rc" = 0 ] || { echo "genspeed FAIL control rc $rc ($AV)"; exit 1; }
docker volume create $SV >/dev/null
MSYS_NO_PATHCONV=1 docker run --rm -v "$AV:/s:ro" -v "$SV:/a" pm3ds-host:1 sh -c 'cp -a /s/c /a/c && f=$(sed -n 1p /a/c/out.sha1 | cut -c43-) && printf X >> "/a/c/$f"'
run "$SV" 2>/dev/null; rc=$?
docker volume rm $SV >/dev/null
[ "$rc" = 7 ] || { echo "genspeed FAIL seeded rc $rc (want 7)"; exit 1; }
echo "genspeed ok"
