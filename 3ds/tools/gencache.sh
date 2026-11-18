#!/bin/sh
# GENCACHE (VOLGEN): the shared store of the generator outputs of 3ds/build.sh, one store for every worktree and lane:
# <main folder>/build3ds/gencache/<gen>/<16 hex of sha1(key)>/. Cache safety (docs/handoff/plan_buildspeed.md):
# - the key is a text of content hashes of every input (tool and source files, ROM, asset files, image id, command text);
# - an entry holds key.txt (the full key), out.sha1 (the sha1 of each stored file) and the files; every use checks both,
#   and a failed entry is removed (a miss);
# - a new entry is a temp folder that one mv renames: no half entry.
# PM_CACHE=off: no read, no write. PM_CACHE=check (cachecheck): every get misses; every put compares the new files with the
# entry of the same key and prints "cachecheck <gen> ok" or "CACHECHECK FAIL <gen>" (the entry is then replaced, GC_FAIL=1).
# Needs MAIN_ROOT (or GENCACHE_DIR) and OUT. Host self-test: 3ds/tests/host.d/volgen.sh.
GC_ROOT=${GENCACHE_DIR:-$MAIN_ROOT/build3ds/gencache}
GC_FAIL=0
# Files under the key paths that are no input of a generator: pm_build.h is written after the generators and read only by
# 3ds/src/pm_main.c, pm_options.c and pm_wdog.c; the others are generator outputs (gen_pso_tab adds the 2 gfx outputs itself).
GC_EXCL="3ds/include/pm_build.h 3ds/src/nd_pso_tab.c 3ds/src/nd_imgfx_tab.c 3ds/src/nd_gfx_tab.c 3ds/src/nd_ntab_descs.c"

gc_dir() { printf '%s/%s/%s' "$GC_ROOT" "$1" "$(printf '%s' "$2" | sha1sum | cut -c1-16)"; }

# gc_ok <entry dir> <key>: 0 when the entry holds this key and every file matches out.sha1. A failed entry is removed.
gc_ok() {
    [ -d "$1" ] || return 1
    if [ -f "$1/key.txt" ] && [ "$(cat "$1/key.txt")" = "$2" ] && [ -s "$1/out.sha1" ] && (cd "$1" && sha1sum -c --status out.sha1 2>/dev/null); then
        return 0
    fi
    echo "gencache: entry $1 failed its check (key or file hash), removed" >&2
    rm -rf "$1"
    return 1
}

# gc_inputs <path>...: the lines of $OUT/in_now.txt (src_sync.sh in_manifest) under the paths, without GC_EXCL.
gc_inputs() {
    awk -F'\t' -v L="$*" -v X="$GC_EXCL" 'BEGIN { n = split(L, a, " "); m = split(X, x, " "); for (i = 1; i <= m; i++) ex[x[i]] = 1 }
        ($1 in ex) { next } { for (i = 1; i <= n; i++) if ($1 == a[i] || index($1, a[i] "/") == 1) { print; next } }' "$OUT/in_now.txt"
}

# gc_get <gen> <key> <name:path>...: on a hit 0, and each stored file is copied to its path (an equal file is not touched).
gc_get() {
    g=$1; k=$2; shift 2
    case "${PM_CACHE:-on}" in off|check) return 1 ;; esac
    d=$(gc_dir "$g" "$k")
    gc_ok "$d" "$k" || return 1
    for p in "$@"; do [ -f "$d/${p%%:*}" ] || { rm -rf "$d"; return 1; }; done
    for p in "$@"; do
        f=${p#*:}
        mkdir -p "$(dirname "$f")"
        cmp -s "$d/${p%%:*}" "$f" || cp -f "$d/${p%%:*}" "$f"
    done
    return 0
}

# gc_check <gen> <entry dir> <0|1 equal>: the cachecheck line of one compare.
gc_check() {
    if [ "$3" = 1 ]; then echo "cachecheck $1 ok" >&2; return 0; fi
    echo "CACHECHECK FAIL $1: the entry $2 differs from a cold run, replaced" >&2
    GC_FAIL=1
    rm -rf "$2"
}

