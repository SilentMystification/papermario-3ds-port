#!/bin/sh
# BUILDLOCK host test (no Docker, no Azahar): a fake docker on PATH (info at once, run sleeps 3 s). Five wrapper runs take a
# slot, run the fake docker, release. Checks: all 5 start and end, at most 3 overlap, 3 do overlap. Sixth case: a slot dir
# whose holder pid is dead is taken over. Prints "buildlock test: ok" or "buildlock test: FAIL <check>".
set -e
cd "$(dirname "$0")/../../.."
ROOT=$(pwd)
T="$ROOT/build3ds/out/buildlock_test"
rm -rf "$T"; mkdir -p "$T/bin" "$T/slots" "$T/rec"
cat > "$T/bin/docker" <<'DOCK'
#!/bin/sh
case "$1" in
  info) exit 0 ;;
  run) sleep 3; exit 0 ;;
esac
exit 0
DOCK
chmod +x "$T/bin/docker"
cat > "$T/wrap.sh" <<'WRAP'
#!/bin/sh
. "$ROOT/3ds/tools/build_slot.sh"
PATH="$T/bin:$PATH"
build_slot_take
echo "start $BUILD_SLOT $(date +%s%N)" >> "$T/rec/$1"
docker run --rm x >/dev/null
echo "end $BUILD_SLOT $(date +%s%N)" >> "$T/rec/$1"
build_slot_release
WRAP
export ROOT T BUILDSLOT_DIR="$T/slots"
fail() { echo "buildlock test: FAIL $1"; exit 1; }

# Sixth case first (dead holder): a pid that has exited.
sh -c 'exit 0' & dead=$!
wait $dead
mkdir -p "$T/slots/slot1.d"
printf '%s %s %s\n' "$dead" "$(date +%s)" deadtest > "$T/slots/slot1.d/pid"
out6=$(sh "$T/wrap.sh" dead 2>&1) || fail "dead-holder run exited non-zero: $out6"
echo "$out6"
echo "$out6" | grep -q "buildslot: slot 1 of dead pid $dead removed" || fail "no dead-pid removal line: $out6"
echo "$out6" | grep -q "buildslot: slot 1 taken" || fail "slot 1 not taken after dead holder: $out6"
[ "$(wc -l < "$T/rec/dead")" -eq 2 ] || fail "dead-holder run did not start and end"
[ -z "$(ls -d "$T/slots"/slot*.d 2>/dev/null)" ] || fail "slot left after release (dead-holder case)"

# Five parallel runs.
for i in 1 2 3 4 5; do sh "$T/wrap.sh" r$i & done
wait
ls "$T"/rec/r* >/dev/null 2>&1 || fail "no records"
cat "$T"/rec/r* | awk '$1=="start"{print $3, 1} $1=="end"{print $3, 0}' | sort -k1,1n -k2,2n > "$T/events.txt"
starts=$(grep -c "start" "$T"/rec/r* | awk -F: '{s+=$2} END{print s+0}')
ends=$(grep -c "end" "$T"/rec/r* | awk -F: '{s+=$2} END{print s+0}')
[ "$starts" -eq 5 ] || fail "starts $starts, want 5"
[ "$ends" -eq 5 ] || fail "ends $ends, want 5"
maxov=$(awk '{ if ($2==1) c++; else c--; if (c>m) m=c } END{print m+0}' "$T/events.txt")
echo "max overlap $maxov (events in $T/events.txt)"
[ "$maxov" -le 3 ] || fail "overlap $maxov > 3"
[ "$maxov" -eq 3 ] || fail "overlap $maxov, want 3 (the slots were not all used)"
# No slot left behind.
[ -z "$(ls -d "$T/slots"/slot*.d 2>/dev/null)" ] || fail "slot dirs left after the runs"
# DOCKWD: engine health. Fake docker 2: info fails until $UPF exists. line.sh mirrors the build.sh lines (docker check,
# slot take, docker check after take). DOCKER_RESTART_CMD is a stub that creates $UPF (no Docker Desktop restart).
D="$T/dock"; mkdir -p "$D/bin"
UPF="$D/up"; export UPF
cat > "$D/bin/docker" <<'DK'
#!/bin/sh
case "$1" in info) [ -f "$UPF" ] && exit 0; exit 1 ;; esac
exit 0
DK
chmod +x "$D/bin/docker"
cat > "$D/line.sh" <<'LN'
#!/bin/sh
. "$ROOT/3ds/tools/build_slot.sh"
build_docker_ok || build_docker_restart || { echo "BUILD FAULT: docker engine not answering" >&2; exit 3; }
build_slot_take
build_docker_ok || { build_slot_release; echo "BUILD FAULT: docker stopped answering after the slot was taken" >&2; exit 3; }
echo "BUILD GOES ON"
build_slot_release
LN
# (a) engine down, no slot held: restart runs, build goes on.
rm -f "$UPF" "$D/restarted"
rc=0; out_a=$(PATH="$D/bin:$PATH" DOCKER_RESTART_CMD="touch $D/restarted; touch $UPF" sh "$D/line.sh" 2>&1) || rc=$?
echo "$out_a"
[ "$rc" -eq 0 ] || fail "docker case a: exit $rc, want 0: $out_a"
echo "$out_a" | grep -q "BUILD GOES ON" || fail "docker case a: build did not go on: $out_a"
[ -f "$D/restarted" ] || fail "docker case a: restart command did not run"
[ -z "$(ls -d "$T/slots"/slot*.d 2>/dev/null)" ] || fail "docker case a: slot left after the run"
# (b) engine down, one live slot (a sleeping fake build holds it): no restart, exit 3, "builds running".
rm -f "$UPF" "$D/restarted"
sleep 60 & lp=$!
mkdir -p "$T/slots/slot1.d"
printf '%s %s %s %s\n' "$lp" "$(date +%s)" dockb "" > "$T/slots/slot1.d/pid"
rc=0; out_b=$(PATH="$D/bin:$PATH" DOCKER_RESTART_CMD="touch $D/restarted; touch $UPF" sh "$D/line.sh" 2>&1) || rc=$?
kill "$lp" 2>/dev/null; wait "$lp" 2>/dev/null || true
rm -rf "$T/slots/slot1.d"
echo "$out_b"
[ "$rc" -eq 3 ] || fail "docker case b: exit $rc, want 3: $out_b"
echo "$out_b" | grep -q "docker restart skipped: 1 builds running" || fail "docker case b: no skip line: $out_b"
[ ! -f "$D/restarted" ] || fail "docker case b: restart ran with a live slot"
# (c) engine up: no restart call, build goes on.
touch "$UPF"; rm -f "$D/restarted"
rc=0; out_c=$(PATH="$D/bin:$PATH" DOCKER_RESTART_CMD="touch $D/restarted" sh "$D/line.sh" 2>&1) || rc=$?
echo "$out_c"
[ "$rc" -eq 0 ] || fail "docker case c: exit $rc, want 0: $out_c"
echo "$out_c" | grep -q "BUILD GOES ON" || fail "docker case c: build did not go on: $out_c"
[ ! -f "$D/restarted" ] || fail "docker case c: restart ran with the engine up"
echo "buildlock test: ok"
