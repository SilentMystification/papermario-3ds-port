#!/bin/sh
# Host test for 3ds/tools/merge_row.sh: six cases in temp git repos. Run from the repo root:
#   sh 3ds/tests/mergerow/run.sh
# Prints "mergerow: N/6 PASS"; exit 0 only when all six pass. wt_gc is a stub (WT_GC).
set -u
ROOT=$(git rev-parse --show-toplevel) || exit 2
SCRIPT="$ROOT/3ds/tools/merge_row.sh"
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
MSG="$T/msg.txt"; echo "merge X" > "$MSG"
OUT="$T/out.txt"

cat > "$T/wt_gc_stub.sh" <<'STUB'
#!/bin/sh
# stub for wt_gc.sh --apply --only <row>: removes the row worktree only
echo "wt_gc stub: $*"
row=$3
if [ -d "build3ds/wt/$row" ]; then git worktree remove --force "build3ds/wt/$row"; fi
exit 0
STUB
export WT_GC="$T/wt_gc_stub.sh"

mk() {
    d="$T/r$1"; mkdir -p "$d"; cd "$d" || exit 2
    git init -q -b main && git config user.email t@t && git config user.name t && git config core.autocrlf false
    mkdir -p docs/handoff 3ds/logs/perf
    echo "row X" > docs/handoff/row_X.md
    echo base > f.txt
    printf '{"run_id":1}\n' > 3ds/logs/perf/runs.jsonl
    printf 'build3ds/\n' > .gitignore
    git add -A && git commit -q -m base
}
run() { sh "$SCRIPT" X "$MSG" > "$OUT" 2>&1; echo $?; }
pass=0; n=0
result() {
    n=$((n+1))
    if [ "$1" = yes ]; then pass=$((pass+1)); echo "  PASS $2"; else echo "  FAIL $2"; sed 's/^/      /' "$OUT"; fi
}

# (a) clean branch: merged, branch deleted, merge commit
mk a; git checkout -q -b X; echo g > g.txt; git add g.txt; git commit -q -m "X: g"
git checkout -q main; echo m > main.txt; git add main.txt; git commit -q -m "main: m"
rc=$(run)
p=no
if [ "$rc" = 0 ] && [ -f g.txt ] && [ -z "$(git rev-parse --verify -q refs/heads/X)" ] \
   && [ "$(git cat-file -p HEAD | grep -c '^parent')" = 2 ]; then p=yes; fi
result $p "(a) clean branch merged, branch deleted rc=$rc"

# (b) branch deleted the row file, main edited it: merged, file gone
mk b; git checkout -q -b X; git rm -q docs/handoff/row_X.md; echo g > g.txt; git add g.txt; git commit -q -m "X: g, drop row file"
git checkout -q main; echo edited > docs/handoff/row_X.md; git commit -q -am "main: edit row file"
rc=$(run)
p=no
if [ "$rc" = 0 ] && [ ! -e docs/handoff/row_X.md ] && ! git cat-file -e HEAD:docs/handoff/row_X.md 2>/dev/null \
   && [ -z "$(git rev-parse --verify -q refs/heads/X)" ]; then p=yes; fi
result $p "(b) row file deleted on branch, edited on main: merged rc=$rc"

# (c) real conflict in f.txt: exit 3, main unchanged
mk c; git checkout -q -b X; echo branch > f.txt; git commit -q -am "X: f"
git checkout -q main; echo main > f.txt; git commit -q -am "main: f"
before=$(git rev-parse HEAD)
rc=$(run)
p=no
if [ "$rc" = 3 ] && [ "$(git rev-parse HEAD)" = "$before" ] && [ -z "$(git status --porcelain)" ]; then p=yes; fi
result $p "(c) real conflict: exit 3, main unchanged rc=$rc"

# (d) branch adds a conflict marker line: exit 4, main unchanged
mk d; git checkout -q -b X; printf '<<<<<<< HEAD\n' > m2.txt; git add m2.txt; git commit -q -m "X: marker"
git checkout -q main; echo main > main.txt; git add main.txt; git commit -q -m "main: m"
before=$(git rev-parse HEAD)
rc=$(run)
p=no
if [ "$rc" = 4 ] && [ "$(git rev-parse HEAD)" = "$before" ]; then p=yes; fi
result $p "(d) marker line on branch: exit 4 rc=$rc"

# (e) dirty main: exit 2
mk e; git checkout -q -b X; echo g > g.txt; git add g.txt; git commit -q -m "X: g"
git checkout -q main; echo dirty > dirty.txt
before=$(git rev-parse HEAD)
rc=$(run)
p=no
if [ "$rc" = 2 ] && [ "$(git rev-parse HEAD)" = "$before" ]; then p=yes; fi
result $p "(e) dirty main: exit 2 rc=$rc"

# (f) runs.jsonl: worktree copy has run_id 1 (known), 2 and 3 (new); 2 appended, 1 skipped
mk f; git worktree add -q build3ds/wt/X -b X main
( cd build3ds/wt/X && echo g > g.txt && git add g.txt && git commit -q -m "X: g" \
  && printf '{"run_id":1}\n{"run_id":2}\n{"run_id":3}\n' > 3ds/logs/perf/runs.jsonl )
rc=$(run)
p=no
if [ "$rc" = 0 ] && [ "$(wc -l < 3ds/logs/perf/runs.jsonl | tr -d ' ')" = 3 ] \
   && [ "$(git log -1 --format=%s)" = "runs.jsonl: X runs" ] \
   && grep -q "runs lines added 2, known skipped 1" "$OUT" \
   && [ ! -d build3ds/wt/X ]; then p=yes; fi
result $p "(f) runs.jsonl: 2 appended, 1 known skipped rc=$rc"

if [ "$pass" = "$n" ]; then echo "mergerow: $pass/$n PASS"; exit 0; fi
echo "mergerow: $pass/$n PASS"; exit 1
