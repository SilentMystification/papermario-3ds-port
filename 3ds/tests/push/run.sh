#!/bin/sh
# Push hint test, no console: runs push_3ds.sh --check-send on saved 3dslink outputs.
# Expect: refused_partial.txt and a CR-joined partial send print the NETLOADER REFUSED block (exit 0);
# full_send.txt and connect_only.txt print no block (exit 1).
set -u
cd "$(dirname "$0")/../../.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
truncate -s 13359084 "$tmp/pm_3ds.3dsx" 2>/dev/null || dd if=/dev/zero of="$tmp/pm_3ds.3dsx" bs=1 count=0 seek=13359084 2>/dev/null
printf '3DS at 192.168.1.131\r1048576 sent (7.85%%)\r2097152 sent (15.70%%)\r' >"$tmp/refused_cr.txt"
fail=0
check() {
    want=$1; f=$2
    out=$(PM_SRC3DSX="$tmp/pm_3ds.3dsx" sh 3ds/push_3ds.sh --check-send "$f") && rc=0 || rc=$?
    if [ "$want" = refused ]; then
        if [ "$rc" -eq 0 ] && echo "$out" | grep -q "NETLOADER REFUSED" && echo "$out" | grep -q "13359084"; then echo "ok   refused $(basename "$f")"
        else echo "FAIL refused $(basename "$f") (rc $rc)"; fail=1; fi
    else
        if [ "$rc" -ne 0 ] && ! echo "$out" | grep -q "NETLOADER REFUSED"; then echo "ok   no-hint $(basename "$f")"
        else echo "FAIL no-hint $(basename "$f") (rc $rc)"; fail=1; fi
    fi
}
check refused 3ds/tests/push/refused_partial.txt
check refused "$tmp/refused_cr.txt"
check none 3ds/tests/push/full_send.txt
check none 3ds/tests/push/connect_only.txt
exit $fail
