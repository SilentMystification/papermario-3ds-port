#!/bin/sh
# wt_new.sh <row>: a new worktree build3ds/wt/<row> on branch <row> from main, with the .wt_keep file wt_gc.sh reads.
# Nothing else: no asset copy. build.sh and nd_host_test.sh read assets/us and ver/us/build from the main worktree.
# Usage (any worktree or the main one): sh 3ds/tools/wt_new.sh <row>
set -e
row=$1
[ -n "$row" ] || { echo "usage: sh 3ds/tools/wt_new.sh <row>" >&2; exit 2; }
MAIN=$(cd "$(git rev-parse --git-common-dir)/.." && pwd)
cd "$MAIN"
mkdir -p build3ds/wt
git worktree add "build3ds/wt/$row" -b "$row" main
touch "build3ds/wt/$row/.wt_keep"
echo "worktree build3ds/wt/$row on branch $row (from main)"
echo "build: PM_ROW=$row PM_LANE=$row sh 3ds/build.sh"
