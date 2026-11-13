#!/bin/sh
# PUSHLANE test: run after `PM_LANE=<lane> sh 3ds/build.sh` in a worktree, with PM_LANE=<lane>. A fake 3dslink, no console.
set -u
cd "$(dirname "$0")/../../.."
[ -n "${PM_LANE:-}" ] && [ -f "build3ds/lane-$PM_LANE/pm_3ds.3dsx" ] || { echo "LANE TEST FAIL: no lane build of PM_LANE='${PM_LANE:-}'"; exit 1; }
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
printf '#!/bin/sh\necho "Sending pm_3ds.3dsx"\nexit 0\n' >"$tmp/fake_ok.sh"; chmod +x "$tmp/fake_ok.sh"
o=$(env PM_3DSLINK="$tmp/fake_ok.sh" PM_ARCHIVE_DIR="$tmp" PM_PUSH_LOG="$tmp/pushed.txt" \
    powershell -NoProfile -File 3ds/go.ps1 push o -NoBuild -Fg -ip 127.0.0.1 -Seconds 5 -IdleSeconds 5 2>&1) || true
if echo "$o" | grep -Eq "SEND FILE: .*lane-$PM_LANE[\/]pm_3ds\.3dsx" && ! echo "$o" | grep -q 'STALE 3DSX' && echo "$o" | grep -q '^Sending'; then echo "LANE TEST PASS"; exit 0; fi
echo "LANE TEST FAIL"; echo "$o" | tail -n 20; exit 1
