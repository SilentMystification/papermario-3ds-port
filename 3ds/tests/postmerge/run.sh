#!/bin/sh
# Host test for 3ds/tools/postmerge_bg.sh (POSTASYNC): four cases in a temp git repo, a post_merge.sh stub (POST_MERGE).
#   sh 3ds/tests/postmerge/run.sh      Prints "postmerge: N/4 PASS"; exit 0 only when all pass.
# Seeded faults: (b) a pass on the FAIL commit itself must keep the file; (c) a lock of a dead pid must not block the queue.
set -u
ROOT=$(git rev-parse --show-toplevel) || exit 2
BG="$ROOT/3ds/tools/postmerge_bg.sh"
T=$(mktemp -d) || exit 2
trap 'rm -rf "$T"' EXIT
cat > "$T/stub.sh" <<'STUB'
#!/bin/sh
# post_merge.sh stub <commit> <cold> <shots>: logs start and end to PM_STUB_LOG; PM_STUB_SLEEP seconds
echo "start $1 $2$3" >> "$PM_STUB_LOG"
sleep "${PM_STUB_SLEEP:-0}"
echo "end $1" >> "$PM_STUB_LOG"
exit 0
STUB
export POST_MERGE="$T/stub.sh" PM_STUB_LOG="$T/log.txt"
cd "$T" && mkdir r && cd r || exit 2
git init -q -b main && git config user.email t@t && git config user.name t && git config core.autocrlf false
printf 'build3ds/\n' > .gitignore; echo 1 > f.txt; git add -A; git commit -q -m c1; c1=$(git rev-parse HEAD)
echo 2 > f.txt; git commit -q -am c2; c2=$(git rev-parse HEAD)
echo 3 > f.txt; git commit -q -am c3; c3=$(git rev-parse HEAD)
pass=0; n=0
result() { n=$((n+1)); if [ "$1" = yes ]; then pass=$((pass+1)); echo "  PASS $2"; else echo "  FAIL $2"; cat "$PM_STUB_LOG" 2>/dev/null | sed 's/^/      /'; fi; }
done_wait() { i=0; while [ ! -f "build3ds/out/post_merge_$(git rev-parse --short "$1").done" ] && [ $i -lt 60 ]; do sleep 1; i=$((i+1)); done; }

# (a) two commits queued at once: one runner at a time, oldest first, each in build3ds/wt/postmerge
: > "$PM_STUB_LOG"
PM_STUB_SLEEP=2 sh "$BG" --queue "$c1" > /dev/null; PM_STUB_SLEEP=2 sh "$BG" --queue "$c2" > /dev/null
done_wait "$c1"; done_wait "$c2"
p=no
if [ "$(cut -d' ' -f1,2 "$PM_STUB_LOG" | tr '\n' ' ')" = "start $c1 end $c1 start $c2 end $c2 " ] && [ -f build3ds/wt/postmerge/.wt_keep ] \
   && [ "$(git -C build3ds/wt/postmerge rev-parse HEAD)" = "$c2" ] && [ ! -d build3ds/out/postmerge.lock ]; then p=yes; fi
result $p "(a) queue of 2: one at a time, in order"

# (b) FAIL clear rule: a pass on the FAIL commit keeps the file (seeded fault); a full pass on a newer commit removes it
printf 'commit %s\nstep 3\n' "$c2" > build3ds/out/postmerge_FAIL.txt
sh "$BG" --queue "$c2" > /dev/null; done_wait "$c2"; kept=no; [ -f build3ds/out/postmerge_FAIL.txt ] && kept=yes
sh "$BG" --queue "$c3" > /dev/null; done_wait "$c3"
p=no
if [ "$kept" = yes ] && [ ! -f build3ds/out/postmerge_FAIL.txt ] && grep -q "cold 1 shots 1" "build3ds/out/post_merge_$(git rev-parse --short "$c3").done"; then p=yes; fi
result $p "(b) FAIL kept on the same commit ($kept), removed by a full pass on a newer one"

# (c) a lock of a dead pid (seeded fault) does not block the queue
mkdir -p build3ds/out/postmerge.lock; echo 999999 > build3ds/out/postmerge.lock/pid
sh "$BG" --queue "$c1" > /dev/null; done_wait "$c1"
p=no; grep -q "^post_merge $c1 rc 0" "build3ds/out/post_merge_$(git rev-parse --short "$c1").done" 2>/dev/null && p=yes
result $p "(c) lock of a dead pid removed, job ran"

# (d) the commit text of a job is the commit checked; --status prints no FAIL and an empty queue
p=no
if sh "$BG" --status | grep -q "queue: 0 job" && sh "$BG" --status | grep -q "postmerge_FAIL.txt: none"; then p=yes; fi
result $p "(d) status: empty queue, no FAIL"

echo "postmerge: $pass/$n PASS"
[ "$pass" = "$n" ]
