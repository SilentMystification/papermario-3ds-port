#!/bin/sh
# POSTASYNC: the queue of the post-merge check (cache safety rule 5, docs/handoff/plan_buildspeed.md). Any folder of the repo:
#   sh 3ds/tools/postmerge_bg.sh --queue [--full] [<commit>]   adds <commit> (default HEAD) to the queue, starts the runner
#                                                              detached and returns at once (merge_row.sh step 8 calls it)
#   sh 3ds/tools/postmerge_bg.sh --run                         the runner (one at a time: the lock build3ds/out/postmerge.lock)
#   sh 3ds/tools/postmerge_bg.sh --status                      prints the queue, the runner and the FAIL file
# Files in build3ds/out of the main folder: postmerge_q/<time>.job (one line "<commit> [full]"), post_merge_<short>.txt (output),
# post_merge_<short>.done (one line "post_merge <commit> rc <rc> cold <0|1> shots <0|1>"), postmerge_runner.txt (runner log),
# postmerge_runs.jsonl (runs.jsonl lines of the checks; merge_row.sh step 5 adds them to main), postmerge_FAIL.txt.
# The runner checks the queued commits oldest first, each in the worktree build3ds/wt/postmerge (detached at the commit; never the
# main folder, so a merge can run at the same time): sh 3ds/tools/post_merge.sh <commit> <cold> <shots> (POST_MERGE: a replacement
# for tests). cold 1 on every EVERY-th merge commit (merge count mod EVERY = 0), shots 1 on the next one (mod EVERY = 1); a --full
# job, or any job while postmerge_FAIL.txt exists, runs both. rc != 0 writes postmerge_FAIL.txt: merge_row.sh refuses a merge and
# push_3ds.sh refuses a push while it exists. rc 0 of a job with cold 1 and shots 1 on a commit that descends from the FAIL commit
# (not the same commit) removes the file.
# The script body is functions and the last line calls them: a merge that rewrites this file does not change a running runner.
EVERY=3

pm_lock() {
    if mkdir "$LK" 2>/dev/null; then echo $$ > "$LK/pid"; return 0; fi
    p=$(cat "$LK/pid" 2>/dev/null)
    old=$(find "$LK" -maxdepth 1 -name pid -mmin +180 2>/dev/null)
    if { [ -n "$p" ] && ! kill -0 "$p" 2>/dev/null; } || [ -n "$old" ]; then
        echo "postmerge: lock of pid $p removed (pid gone, or no job start for 180 min)"
        rm -rf "$LK"
        if mkdir "$LK" 2>/dev/null; then echo $$ > "$LK/pid"; return 0; fi
    fi
    return 1
}

pm_one() { # <commit> <cold> <shots>: one check in the worktree $W; prints its output, returns the rc of post_merge.sh (20: worktree)
    if [ -f "$W/.git" ]; then
        git -C "$W" checkout -q -f --detach "$1" && git -C "$W" clean -fdq || return 20
    else
        rm -rf "$W"; git worktree prune
        git worktree add -q --detach "$W" "$1" || return 20
    fi
    touch "$W/.wt_keep" # wt_gc.sh keeps a detached worktree with .wt_keep (wt_gc.sh row_done)
    mkdir -p "$W/build3ds/out"
    R="$W/3ds/logs/perf/runs.jsonl"
    n0=0; [ -f "$R" ] && n0=$(wc -l < "$R" | tr -d ' ')
    ( cd "$W" && sh "${POST_MERGE:-3ds/tools/post_merge.sh}" "$1" "$2" "$3" ); rc=$?
    [ -f "$R" ] && tail -n +"$((n0 + 1))" "$R" >> build3ds/out/postmerge_runs.jsonl
    return $rc
}

