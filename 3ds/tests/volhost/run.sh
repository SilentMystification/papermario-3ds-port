#!/bin/sh
# VOLHOST self-test (needs docker; not a host.d test): 3ds/tools/nd_host_test.sh gives its containers only the key files.
# Control: the tar of the key list (build3ds/out/hostkey/host_in.txt) unpacks with HOST_AV and the asset volume (rc 0), and the
# container holds exactly the files of the key list. Seeded faults: a list with one file outside HOST_IN (3ds/HANDOFF.md) gives
# a container tree that differs from the key list; one appended byte in a copy of the asset volume gives rc 7.
# Usage, from the repo root after one nd_host_test.sh run: sh 3ds/tests/volhost/run.sh. Prints "volhost ok" or "volhost FAIL <why>".
O=build3ds/out/hostkey
[ -s $O/asset_now.txt ] && [ -s $O/host_in.txt ] || { echo "volhost FAIL no $O/asset_now.txt or $O/host_in.txt (run nd_host_test.sh first)"; exit 1; }
AV=pm3ds-assets-$(sha1sum < $O/asset_now.txt | cut -c1-16)
SV=pm3ds-assets-vhseed$$
HOST_AV=; eval "$(grep '^HOST_AV=' 3ds/tools/nd_host_test.sh)"
[ -n "$HOST_AV" ] || { echo "volhost FAIL no HOST_AV line in 3ds/tools/nd_host_test.sh"; exit 1; }
# seen <list> <volume>: the files of the container tree (not the asset links), sorted.
seen() { tar -cf - -T "$1" | MSYS_NO_PATHCONV=1 docker run --rm -i -v "$2:/a:ro" -w /work pm3ds-host:1 sh -c "set -e; $HOST_AV; find . -path ./assets -prune -o -path ./ver -prune -o -type f -print | sed 's|^\./||' | LC_ALL=C sort"; }
cut -f1 $O/host_in.txt | LC_ALL=C sort > $O/vh_list.txt
seen $O/vh_list.txt "$AV" > $O/vh_seen.txt; rc=$?
[ "$rc" = 0 ] || { echo "volhost FAIL control rc $rc ($AV)"; exit 1; }
cmp -s $O/vh_list.txt $O/vh_seen.txt || { echo "volhost FAIL the container files differ from the key list"; diff $O/vh_list.txt $O/vh_seen.txt | head -5; exit 1; }
grep -qx 3ds/build.sh $O/vh_seen.txt && ! grep -q '^3ds/logs/' $O/vh_seen.txt || { echo "volhost FAIL 3ds/build.sh missing or 3ds/logs present"; exit 1; }
# Seeded fault 1: one file outside the key list must show as a difference.
{ cat $O/vh_list.txt; echo 3ds/HANDOFF.md; } > $O/vh_seed.txt
seen $O/vh_seed.txt "$AV" > $O/vh_seen2.txt
cmp -s $O/vh_list.txt $O/vh_seen2.txt && { echo "volhost FAIL seeded outside file not seen"; exit 1; }
# Seeded fault 2: one appended byte in a copy of the asset volume gives rc 7.
docker volume create $SV >/dev/null
MSYS_NO_PATHCONV=1 docker run --rm -v "$AV:/s:ro" -v "$SV:/a" pm3ds-host:1 sh -c 'cp -a /s/c /a/c && f=$(sed -n 1p /a/c/out.sha1 | cut -c43-) && printf X >> "/a/c/$f"'
seen $O/vh_list.txt "$SV" > /dev/null 2>&1; rc=$?
docker volume rm $SV >/dev/null
[ "$rc" = 7 ] || { echo "volhost FAIL seeded asset byte rc $rc (want 7)"; exit 1; }
rm -f $O/vh_list.txt $O/vh_seen.txt $O/vh_seed.txt $O/vh_seen2.txt
echo "volhost ok"
