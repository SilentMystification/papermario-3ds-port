#!/bin/sh
# Host test for 3ds/tools/merge_row.sh: eleven cases in temp git repos. Run from the repo root:
#   sh 3ds/tests/mergerow/run.sh
# Prints "mergerow: N/11 PASS"; exit 0 only when all pass. wt_gc is a stub (WT_GC). The real postmerge_bg.sh (POSTMERGE_BG)
# runs a post_merge.sh stub (POST_MERGE) detached; each case waits for its .done file (60 s cap).
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
# stub for post_merge.sh <commit> <cold> <shots>: PM_STUB_RUN: a line for runs.jsonl; PM_STUB_SLEEP: seconds; PM_STUB_FAIL: exit 3
echo "post_merge stub $*"
[ -n "${PM_STUB_RUN:-}" ] && printf '%s\n' "$PM_STUB_RUN" >> 3ds/logs/perf/runs.jsonl
sleep "${PM_STUB_SLEEP:-0}"
[ -n "${PM_STUB_FAIL:-}" ] && exit 3
exit 0
STUB
export POST_MERGE="$T/post_stub.sh"
export POSTMERGE_BG="$ROOT/3ds/tools/postmerge_bg.sh"

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
# pmwait: wait (60 s cap) for the .done file of the main HEAD when merge_row queued a check
pmwait() {
    grep -q "post_merge queued" "$OUT" || return 0
    i=0; while [ ! -f "build3ds/out/post_merge_$(git rev-parse --short HEAD).done" ] && [ $i -lt 60 ]; do sleep 1; i=$((i+1)); done
}
run() { sh "$SCRIPT" X "$MSG" > "$OUT" 2>&1; r=$?; pmwait; echo $r; }
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

# (h) POSTASYNC: a postmerge_FAIL.txt in main: exit 8, nothing merged, branch kept
mk h; git checkout -q -b X; echo g > g.txt; git add g.txt; git commit -q -m "X: g"
git checkout -q main; mkdir -p build3ds/out; printf 'commit 0123\nstep 3\n' > build3ds/out/postmerge_FAIL.txt
rc=$(run)
p=no
if [ "$rc" = 8 ] && [ ! -f g.txt ] && [ -n "$(git rev-parse --verify -q refs/heads/X)" ] && grep -q "commit 0123" "$OUT"; then p=yes; fi
result $p "(h) post-merge FAIL file: refused, exit 8 rc=$rc"

# (i) POSTASYNC: merge_row returns before the check ends (stub sleeps 6 s); the check ends rc 0 on the merge commit
mk i; git checkout -q -b X; echo g > g.txt; git add g.txt; git commit -q -m "X: g"
git checkout -q main
t0=$(date +%s); PM_STUB_SLEEP=6 sh "$SCRIPT" X "$MSG" > "$OUT" 2>&1; rc=$?; t1=$(date +%s); pmwait
h=$(git rev-parse HEAD); s=$(git rev-parse --short HEAD)
p=no
if [ "$rc" = 0 ] && [ $((t1 - t0)) -lt 5 ] && grep -q "post_merge queued $s" "$OUT" \
   && grep -q "^post_merge $h rc 0 " "build3ds/out/post_merge_$s.done" && grep -q "post_merge stub $h" "build3ds/out/post_merge_$s.txt" \
   && [ ! -f build3ds/out/postmerge_FAIL.txt ]; then p=yes; fi
result $p "(i) detached check: merge_row took $((t1 - t0)) s, check rc 0 rc=$rc"

# (j) POSTASYNC: the check fails: postmerge_FAIL.txt names the commit; the next merge is refused (exit 8)
mk j; git checkout -q -b X; echo g > g.txt; git add g.txt; git commit -q -m "X: g"
git checkout -q -b Y main; echo y > y.txt; git add y.txt; git commit -q -m "Y: y"; git checkout -q main
PM_STUB_FAIL=1 sh "$SCRIPT" X "$MSG" > "$OUT" 2>&1; rc=$?; pmwait; h=$(git rev-parse HEAD)
sh "$SCRIPT" Y "$MSG" > "$T/out2.txt" 2>&1; rc2=$?
p=no
if [ "$rc" = 0 ] && grep -q "^commit $h" build3ds/out/postmerge_FAIL.txt && [ "$rc2" = 8 ] && [ ! -f y.txt ]; then p=yes; fi
result $p "(j) failed check writes the FAIL file, next merge refused rc=$rc rc2=$rc2"

# (k) POSTASYNC: a run line of the check (stub) reaches main runs.jsonl at the next merge
mk k; git checkout -q -b X; echo g > g.txt; git add g.txt; git commit -q -m "X: g"
git checkout -q -b Y main; echo y > y.txt; git add y.txt; git commit -q -m "Y: y"; git checkout -q main
PM_STUB_RUN='{"run_id":7}' sh "$SCRIPT" X "$MSG" > "$OUT" 2>&1; rc=$?; pmwait
grep -q '"run_id":7' build3ds/out/postmerge_runs.jsonl 2>/dev/null && r7=yes || r7=no
sh "$SCRIPT" Y "$MSG" > "$OUT" 2>&1; rc2=$?; pmwait
p=no
if [ "$rc" = 0 ] && [ "$rc2" = 0 ] && [ "$r7" = yes ] && [ "$(tail -n 1 3ds/logs/perf/runs.jsonl)" = '{"run_id":7}' ] \
   && [ "$(git log -1 --format=%s)" = "runs.jsonl: Y runs" ]; then p=yes; fi
result $p "(k) check run line added to main at the next merge rc=$rc rc2=$rc2 r7=$r7"

if [ "$pass" = "$n" ]; then echo "mergerow: $pass/$n PASS"; exit 0; fi
echo "mergerow: $pass/$n PASS"; exit 1
