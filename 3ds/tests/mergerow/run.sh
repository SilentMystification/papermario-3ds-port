#!/bin/sh
# Host test for 3ds/tools/merge_row.sh: seven cases in temp git repos. Run from the repo root:
#   sh 3ds/tests/mergerow/run.sh
# Prints "mergerow: N/9 PASS"; exit 0 only when all nine pass. wt_gc is a stub (WT_GC), post_merge.sh too (POST_MERGE).
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
cat > "$T/post_stub.sh" <<'STUB'
#!/bin/sh
# stub for post_merge.sh: fails when the file post_fail exists in the repo top
echo "post_merge stub"
[ -f post_fail ] && exit 3
exit 0
STUB
export POST_MERGE="$T/post_stub.sh"

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

# (g) TOOLB4 (S7TEX1): branch and main both edit runs.jsonl: merged, main's version kept, no unmerged file left
mk g; git checkout -q -b X; echo g > g.txt; printf '{"run_id":1}\n{"run_id":5,"branch":1}\n' > 3ds/logs/perf/runs.jsonl
git add g.txt 3ds/logs/perf/runs.jsonl; git commit -q -m "X: g, runs"
git checkout -q main; printf '{"run_id":1}\n{"run_id":4,"main":1}\n' > 3ds/logs/perf/runs.jsonl
git commit -q -am "main: runs"
rc=$(run)
p=no
if [ "$rc" = 0 ] && [ "$(cat 3ds/logs/perf/runs.jsonl)" = "$(printf '{"run_id":1}\n{"run_id":4,"main":1}')" ] \
   && [ -f g.txt ] && [ -z "$(git ls-files -u)" ] && [ -z "$(git rev-parse --verify -q refs/heads/X)" ]; then p=yes; fi
result $p "(g) runs.jsonl edited on branch and main: merged, main's version kept rc=$rc"

# (h) MAINBIN: post_merge fails: the merge stays, exit 7, the output file names the stub
mk h; printf 'post_fail
' >> .gitignore; git commit -q -am "ignore post_fail"; git checkout -q -b X; echo g > g.txt; git add g.txt; git commit -q -m "X: g"
git checkout -q main; touch post_fail
rc=$(run)
p=no
if [ "$rc" = 7 ] && [ -f g.txt ] && grep -q "post_merge rc 3" "$OUT" && grep -q "post_merge stub" build3ds/out/post_merge_*.txt; then p=yes; fi
result $p "(h) post_merge fails: merged, exit 7 rc=$rc"

# (i) MAINBIN: PM_POSTMERGE_EVERY=2 after the first merge: post_merge skipped, exit 0, no output file
mk i; git checkout -q -b X; echo g > g.txt; git add g.txt; git commit -q -m "X: g"
git checkout -q main; echo m > main.txt; git add main.txt; git commit -q -m "main: m"
PM_POSTMERGE_EVERY=2 sh "$SCRIPT" X "$MSG" > "$OUT" 2>&1; rc=$?
p=no
if [ "$rc" = 0 ] && [ -f g.txt ] && grep -q "post_merge skipped (merge 1, every 2)" "$OUT"    && ! ls build3ds/out/post_merge_* >/dev/null 2>&1; then p=yes; fi
result $p "(i) PM_POSTMERGE_EVERY=2, merge 1: post_merge skipped rc=$rc"

if [ "$pass" = "$n" ]; then echo "mergerow: $pass/$n PASS"; exit 0; fi
echo "mergerow: $pass/$n PASS"; exit 1