# gc_put <gen> <key> <name:path>...: stores the files under the key. 1 when a file is missing (nothing stored).
gc_put() {
    g=$1; k=$2; shift 2
    [ "${PM_CACHE:-on}" = off ] && return 0
    d=$(gc_dir "$g" "$k")
    if [ "${PM_CACHE:-on}" = check ]; then
        if [ -d "$d" ]; then
            eq=1; gc_ok "$d" "$k" || eq=0
            if [ "$eq" = 1 ]; then for p in "$@"; do cmp -s "$d/${p%%:*}" "${p#*:}" || eq=0; done; fi
            gc_check "$g" "$d" "$eq"
        else
            echo "cachecheck $g: no entry for this key" >&2
        fi
    fi
    [ -d "$d" ] && return 0
    t="$d.tmp.$$"; rm -rf "$t"; mkdir -p "$t" || return 1
    for p in "$@"; do f=${p#*:}; [ -f "$f" ] || { rm -rf "$t"; return 1; }; cp -f "$f" "$t/${p%%:*}"; done
    printf '%s' "$k" > "$t/key.txt"
    (cd "$t" && for p in "$@"; do sha1sum "${p%%:*}"; done > out.sha1)
    mv -T "$t" "$d" 2>/dev/null || rm -rf "$t"
    gc_trim "$g"
    return 0
}

# gc_get_dir <gen> <key> <dest dir> <marker file>: on a hit 0, and <dest dir> holds hard links (else copies) of the entry files.
# The marker names the entry; a dest dir with that marker and the same file count is kept as it is.
gc_get_dir() {
    case "${PM_CACHE:-on}" in off|check) return 1 ;; esac
    d=$(gc_dir "$1" "$2")
    gc_ok "$d" "$2" || return 1
    e=$(basename "$d"); n=$(grep -c . "$d/out.sha1")
    if [ "$(cat "$4" 2>/dev/null)" = "$e" ] && [ "$(ls -A "$3" 2>/dev/null | wc -l | tr -d ' ')" = "$n" ]; then return 0; fi
    rm -rf "$3"; mkdir -p "$3"
    cp -l "$d"/f/* "$3"/ 2>/dev/null || cp -f "$d"/f/* "$3"/
    printf '%s' "$e" > "$4"
    return 0
}

# gc_put_dir <gen> <key> <src dir> <marker file>: stores every file of <src dir> under the key (hard links, else copies).
gc_put_dir() {
    d=$(gc_dir "$1" "$2")
    [ "${PM_CACHE:-on}" = off ] && return 0
    if [ "${PM_CACHE:-on}" = check ]; then
        if [ -d "$d" ]; then
            eq=1; gc_ok "$d" "$2" || eq=0
            [ "$eq" = 1 ] && { diff -r -q "$d/f" "$3" >/dev/null 2>&1 || eq=0; }
            gc_check "$1" "$d" "$eq"
        else
            echo "cachecheck $1: no entry for this key" >&2
        fi
    fi
    if [ ! -d "$d" ]; then
        t="$d.tmp.$$"; rm -rf "$t"; mkdir -p "$t/f" || return 1
        cp -l "$3"/* "$t/f/" 2>/dev/null || cp -f "$3"/* "$t/f/"
        printf '%s' "$2" > "$t/key.txt"
        (cd "$t" && sha1sum f/* > out.sha1)
        mv -T "$t" "$d" 2>/dev/null || rm -rf "$t"
        gc_trim "$1"
    fi
    [ -d "$d" ] && printf '%s' "$(basename "$d")" > "$4"
    return 0
}

# gc_trim <gen>: keeps the newest GENCACHE_KEEP (default 12) entries of the gen.
gc_trim() {
    ls -1dt "$GC_ROOT/$1"/*/ 2>/dev/null | grep -v '\.tmp\.' | tail -n +$((${GENCACHE_KEEP:-12} + 1)) | while read -r x; do rm -rf "$x"; done
}
