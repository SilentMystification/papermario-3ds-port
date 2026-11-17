#!/bin/sh
# POSTMERGE (MAINBIN, POSTASYNC): the post-merge check of one main commit (cache safety rule 5, docs/handoff/plan_buildspeed.md).
# 3ds/tools/postmerge_bg.sh runs it detached in the worktree build3ds/wt/postmerge (checked out at the commit), never in the
# main folder:  sh 3ds/tools/post_merge.sh <commit> <cold 0|1> <shots 0|1>
# 1 (every commit): cachecheck of the generator cache and the gconv object cache (the generators run cold and compare with
# their entries). 2 (every commit): the base build of <commit> goes into the basebin store, so a row started from it finds its
# base built. 3: basebin cachecheck. Every commit: entries whose content key changed are removed. cold 1: a cold build
# (PM_CACHE=off: no cache, no ccache, new build dir, new asset volume) of <commit> must equal its entry byte for byte.
# shots 1: one stored bench runs again and its shots must equal the stored ones.
# It commits nothing (postmerge_bg.sh moves the new runs.jsonl lines to the main folder). Exit 0 when all pass, else the step.
cd "$(git rev-parse --show-toplevel)" || exit 9
c=${1:?usage: sh 3ds/tools/post_merge.sh <commit> <cold 0|1> <shots 0|1>}; cold=${2:-1}; shots=${3:-1}
PY=python; command -v py >/dev/null 2>&1 && PY="py -3"
export PM_ROW=POSTMERGE PM_LANE=postmerge
export PM_BUILD_CAP=${PM_BUILD_CAP:-1800} # the cold build has no ccache (FACT 540 s, MAINBIN): above the 600 s default cap
echo "post_merge $c cold $cold shots $shots"
echo "post_merge 1: generator cachecheck"
PM_CACHE=check PM_KEY_ONLY=1 sh 3ds/build.sh > /dev/null || exit 1
echo "post_merge 2: basebin prebuild of $c"
$PY 3ds/tools/bench.py base --prebuild --commit "$c" --row POSTMERGE || exit 2
fl=""; [ "$cold" = 1 ] && fl="$fl --cold"; [ "$shots" = 1 ] && fl="$fl --shots"
echo "post_merge 3: basebin cachecheck$fl"
$PY 3ds/tools/bench.py base --check --commit "$c"$fl --row POSTMERGE || exit 3
echo "post_merge ok"
