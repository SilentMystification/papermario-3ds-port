#!/bin/sh
# POSTMERGE (MAINBIN): run by 3ds/tools/merge_row.sh in the main folder after each merge (cache safety rule 5,
# docs/handoff/plan_buildspeed.md). 1: cachecheck of the generator cache (the generators run cold and compare with their
# entries). 2: the base build of main HEAD goes into the basebin store, so a row started from this main finds its base built.
# 3: cachecheck of basebin and of the stored shots (one cold rebuild, one shot rerun). 4: the runs of step 3 are committed
# (runs.jsonl is tracked; the main folder stays clean). Exit 0 when all pass, else the number of the failing step.
cd "$(git rev-parse --show-toplevel)" || exit 9
PY=python; command -v py >/dev/null 2>&1 && PY="py -3"
unset PM_LANE
export PM_ROW=MAINBIN
export PM_BUILD_CAP=${PM_BUILD_CAP:-1800} # step 3 builds with no ccache (GUESS 5-10 min): above the 600 s default cap
echo "post_merge 1: generator cachecheck"
PM_CACHE=check PM_KEY_ONLY=1 sh 3ds/build.sh > /dev/null || exit 1
echo "post_merge 2: basebin prebuild of main HEAD"
$PY 3ds/tools/bench.py base --prebuild --row MAINBIN || exit 2
echo "post_merge 3: basebin and shots cachecheck"
$PY 3ds/tools/bench.py base --check --row MAINBIN || exit 3
if [ -n "$(git status --porcelain -- 3ds/logs/perf/runs.jsonl)" ]; then
    git add -- 3ds/logs/perf/runs.jsonl && git commit -q -m "runs.jsonl: post_merge cachecheck runs" || exit 4
fi
echo "post_merge ok"
