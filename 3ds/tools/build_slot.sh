#!/bin/sh
# BUILDLOCK: at most 4 Docker builds (PM_BUILD_SLOTS) at once on this host. A slot is the directory build3ds/buildslots/slot<k>.d in the main
# folder (mkdir is atomic). Its file pid holds: "<pid> <unix time> <lane> <Windows pid>". Sourced by 3ds/build.sh:
#   build_slot_take     waits for a free slot, sets BUILD_SLOT (the number) and BUILD_SLOT_DIR
#   build_slot_release  removes the slot this shell took
# BUILDSLOT_DIR overrides the folder (the host test 3ds/tests/buildlock/run.sh uses it).
BUILD_SLOTS_N=${PM_BUILD_SLOTS:-4} # 4 slots: peak 2.89 GiB per build, 8.62 GiB at 3 builds (memlog 2026-11-16), WSL cap 14 GB

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
# SLOTFIX: the dir is judged by the pid line read once (pl). A dir without a pid line (an older take that writes the pid after
# its mkdir) is dead only when it is 30 s old. The dir is moved away, then the moved dir must still hold the judged line: two
# reapers can judge the same dead slot, and between the judgement and the mv another taker can free and retake the slot.
# On a different line the slot is live: it is moved back (FACT post_merge 6600a1eedd: 'slot4.d/pid: No such file').
build_slot_reap() {
    s=$1; k=$2
    [ -d "$s" ] || return 1
    pl=$(cat "$s/pid" 2>/dev/null); p=${pl%% *}
    if [ -n "$p" ]; then
        w=$(printf '%s\n' "$pl" | cut -d' ' -f4)
        build_slot_alive "$p" "$w" && return 1
    else
        now=$(date +%s); m=$(date -r "$s" +%s 2>/dev/null || echo "$now")
        [ $((now - m)) -lt 30 ] && return 1
        p=none
    fi
    g="$s.dead.$$"
    mv "$s" "$g" 2>/dev/null || return 1
    if [ "$(cat "$g/pid" 2>/dev/null)" != "$pl" ]; then
        mv -T "$g" "$s" 2>/dev/null || rm -rf "$g" # the slot was retaken meanwhile: give it back (taken again: its holder ends)
        return 1
    fi
    rm -rf "$g"
    echo "buildslot: slot $k of dead pid $p removed"
    return 0
}

# SLOTFIX: a slot dir appears with its pid file. The take writes the pid into a private dir (.take.<pid>) and renames it to
# slot<k>.d in one step; the rename fails while slot<k>.d exists (a held slot holds its pid file). So no reaper sees a fresh
# slot without a pid, and the pid write never goes to a dir that a reaper moved away.
build_slot_newtk() {
    rm -rf "$BUILD_SLOT_TK"; mkdir "$BUILD_SLOT_TK" 2>/dev/null && printf '%s %s %s %s\n' "$$" "$(date +%s)" "$lane" "$(cat /proc/$$/winpid 2>/dev/null)" > "$BUILD_SLOT_TK/pid"
}
build_slot_take() {
    dir=$(build_slot_dir)
    mkdir -p "$dir"
    lane=${PM_LANE:-main}
    t0=$(date +%s); last=0
    BUILD_SLOT_TK="$dir/.take.$$"
    build_slot_newtk || { echo "buildslot: cannot write $BUILD_SLOT_TK" >&2; return 1; }
    while :; do
        k=1
        while [ "$k" -le "$BUILD_SLOTS_N" ]; do
            s="$dir/slot$k.d"
            if [ -e "$s" ] && ! build_slot_reap "$s" "$k"; then k=$((k + 1)); continue; fi
            if [ ! -e "$s" ] && mv -T "$BUILD_SLOT_TK" "$s" 2>/dev/null; then
                if [ "$(cut -d' ' -f1 "$s/pid" 2>/dev/null)" = "$$" ]; then
                    BUILD_SLOT=$k; BUILD_SLOT_DIR=$s; BUILD_SLOT_SHELL=${BASHPID:-$$}
                    export BUILD_SLOT
                    echo "buildslot: slot $k taken"
                    return 0
                fi
                build_slot_newtk || { echo "buildslot: cannot write $BUILD_SLOT_TK" >&2; return 1; } # an older taker wrote over it
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
    [ -n "$BUILD_SLOT_TK" ] && [ "${BASHPID:-$$}" = "$$" ] && rm -rf "$BUILD_SLOT_TK" # a take that never got a slot
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
