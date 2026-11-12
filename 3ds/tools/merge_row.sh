#!/bin/sh
# merge_row.sh <row> <message-file>: merge the finished row branch <row> into main, append its new runs.jsonl lines,
# then remove the row worktree and the branch. Run in the main folder (Git Bash):
#   sh 3ds/tools/merge_row.sh <row> <message-file>
# Exit: 0 merged (summary printed); 2 refused (main dirty, branch missing, not on main); 3 conflict other than the row
# file (or merge-tree failed); 4 conflict marker line added on the branch; 5 merge failed and aborted (or runs commit
# failed); 6 branch kept (not merged, or still checked out).
# WT_GC: a wt_gc.sh replacement read by the host test (run as: sh "$WT_GC" --apply --only <row>).
set -u
row=${1:-}; msg=${2:-}
if [ -z "$row" ] || [ -z "$msg" ]; then echo "usage: sh 3ds/tools/merge_row.sh <row> <message-file>" >&2; exit 2; fi
here=$(pwd)
case "$msg" in /*|[A-Za-z]:*) ;; *) msg="$here/$msg" ;; esac
top=$(git rev-parse --show-toplevel 2>/dev/null) || { echo "merge_row: refused: not in a git repo" >&2; exit 2; }
cd "$top" || exit 2
rowfile="docs/handoff/row_$row.md"
tmpd=$(mktemp -d) || exit 2
trap 'rm -rf "$tmpd"' EXIT

# 1. refuse
if [ -n "$(git status --porcelain)" ]; then
    echo "merge_row: refused: main folder has uncommitted or untracked files" >&2; exit 2
fi
if ! git rev-parse --verify -q "refs/heads/$row" >/dev/null; then
    echo "merge_row: refused: branch $row does not exist" >&2; exit 2
fi
cur=$(git symbolic-ref --short -q HEAD || true)
if [ "$cur" != main ]; then
    echo "merge_row: refused: current branch is '$cur', not main" >&2; exit 2
fi

# 2. conflict check without touching the tree
git merge-tree --write-tree --name-only main "$row" > "$tmpd/mt" 2>/dev/null; rc=$?
case $rc in
    0) ;;
    1)
        conf=$(awk 'NR>1 && /^$/ {exit} NR>1 {print}' "$tmpd/mt")
        n=$(printf '%s\n' "$conf" | awk 'NF {n++} END {print n+0}')
        if [ "$n" != 1 ] || [ "$conf" != "$rowfile" ]; then
            echo "merge_row: conflicts in:" >&2
            printf '%s\n' "$conf" | sed 's/^/  /' >&2
            exit 3
        fi ;;
    *) echo "merge_row: merge-tree failed rc=$rc" >&2; exit 3 ;;
esac

# 3. conflict markers added on the branch
marks=$(git diff "main...$row" | awk '/^\+\+\+ /{f=$2; sub(/^b\//,"",f)} /^\+(<<<<<<<|=======|>>>>>>>)( |$)/{print f": "substr($0,2)}')
if [ -n "$marks" ]; then
    echo "merge_row: conflict marker lines added on $row:" >&2
    printf '%s\n' "$marks" >&2
    exit 4
fi

# 4. merge (the row file deleted on the branch and edited on main is the one conflict allowed)
if ! git merge --no-ff -F "$msg" "$row" > "$tmpd/merge" 2>&1; then
    unm=$(git diff --name-only --diff-filter=U)
    if [ "$unm" = "$rowfile" ] && git rm -q "$rowfile" && git commit -q --no-edit -F "$msg"; then
        :
    else
        cat "$tmpd/merge" >&2
        git merge --abort
        echo "merge_row: merge of $row failed, aborted; unmerged: $(printf '%s' "$unm" | tr '\n' ' ')" >&2
        exit 5
    fi
fi
mhash=$(git rev-parse --short HEAD)

# 5. runs.jsonl: lines of the row worktree copy whose run_id main does not have, appended in file order
wrun="$top/build3ds/wt/$row/3ds/logs/perf/runs.jsonl"
mrun="$top/3ds/logs/perf/runs.jsonl"
added=0; skipped=0
if [ -f "$wrun" ]; then
    awk -v sk="$tmpd/skip" '
        function rid(s,  v) {
            if (match(s, /"run_id":[^,}]*/)) { v = substr(s, RSTART + 9, RLENGTH - 9); gsub(/\r/, "", v); return v }
            return ""
        }
        FNR == NR { r = rid($0); if (r != "") known[r] = 1; next }
        $0 == "" || $0 == "\r" { next }
        { r = rid($0)
          if (r == "" || !(r in known)) { print; if (r != "") known[r] = 1 }
          else skip++ }
        END { print skip + 0 > sk }
    ' "$mrun" "$wrun" > "$tmpd/new"
    added=$(awk 'END {print NR}' "$tmpd/new")
    skipped=$(cat "$tmpd/skip")
    if [ "$added" -gt 0 ]; then
        last=$(tail -c 1 "$mrun" | od -An -tx1 | tr -d ' ')
        if [ -s "$mrun" ] && [ "$last" != 0a ]; then printf '\n' >> "$mrun"; fi
        cat "$tmpd/new" >> "$mrun"
        git add -- 3ds/logs/perf/runs.jsonl
        if ! git commit -q -m "runs.jsonl: $row runs"; then
            echo "merge_row: runs.jsonl commit failed" >&2; exit 5
        fi
    fi
fi

# 6. remove the row worktree (wt_gc) and the branch (-d: only when merged)
if [ -n "${WT_GC:-}" ]; then sh "$WT_GC" --apply --only "$row"; else sh 3ds/tools/wt_gc.sh --apply --only "$row"; fi
gcrc=$?
if git branch -d "$row" >/dev/null 2>&1; then bstate=deleted; else bstate=kept; fi

# 7. summary
echo "merge_row: merge $mhash of $row; runs lines added $added, known skipped $skipped; wt_gc rc $gcrc; branch $row $bstate"
if [ "$bstate" = kept ]; then
    echo "merge_row: branch $row kept: not merged into main, or still checked out" >&2
    exit 6
fi
exit 0
