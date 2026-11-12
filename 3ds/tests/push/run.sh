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

# PUSHFIX cases (no console). PM_3DSLINK = a fake 3dslink script, PM_SRC3DSX = the dummy 3dsx above,
# archive and push log in $tmp. Cases a, c, d pass -Seconds 5 / -t 1 / -IdleSeconds 5 so the live-log read ends fast.
fake_ok="$tmp/fake_ok.sh"; fake_partial="$tmp/fake_partial.sh"
printf '#!/bin/sh\necho "Sending pm_3ds.3dsx"\nexit 0\n' >"$fake_ok"
printf '#!/bin/sh\necho "10 sent (5%%)"\nexit 1\n' >"$fake_partial"
chmod +x "$fake_ok" "$fake_partial"

# (a) go.ps1 push o -NoBuild -Fg: no build, the fake 3dslink runs.
oa=$(env PM_3DSLINK="$fake_ok" PM_SRC3DSX="$tmp/pm_3ds.3dsx" PM_ARCHIVE_DIR="$tmp" PM_PUSH_LOG="$tmp/pushed.txt" \
    powershell -NoProfile -File 3ds/go.ps1 push o -NoBuild -Fg -ip 127.0.0.1 -Seconds 5 -IdleSeconds 5 2>&1) || true
if echo "$oa" | grep -q 'Sending' && ! echo "$oa" | grep -q 'STAGE' && ! echo "$oa" | grep -qi '^build'; then echo "ok   go -NoBuild no build"
else echo "FAIL go -NoBuild no build"; echo "$oa" | tail -n 20; fail=1; fi

# (b) a 3dsx whose build key (build_key.txt) is not the key of the tree now: exit 5, nothing sent.
printf '0000\n' >"$tmp/build_key_old.txt"
ob=$(env PM_SRC3DSX= PM_BUILD_KEY_FILE="$tmp/build_key_old.txt" sh 3ds/push_3ds.sh --no-build -t 1 2>&1) && rb=0 || rb=$?
if [ "$rb" -eq 5 ] && echo "$ob" | grep -q 'STALE 3DSX'; then echo "ok   stale 3dsx exit 5"
else echo "FAIL stale 3dsx (rc $rb)"; echo "$ob" | tail -n 20; fail=1; fi

# (b2) a build key file holding the worktree's own key: no STALE line, SEND FILE printed, the fake 3dslink sends.
PM_LANE= PM_KEY_ONLY=1 sh 3ds/build.sh >"$tmp/build_key_now.txt"
ob2=$(env PM_SRC3DSX="$tmp/pm_3ds.3dsx" PM_BUILD_KEY_FILE="$tmp/build_key_now.txt" PM_3DSLINK="$fake_ok" PM_ARCHIVE_DIR="$tmp" PM_PUSH_LOG="$tmp/pushed.txt" \
    sh 3ds/push_3ds.sh --no-build --ip 127.0.0.1 -t 1 --idle 1 2>&1) || true
if ! echo "$ob2" | grep -q 'STALE 3DSX' && echo "$ob2" | grep -q 'SEND FILE:' && echo "$ob2" | grep -q 'Sending'; then echo "ok   matching key checked, SEND FILE printed, fake 3dslink sent"
else echo "FAIL matching key"; echo "$ob2" | tail -n 20; fail=1; fi

# (c) a partial send: the attempt's 3dslink output is kept and the NETLOADER REFUSED block is printed.
oc=$(env PM_3DSLINK="$fake_partial" PM_SRC3DSX="$tmp/pm_3ds.3dsx" PM_ARCHIVE_DIR="$tmp" PM_PUSH_LOG="$tmp/pushed.txt" \
    sh 3ds/push_3ds.sh --no-build --ip 127.0.0.1 -t 1 --idle 1 2>&1) || true
kept=$(grep -l '10 sent (5%)' build3ds/out/push_127.0.0.1_*_a1.txt 2>/dev/null | head -n 1)
if [ -n "$kept" ] && echo "$oc" | grep -q 'NETLOADER REFUSED'; then echo "ok   partial send kept output"
else echo "FAIL partial send kept output"; echo "$oc" | tail -n 20; fail=1; fi

