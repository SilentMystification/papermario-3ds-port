#!/bin/sh
# BUILDLOCK host test (no Docker, no Azahar): a fake docker on PATH (info at once, run sleeps 3 s). Five wrapper runs take a
# slot, run the fake docker, release. Checks: all 5 start and end, at most 3 overlap, 3 do overlap. Sixth case: a slot dir
# whose holder pid is dead is taken over. Prints "buildlock test: ok" or "buildlock test: FAIL <check>".
set -e
cd "$(dirname "$0")/../../.."
export PM_BUILD_SLOTS=3 # the test checks 3 slots
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
# SLOTFIX seeded race: 12 fake builders on 4 slots while a cleaner loop plants dead-holder slots in free slot names and reaps
# every slot (FACT post_merge 6600a1eedd: a reaper moved a fresh slot away, 'slot4.d/pid: No such file'). Each builder must take
# a slot, still own it at its end, and exit 0; at most 4 builders hold a slot at once; no slot dir is left.
R="$T/race"; rm -rf "$R"; mkdir -p "$R/slots" "$R/rec"
cat > "$R/b.sh" <<'RB'
#!/bin/sh
. "$ROOT/3ds/tools/build_slot.sh"
build_slot_take || exit 11
echo "start $(date +%s%N)" > "$R/rec/$1"
sleep 1
[ "$(cut -d' ' -f1 "$BUILD_SLOT_DIR/pid" 2>/dev/null)" = "$$" ] || { echo "lost slot $BUILD_SLOT" >> "$R/rec/$1"; exit 12; }
echo "end $(date +%s%N)" >> "$R/rec/$1"
build_slot_release
RB
cat > "$R/clean.sh" <<'RC'
#!/bin/sh
. "$ROOT/3ds/tools/build_slot.sh"
sh -c 'exit 0' & dp=$!; wait $dp
while [ ! -f "$R/stop" ]; do
    for k in 1 2 3 4; do
        s="$R/slots/slot$k.d"
        if [ ! -e "$s" ] && mkdir "$R/slots/.plant" 2>/dev/null; then
            printf '%s %s plant\n' "$dp" "$(date +%s)" > "$R/slots/.plant/pid"; mv -T "$R/slots/.plant" "$s" 2>/dev/null || rm -rf "$R/slots/.plant"
        fi
        build_slot_reap "$s" "$k" >/dev/null
    done
done
RC
export R
( BUILDSLOT_DIR="$R/slots" sh "$R/clean.sh" ) & cp=$!
pids=""
for i in 1 2 3 4 5 6 7 8 9 10 11 12; do ( BUILDSLOT_DIR="$R/slots" PM_BUILD_SLOTS=4 PM_LANE=race$i sh "$R/b.sh" r$i > "$R/rec/r$i.out" 2>&1; echo $? > "$R/rec/r$i.rc" ) & pids="$pids $!"; done
for p in $pids; do wait $p || true; done
touch "$R/stop"; wait $cp || true
for i in 1 2 3 4 5 6 7 8 9 10 11 12; do
    [ "$(cat "$R/rec/r$i.rc" 2>/dev/null)" = 0 ] || fail "race builder $i exit $(cat "$R/rec/r$i.rc"): $(cat "$R/rec/r$i" "$R/rec/r$i.out" 2>/dev/null | tr '\n' ' ')"
done
cat "$R"/rec/r? "$R"/rec/r1? | awk '$1=="start"{print $2, 1} $1=="end"{print $2, 0}' | sort -k1,1n -k2,2n > "$R/events.txt"
rov=$(awk '{ if ($2==1) c++; else c--; if (c>m) m=c } END{print m+0}' "$R/events.txt")
echo "race: 12 builders, 4 slots, cleaner with planted dead slots: all exit 0, max overlap $rov"
[ "$rov" -le 4 ] || fail "race overlap $rov > 4"
# A slot dir can stay only with a dead holder (a reaper that gave a moved slot back after its holder ended): the next take
# reaps it. One reap pass must leave no slot dir.
. 3ds/tools/build_slot.sh
for k in 1 2 3 4; do s="$R/slots/slot$k.d"; [ -e "$s" ] || continue; build_slot_reap "$s" "$k" >/dev/null || fail "race: slot $k held after the end: $(cat "$s/pid" 2>/dev/null)"; done
ls -d "$R/slots"/.take.* >/dev/null 2>&1 && fail "race: a .take dir was left"
# SLOTFIX seeded interleaving (deterministic): reaper A judges a dead slot, then its mv waits 2 s (a PATH mv shim); builder B
# reaps the same slot, takes it and holds it; then A's mv runs. A must not take B's slot away (the old code moved and removed it).
Q="$T/ilv"; rm -rf "$Q"; mkdir -p "$Q/slots/slot1.d" "$Q/slow"
REALMV=$(command -v mv)
printf '#!/bin/sh\ncase "$2" in *.dead.*) sleep 2 ;; esac\nexec %s "$@"\n' "$REALMV" > "$Q/slow/mv"; chmod +x "$Q/slow/mv"
sh -c 'exit 0' & dq=$!; wait $dq
printf '%s %s deadilv\n' "$dq" "$(date +%s)" > "$Q/slots/slot1.d/pid"
cat > "$Q/b.sh" <<'QB'
#!/bin/sh
. "$ROOT/3ds/tools/build_slot.sh"
build_slot_take || exit 11
sleep 4
[ "$(cut -d' ' -f1 "$BUILD_SLOT_DIR/pid" 2>/dev/null)" = "$$" ] || { echo "lost slot"; exit 12; }
build_slot_release
QB
( PATH="$Q/slow:$PATH"; BUILDSLOT_DIR="$Q/slots"; . 3ds/tools/build_slot.sh; build_slot_reap "$Q/slots/slot1.d" 1 >/dev/null; true ) & ap=$!
sleep 1
qrc=0; qout=$(BUILDSLOT_DIR="$Q/slots" PM_BUILD_SLOTS=1 PM_LANE=ilv sh "$Q/b.sh" 2>&1) || qrc=$?
wait $ap || true
[ "$qrc" = 0 ] || fail "interleaving: builder B lost its slot to a late reaper (exit $qrc): $qout"
echo "interleaving: a late reaper left the retaken slot to its holder"
echo "buildlock test: ok"
