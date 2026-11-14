#!/bin/sh
# Host test of 3ds/tools/entapply.py. Run from the repo root: sh 3ds/tests/entapply/run.sh
# The case (case/fx.in.c + case/entcap.txt) is run in a temp dir; src/effects/fx.c and the stdout must equal case/fx.out.c and case/stdout.out.
# A seeded fault (the byte order of the 12 desc bytes reversed) must fail the case, so the compare is shown to bite.
# Prints "entapply: N/N PASS"; exit 0 only when the case passes and the seeded fault is caught.
set -u
ROOT=$(git rev-parse --show-toplevel) || exit 2
TOOL="$ROOT/3ds/tools/entapply.py"
CASE="$ROOT/3ds/tests/entapply/case"
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
PY=python
command -v python >/dev/null 2>&1 || PY=python3

# run_case <tool>: prints 1 to $T/pass when the case matches, else 0
run_case() {
    w="$T/w.$$.$RANDOM"; rm -rf "$w"; mkdir -p "$w/src/effects"; cp "$CASE/fx.in.c" "$w/src/effects/fx.c"; cp "$CASE/entcap.txt" "$w/entcap.txt"
    ( cd "$w" && $PY -I "$1" entcap.txt > stdout.txt ) || { echo "FAIL tool exit"; echo 0; return; }
    if cmp -s "$w/src/effects/fx.c" "$CASE/fx.out.c" && cmp -s "$w/stdout.txt" "$CASE/stdout.out"; then echo 1; else echo "FAIL fx"; echo 0; fi
}

sed 's/8 \* k/8 * (3 - k)/' "$TOOL" > "$T/entapply_fault.py"
cmp -s "$TOOL" "$T/entapply_fault.py" && { echo "entapply: seeded fault did not change the tool"; exit 1; }
fault=$(run_case "$T/entapply_fault.py")
[ "$(echo "$fault" | tail -1)" = 0 ] || { echo "entapply: seeded fault not caught"; exit 1; }
ok=$(run_case "$TOOL")
[ "$(echo "$ok" | tail -1)" = 1 ] || { echo "$ok"; echo "entapply: 0/1 PASS"; exit 1; }
echo "entapply: 1/1 PASS"