# (d) go.ps1 push o without -Fg: detached, PUSH STARTED at once, PUSH END in build3ds/out/push_last.txt within 60 s.
rm -f build3ds/out/push_last.txt
t0=$(date +%s)
od=$(env PM_3DSLINK="$fake_ok" PM_SRC3DSX="$tmp/pm_3ds.3dsx" PM_ARCHIVE_DIR="$tmp" PM_PUSH_LOG="$tmp/pushed.txt" \
    powershell -NoProfile -File 3ds/go.ps1 push o -NoBuild -ip 127.0.0.1 -Seconds 5 -IdleSeconds 5 2>&1) || true
t1=$(date +%s)
if echo "$od" | grep -q 'PUSH STARTED' && [ $((t1 - t0)) -le 5 ]; then echo "ok   detached PUSH STARTED in $((t1 - t0)) s"
else echo "FAIL detached PUSH STARTED ($((t1 - t0)) s)"; echo "$od" | tail -n 20; fail=1; fi
w0=$(date +%s)
until grep -q 'PUSH END' build3ds/out/push_last.txt 2>/dev/null || [ $(( $(date +%s) - w0 )) -ge 60 ]; do sleep 1; done
if grep -q 'PUSH END' build3ds/out/push_last.txt 2>/dev/null; then echo "ok   detached PUSH END ($(tr -d '\r\n\357\273\277' <build3ds/out/push_last.txt))"
else echo "FAIL detached PUSH END within 60 s"; fail=1; fi

# (e) slow send (PUSHFIX2): progress every 2 s for 16 s, then 100% and exit 0. NOGROW_SECS=4 must not kill it;
# the cap for the 13 MB dummy is 195 s (15 s per MB).
fake_slow="$tmp/fake_slow.sh"; fake_hung="$tmp/fake_hung.sh"
printf '#!/bin/sh\necho "Sending pm_3ds.3dsx"\nn=1\nwhile [ $n -le 8 ]; do sleep 2; echo "$((n * 1000000)) sent ($((n * 12)).00%%)"; n=$((n + 1)); done\necho "100.00%%"\nexit 0\n' >"$fake_slow"
printf '#!/bin/sh\necho "Sending pm_3ds.3dsx"\necho "10 sent (5%%)"\nsleep 60\n' >"$fake_hung"
chmod +x "$fake_slow" "$fake_hung"
t0=$(date +%s)
oe=$(env PM_3DSLINK="$fake_slow" NOGROW_SECS=4 PM_SRC3DSX="$tmp/pm_3ds.3dsx" PM_ARCHIVE_DIR="$tmp" PM_PUSH_LOG="$tmp/pushed.txt" \
    sh 3ds/push_3ds.sh --no-build --ip 127.0.0.1 -t 1 --idle 1 2>&1) && re=0 || re=$?
t1=$(date +%s)
# The run ends in PUSH FAILED stage=no log (no game log on this host): the send itself is what is tested, so rc is not checked.
if ! echo "$oe" | grep -q 'killing 3dslink' && echo "$oe" | grep -q 'send cap 195s for 13 MB' && [ $((t1 - t0)) -ge 15 ]; then echo "ok   slow send not killed"
else echo "FAIL slow send not killed (rc $re, $((t1 - t0)) s)"; echo "$oe" | tail -n 20; fail=1; fi

# (f) hung send (PUSHFIX2): one progress line, then silence. NOGROW_SECS=4 kills each attempt; 3 attempts of about 6 s, well under 45 s.
t0=$(date +%s)
of=$(env PM_3DSLINK="$fake_hung" NOGROW_SECS=4 PM_SRC3DSX="$tmp/pm_3ds.3dsx" PM_ARCHIVE_DIR="$tmp" PM_PUSH_LOG="$tmp/pushed.txt" \
    sh 3ds/push_3ds.sh --no-build --ip 127.0.0.1 -t 1 --idle 1 2>&1) || true
t1=$(date +%s)
if echo "$of" | grep -q 'no send progress for 4s - killing 3dslink' && [ $((t1 - t0)) -lt 45 ]; then echo "ok   hung send killed ($((t1 - t0)) s)"
else echo "FAIL hung send killed ($((t1 - t0)) s)"; echo "$of" | tail -n 20; fail=1; fi

rm -f build3ds/out/push_127.0.0.1_*
if [ "$fail" -eq 0 ]; then echo "PUSH TESTS PASS"; else echo "PUSH TESTS FAIL"; fi
exit $fail
