#!/bin/sh
# HOSTSLOT host test (no Docker, no Azahar): 3ds/tools/nd_host_test.sh takes a build slot before its docker runs.
# HOLD slots are held by live sleeping holders (BUILDSLOT_DIR in build3ds/out). A fake docker on PATH records each `run`.
# With all 3 slots held, nd_host_test.sh must not run docker for 5 s. After one slot is released it runs docker.
# Prints "hostslot test: ok" or "hostslot test: FAIL <check>".
HOLD=3
cd "$(dirname "$0")/../../.." || exit 2
ROOT=$(pwd)
T="$ROOT/build3ds/out/hostslot_test"
rm -rf "$T"; mkdir -p "$T/bin" "$T/slots"
cat > "$T/bin/docker" <<'DOCK'
#!/bin/sh
case "$1" in
  info) exit 0 ;;
  run) echo run >> "$REC"; exit 0 ;;
esac
exit 0
DOCK
chmod +x "$T/bin/docker"
REC="$T/rec"; export REC
BUILDSLOT_DIR="$T/slots"; export BUILDSLOT_DIR
PATH="$T/bin:$PATH"; export PATH

HOLDPIDS=""
hold() { # $1 slot number
    sleep 120 & p=$!
    mkdir -p "$T/slots/slot$1.d"
    printf '%s %s %s %s\n' "$p" "$(date +%s)" "hold$1" "" > "$T/slots/slot$1.d/pid"
    HOLDPIDS="$HOLDPIDS $p"
    [ "$1" = 1 ] && HOLDPID1=$p
}
cleanup() {
    kill $HOLDPIDS 2>/dev/null
    [ -n "$ndpid" ] && kill "$ndpid" 2>/dev/null
    rm -rf "$T/slots"
}

k=1
while [ "$k" -le "$HOLD" ]; do hold "$k"; k=$((k + 1)); done

timeout 120 sh "$ROOT/3ds/tools/nd_host_test.sh" > "$T/nd.txt" 2>&1 &
ndpid=$!
i=0
while [ "$i" -lt 5 ]; do sleep 1; i=$((i + 1)); done
if [ -f "$REC" ]; then cleanup; echo "hostslot test: FAIL waits"; exit 1; fi

# Release slot 1: its holder dies and its dir goes. nd_host_test.sh takes it and runs docker.
kill "$HOLDPID1" 2>/dev/null
rm -rf "$T/slots/slot1.d"
wait "$ndpid" 2>/dev/null
ndpid=""
if [ ! -f "$REC" ] || ! grep -q run "$REC"; then cleanup; echo "hostslot test: FAIL runs"; exit 1; fi
cleanup
echo "hostslot test: ok"
