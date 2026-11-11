#!/bin/sh
# BUILDLOCK: at most 3 Docker builds at once on this host. A slot is the directory build3ds/buildslots/slot<k>.d in the main
# folder (mkdir is atomic). Its file pid holds: "<pid> <unix time> <lane> <Windows pid>". Sourced by 3ds/build.sh:
#   build_slot_take     waits for a free slot, sets BUILD_SLOT (the number) and BUILD_SLOT_DIR
#   build_slot_release  removes the slot this shell took
# BUILDSLOT_DIR overrides the folder (the host test 3ds/tests/buildlock/run.sh uses it).
BUILD_SLOTS_N=3

build_slot_dir() {
    if [ -n "$BUILDSLOT_DIR" ]; then echo "$BUILDSLOT_DIR"; return; fi
    MAIN=$(cd "$(git rev-parse --git-common-dir)/.." && pwd)
    echo "$MAIN/build3ds/buildslots"
}

# $1 pid, $2 Windows pid (may be empty). 0 when the holder is alive. Git Bash: kill -0 can miss a Windows process, tasklist then.
build_slot_alive() {
    kill -0 "$1" 2>/dev/null && return 0
    [ -n "$2" ] && tasklist //FI "PID eq $2" 2>/dev/null | grep -q "[[:space:]]$2[[:space:]]"
}

# $1 slot dir, $2 slot number. Removes the dir when its holder is dead. 0 when removed, 1 when kept.
build_slot_reap() {
    s=$1; k=$2
    [ -d "$s" ] || return 1
    p=$(cut -d' ' -f1 "$s/pid" 2>/dev/null)
    if [ -n "$p" ]; then
        w=$(cut -d' ' -f4 "$s/pid" 2>/dev/null)
        build_slot_alive "$p" "$w" && return 1
    else
        # no holder file yet: a take is writing it. Dead only when the dir is 30 s old without one.
        now=$(date +%s); m=$(date -r "$s" +%s 2>/dev/null || echo "$now")
        [ $((now - m)) -lt 30 ] && return 1
        p=none
    fi
    # mv is atomic: of two takers that see the same dead slot, one moves it, the other fails and goes on.
    mv "$s" "$s.dead.$$" 2>/dev/null || return 1
    rm -rf "$s.dead.$$"
    echo "buildslot: slot $k of dead pid $p removed"
    return 0
}

build_slot_take() {
    dir=$(build_slot_dir)
    mkdir -p "$dir"
    lane=${PM_LANE:-main}
    t0=$(date +%s); last=0
    while :; do
        k=1
        while [ "$k" -le "$BUILD_SLOTS_N" ]; do
            s="$dir/slot$k.d"
            if mkdir "$s" 2>/dev/null || { build_slot_reap "$s" "$k" && mkdir "$s" 2>/dev/null; }; then
                printf '%s %s %s %s\n' "$$" "$(date +%s)" "$lane" "$(cat /proc/$$/winpid 2>/dev/null)" > "$s/pid"
                BUILD_SLOT=$k; BUILD_SLOT_DIR=$s; BUILD_SLOT_SHELL=${BASHPID:-$$}
                export BUILD_SLOT
                echo "buildslot: slot $k taken"
                return 0
            fi
            k=$((k + 1))
        done
        n=$(( $(date +%s) - t0 ))
        if [ $((n - last)) -ge 60 ]; then
            lanes=""
            for f in "$dir"/slot*.d/pid; do [ -f "$f" ] && lanes="$lanes $(cut -d' ' -f3 "$f")"; done
            echo "buildslot: waiting ($BUILD_SLOTS_N builds running:$lanes) $n s"
            last=$n
        fi
        sleep 2
    done
}

build_slot_release() {
    [ -n "$BUILD_SLOT_DIR" ] || return 0
    # Only the shell that took the slot removes it: a subshell inherits the EXIT trap.
    if [ "${BASHPID:-$$}" = "$BUILD_SLOT_SHELL" ] && [ "$(cut -d' ' -f1 "$BUILD_SLOT_DIR/pid" 2>/dev/null)" = "$$" ]; then
        rm -rf "$BUILD_SLOT_DIR"
    fi
    BUILD_SLOT_DIR=
    return 0
}
