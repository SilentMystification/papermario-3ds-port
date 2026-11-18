#!/bin/sh
# GENKEY: read-set keys for a traced step (a generator of 3ds/build.sh, the gfx table of nd_host_test.sh). Sourced after
# src_sync.sh (in_manifest) and gencache.sh (gc_get, gc_put, GC_EXCL). Cache safety (docs/handoff/plan_buildspeed.md):
# - the step runs in the host image on a tar of exactly its path list (rk_paths) and logs every file it opens below /work
#   (3ds/tools/readtrace.c, LD_PRELOAD, RK_TRACE below);
# - K0 = the step text, the image id, the assets, the trace tool, and the sha1 of the full path list of the tar: a new or a
#   removed file anywhere in the tar is a new K0 (this covers every failed include probe and every directory scan);
# - the full key = K0 plus "path id" of every traced read (in_manifest content id; a generator output of GC_EXCL is hashed
#   from the disk); an entry stores its read list (reads.txt) with the outputs, all under out.sha1. A lookup recomputes the
#   full key of each stored read list of K0 and needs key.txt and every file hash to match (gc_get). A changed read misses.
# Self-tests: 3ds/tests/host.d/readtrace.sh (the trace), 3ds/tests/host.d/genkey.sh (key: a read file misses, an unread hits).

# RK_SO: container text that builds the trace shim for 32 and 64 bit programs (cwd /work, 3ds/tools/readtrace.c in the tar).
RK_SO='mkdir -p /tmp/rt; for p in x86_64 i386 i486 i586 i686; do mkdir -p /tmp/rt/$p; done; gcc -m64 -O2 -shared -fPIC -o /tmp/rt/x86_64/rt.so 3ds/tools/readtrace.c -ldl; gcc -m32 -O2 -shared -fPIC -o /tmp/rt/i386/rt.so 3ds/tools/readtrace.c -ldl; for p in i486 i586 i686; do cp /tmp/rt/i386/rt.so /tmp/rt/$p/rt.so; done; rm -f /tmp/rt/log'
# RK_TRACE: the prefix of a traced command (log /tmp/rt/log: "<exe><TAB><path below /work>").
RK_TRACE='RT_LOG=/tmp/rt/log RT_ROOT=/work/ LD_PRELOAD=/tmp/rt/\$PLATFORM/rt.so'
# RKID: the trace tool is part of every K0 (it is compiled before the trace starts, so no trace names it).
rk_id() { cat 3ds/tools/readtrace.c 3ds/tools/readkey.sh | sha1sum | cut -c1-40; }

# rk_pathsum <paths file>: the K0 line of the path list. A new, a removed, or a created (once probed and missing) file under the
# step's tar paths changes it: this covers every directory listing, glob and failed open of the step (the container sees only
# the tar below /work). The command line and the environment of the step are in K0 as the command text (cmd) and the image id.
rk_pathsum() { sha1sum < "$1" | cut -c1-40; }

# rk_paths <idfile> <path>...: the tar list of a step: the files of <idfile> (in_manifest lines) under the paths, plus the
# generator outputs of GC_EXCL under the paths that exist on the disk now (their idfile lines can be older).
rk_paths() {
    rf=$1; shift
    { awk -F'\t' -v L="$*" 'BEGIN { n = split(L, a, " ") } { for (i = 1; i <= n; i++) if ($1 == a[i] || index($1, a[i] "/") == 1) { print $1; next } }' "$rf"
      for x in $GC_EXCL; do for p in "$@"; do case $x in "$p"|"$p"/*) [ -f "$x" ] && echo "$x" ;; esac; done; done; } | LC_ALL=C sort -u
}

# rk_ids <idfile> <reads file>: "path<TAB>id" of each read: the idfile id, else (GC_EXCL or not listed) d:<sha1 of the disk
# file>, or "-" when the file is gone.
rk_ids() {
    # BUILDO: awk joins the lists (a shell read loop cost about 1 s per 1000 reads in Git Bash); only the "?" paths (few) are hashed.
    rq=${OUT:-.}/.rkq.$$
    awk -F'	' -v X="$GC_EXCL" 'BEGIN { m = split(X, x, " "); for (i = 1; i <= m; i++) ex[x[i]] = 1 }
        FILENAME == ARGV[1] { if (!($1 in ex)) id[$1] = $2; next } !($1 in id) { print $1 }' "$1" "$2" > $rq.q
    : > $rq.u
    while IFS= read -r p; do
        if [ -f "$p" ]; then i="d:$(sha1sum < "$p" | cut -c1-40)"; else i=-; fi
        printf '%s\t%s\n' "$p" "$i" >> $rq.u
    done < $rq.q
    awk -F'	' -v X="$GC_EXCL" 'BEGIN { m = split(X, x, " "); for (i = 1; i <= m; i++) ex[x[i]] = 1 }
        FILENAME == ARGV[1] { u[$1] = $2; next } FILENAME == ARGV[2] { if (!($1 in ex)) id[$1] = $2; next }
        { print $1 "	" (($1 in id) ? id[$1] : u[$1]) }' $rq.u "$1" "$2"
    rm -f $rq.q $rq.u
}

# rk_full <K0> <idfile> <reads file>: the full key text.
rk_full() { printf '%s\nreads\n%s' "$1" "$(rk_ids "$2" "$3")"; }

# rk_get <gen> <K0> <idfile> <name:path>...: a hit (0) copies the outputs of the entry whose read list gives a full key that
# matches its key.txt and out.sha1 (gc_get). RK_DIR is the K0 folder (gen/<16 hex>).
rk_get() {
    rg=$1; rk0=$2; rid=$3; shift 3
    RK_DIR="$rg/$(printf '%s' "$rk0" | sha1sum | cut -c1-16)"
    case "${PM_CACHE:-on}" in off|check) return 1 ;; esac
    for c in "$GC_ROOT/$RK_DIR"/*/; do
        case $c in *.tmp.*) continue ;; esac
        [ -f "$c/reads.txt" ] || continue
        gc_get "$RK_DIR" "$(rk_full "$rk0" "$rid" "$c/reads.txt")" "$@" && return 0
    done
    return 1
}

# rk_reads <trace log> <paths file> <out reads file> <must>...: the sorted reads that are in the tar list (a file the step
# made itself is no input). Each <must> path must be in the reads, else the trace is broken: exit 1, nothing is stored.
rk_reads() {
    cut -f2 "$1" | LC_ALL=C sort -u | LC_ALL=C comm -12 - "$2" > "$3"
    l=$1; ro=$3; shift 3
    for m in "$@"; do grep -qxF "$m" "$ro" || { echo "GENKEY FAULT: the read trace $l lacks $m (readtrace.c). Nothing stored." >&2; exit 1; }; done
}
