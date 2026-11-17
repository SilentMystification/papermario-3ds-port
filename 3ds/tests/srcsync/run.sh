#!/bin/sh
# VOLGEN: host test of in_manifest (3ds/tools/src_sync.sh) in Git Bash (it needs git; the host image has none). Run from the
# repo root: sh 3ds/tests/srcsync/run.sh. Seeded faults: one changed byte of a tracked file, an untracked file and an ignored
# file must change their id; an unchanged file keeps its b: id. Prints "srcsync ok", exit 1 on a failure.
R=$(pwd); T=$R/build3ds/out/srcsync_t; rm -rf "$T"; mkdir -p "$T/out"
TAB=$(printf '	')
fail() { echo "srcsync FAIL $1"; rm -rf "$T"; exit 1; }
cd "$T" && git init -q r && cd r || fail "git init"
git config user.email t@t; git config user.name t
mkdir -p d; printf 'a\n' > d/a.c; printf 'b\n' > d/b.c; printf 'ign\n' > .gitignore; git add d .gitignore; git commit -q -m t
OUT=$T/out; . "$R/3ds/tools/src_sync.sh"
in_manifest d > $T/m0.txt
grep -q "^d/a.c${TAB}b:" $T/m0.txt || fail "a clean tracked file has no b: id"
printf 'a2\n' > d/a.c; in_manifest d > $T/m1.txt
[ "$(grep "^d/a.c${TAB}" $T/m0.txt)" != "$(grep "^d/a.c${TAB}" $T/m1.txt)" ] || fail "one changed byte kept the id"
grep -q "^d/a.c${TAB}h:" $T/m1.txt || fail "a modified file has no h: id"
[ "$(grep "^d/b.c${TAB}" $T/m0.txt)" = "$(grep "^d/b.c${TAB}" $T/m1.txt)" ] || fail "an unchanged file changed its id"
printf 'u\n' > d/u.c; printf 'i\n' > d/ign; in_manifest d > $T/m2.txt
grep -q "^d/u.c${TAB}h:" $T/m2.txt || fail "an untracked file is missing"
grep -q "^d/ign${TAB}h:" $T/m2.txt || fail "an ignored file is missing"
rm d/b.c; in_manifest d > $T/m3.txt
grep -q "^d/b.c${TAB}" $T/m3.txt && fail "a deleted file is listed"
git checkout -q -- d/a.c d/b.c; in_manifest d > $T/m5.txt
PM_BUILD_REV=HEAD in_manifest d > $T/m4.txt
[ "$(grep "^d/a.c${TAB}" $T/m4.txt)" = "$(grep "^d/a.c${TAB}" $T/m5.txt)" ] || fail "PM_BUILD_REV: a tracked file has another id than in a worktree build"
grep -q "^d/u.c${TAB}h:" $T/m4.txt || fail "PM_BUILD_REV: an untracked file has no h: id"
cd "$R"; rm -rf "$T"
echo "srcsync ok"