pm_drain() {
    while j=$(ls "$Q"/*.job 2>/dev/null | sort | head -n 1); [ -n "$j" ]; do
        touch "$LK/pid"
        c=$(cut -d' ' -f1 "$j"); full=$(cut -s -d' ' -f2 "$j")
        s=$(git rev-parse --short "$c")
        n=$(git rev-list --merges --first-parent --count "$c")
        cold=0; shots=0
        [ $((n % EVERY)) -eq 0 ] && cold=1
        [ $((n % EVERY)) -eq 1 ] && shots=1
        if [ -n "$full" ] || [ -f "$FAIL" ]; then cold=1; shots=1; fi
        o=build3ds/out/post_merge_$s.txt
        echo "postmerge: $s start $(date '+%F %T') cold $cold shots $shots"
        pm_one "$c" $cold $shots > "$o" 2>&1; rc=$?
        if [ "$rc" -ne 0 ]; then
            printf 'commit %s\nstep %s\noutput %s\ntime %s\nFix on main, then: sh 3ds/tools/postmerge_bg.sh --queue. A full pass on a newer main commit removes this file.\n' \
                "$c" "$rc" "$o" "$(date '+%F %T')" > "$FAIL.tmp" && mv -f "$FAIL.tmp" "$FAIL"
        elif [ -f "$FAIL" ] && [ "$cold$shots" = 11 ]; then
            fc=$(sed -n 's/^commit //p' "$FAIL")
            if [ "$fc" != "$c" ] && git merge-base --is-ancestor "$fc" "$c" 2>/dev/null; then
                rm -f "$FAIL"; echo "postmerge: FAIL of $fc removed: full pass on $c" | tee -a "$o"
            fi
        fi
        echo "post_merge $c rc $rc cold $cold shots $shots" > "build3ds/out/post_merge_$s.done"
        echo "postmerge: $s end $(date '+%F %T') rc $rc"
        rm -f "$j"
    done
}

pm_run() {
    while :; do
        pm_lock || { echo "postmerge: runner $$: lock held, its holder checks the queue"; return 0; }
        trap 'rm -rf "$LK"; exit 1' INT TERM HUP
        echo "postmerge: runner $$ start $(date '+%F %T')"
        pm_drain
        rm -rf "$LK"; trap - INT TERM HUP
        # a job queued while this runner held the lock (its own runner found the lock held): take the lock again
        [ -n "$(ls "$Q"/*.job 2>/dev/null)" ] || { echo "postmerge: runner $$ end $(date '+%F %T')"; return 0; }
    done
}

pm_queue() { # [full] <commit>
    c=$(git rev-parse --verify -q "$2^{commit}") || { echo "postmerge: unknown commit $2" >&2; return 2; }
    mkdir -p "$Q"
    printf '%s %s\n' "$c" "$1" > "$Q/.tmp.$$" && mv -f "$Q/.tmp.$$" "$Q/$(date +%s%N).job" || return 2
    rm -f "build3ds/out/post_merge_$(git rev-parse --short "$c").done"
    nohup sh "$SELF" --run >> build3ds/out/postmerge_runner.txt 2>&1 < /dev/null &
    echo "postmerge: queued $(git rev-parse --short "$c"), $(ls "$Q"/*.job 2>/dev/null | wc -l | tr -d ' ') in the queue; log build3ds/out/postmerge_runner.txt"
}

pm_status() {
    echo "queue: $(ls "$Q"/*.job 2>/dev/null | wc -l | tr -d ' ') job(s)"
    for j in $(ls "$Q"/*.job 2>/dev/null | sort); do echo "  $(cat "$j")"; done
    if [ -d "$LK" ]; then echo "runner: pid $(cat "$LK/pid" 2>/dev/null)"; else echo "runner: none"; fi
    if [ -f "$FAIL" ]; then echo "POST-MERGE CHECK FAILED:"; sed 's/^/  /' "$FAIL"; else echo "postmerge_FAIL.txt: none"; fi
}

pm_main() {
    SELF=$(cd "$(dirname "$0")" && pwd)/$(basename "$0")
    MAIN=$(cd "$(git rev-parse --git-common-dir)" && cd .. && pwd) || return 2
    cd "$MAIN" || return 2
    mkdir -p build3ds/out
    Q=build3ds/out/postmerge_q; LK=build3ds/out/postmerge.lock; FAIL=build3ds/out/postmerge_FAIL.txt; W=build3ds/wt/postmerge
    case "${1:-}" in
        --queue) f=""; if [ "${2:-}" = --full ]; then f=full; shift; fi; pm_queue "$f" "${2:-HEAD}" ;;
        --run) pm_run ;;
        --status) pm_status ;;
        *) echo "usage: sh 3ds/tools/postmerge_bg.sh --queue [--full] [<commit>] | --run | --status" >&2; return 2 ;;
    esac
}
pm_main "$@"; exit $?
