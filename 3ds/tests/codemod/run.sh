#!/bin/sh
# Host test of 3ds/tools/codemod.py. Run from the repo root: sh 3ds/tests/codemod/run.sh
# Each cases/<n>.in.c is rewritten in a temp dir; the result must equal cases/<n>.out.c and the keep lines cases/<n>.log, byte for byte.
# First a seeded fault (a copy of the tool with the env mapping pointing at nd_val_prim) must fail at least one case, so the compare is
# shown to bite. Prints "codemod: N/N PASS"; exit 0 only when all pass.
set -u
ROOT=$(git rev-parse --show-toplevel) || exit 2
TOOL="$ROOT/3ds/tools/codemod.py"
CASES="$ROOT/3ds/tests/codemod/cases"
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
PY=python
command -v python >/dev/null 2>&1 || PY=python3

# run_all <tool>: prints the number of passing cases to $T/pass and the total to $T/total; failures go to stdout
run_all() {
    tool=$1; pass=0; total=0
    for in in "$CASES"/*.in.c; do
        n=$(basename "$in" .in.c); total=$((total + 1))
        w="$T/w.$n"; rm -rf "$w"; mkdir -p "$w"; cp "$in" "$w/$n.c"
        fl=; case "$n" in st_*) fl=--state ;; esac  # the state cases (st_*) run with --state, the others without
        ( cd "$w" && $PY "$tool" $fl "$n.c" > "$n.log" ) || { echo "FAIL $n: tool exit"; continue; }
        if cmp -s "$w/$n.c" "$CASES/$n.out.c" && cmp -s "$w/$n.log" "$CASES/$n.log"; then pass=$((pass + 1)); else echo "FAIL $n"; fi
    done
    echo "$pass" > "$T/pass"; echo "$total" > "$T/total"
}

sed "s/'nd_val_env'/'nd_val_prim'/" "$TOOL" > "$T/codemod_fault.py"
cmp -s "$TOOL" "$T/codemod_fault.py" && { echo "codemod: seeded fault did not change the tool"; exit 1; }
run_all "$T/codemod_fault.py" > "$T/fault.out"
cat "$T/fault.out"
if [ "$(cat "$T/pass")" = "$(cat "$T/total")" ]; then echo "codemod: seeded fault NOT caught"; exit 1; fi
echo "codemod: seeded fault caught ($(cat "$T/pass")/$(cat "$T/total") pass)"

run_all "$TOOL"
p=$(cat "$T/pass"); t=$(cat "$T/total")
echo "codemod: $p/$t PASS"
[ "$p" = "$t" ]
