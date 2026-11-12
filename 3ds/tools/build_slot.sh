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

# DOCKWD: docker engine health. build_docker_ok: 0 when docker info answers within 20 s. build_docker_restart: restarts
# Docker Desktop only when no build holds a slot (a live slot would lose its container), then waits up to 180 s for the
# engine. DOCKER_RESTART_CMD replaces the restart command (the host test uses a stub). 0 when the engine answers, 1 when not.
build_docker_ok() {
    timeout 20 docker info >/dev/null 2>&1
}

build_docker_restart() {
    dir=$(build_slot_dir)
    live=0
    for s in "$dir"/slot*.d; do
        [ -d "$s" ] || continue
        build_slot_reap "$s" 0 || live=$((live + 1))
    done
    if [ "$live" -gt 0 ]; then
        echo "docker restart skipped: $live builds running" >&2
        return 1
    fi
    echo "docker restart: engine not answering, restarting Docker Desktop" >&2
    if [ -n "$DOCKER_RESTART_CMD" ]; then
        sh -c "$DOCKER_RESTART_CMD"
    else
        powershell.exe -NoProfile -Command "Stop-Process -Name 'Docker Desktop' -Force -ErrorAction SilentlyContinue; Start-Process 'C:\Program Files\Docker\Docker\Docker Desktop.exe'"
    fi
    t=0
    while :; do
        if build_docker_ok; then
            echo "docker restart: engine answers after $t s" >&2
            return 0
        fi
        [ "$t" -ge 180 ] && { echo "docker restart: engine not answering after 180 s" >&2; return 1; }
        echo "docker restart: try at $t s, engine not answering" >&2
        sleep 5
        t=$((t + 5))
    done
}
