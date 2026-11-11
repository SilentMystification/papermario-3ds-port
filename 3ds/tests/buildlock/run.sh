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
echo "buildlock test: ok"
