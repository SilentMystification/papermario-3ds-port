#!/bin/sh
# Push build3ds/pm_3ds.3dsx to the 3DS Homebrew Launcher netloader, then read the game's
# live log over Wi-Fi for -t seconds.
# Usage (repo root, Git Bash): sh 3ds/push_3ds.sh [-t seconds] [game args...]
#   e.g. sh 3ds/push_3ds.sh -t 90 gputest
#        sh 3ds/push_3ds.sh --attach --ip <ip> -t <s>   (no send: read the live log of a game already running)
# A send that fails after the console answered still reads the live log (the game may have started).
# A live log that ends early (no data for a while, cap not reached) is read again with livelog.py --append, 3 times at most.
# After a send failure the log's boot line "build N git H" must equal the sent build (PM_SENT_BUILD, PM_SENT_GIT, set by
# bench.py build-at; else 3ds/include/pm_build.h). Otherwise the log is from another binary: STALE BINARY, no ingest hint.
# "--check-log <file>" runs only that check (tests).
# Every push first archives build3ds/pm_3ds.3dsx to 3ds/logs/builds/pm_3ds_<build>_<hash>.3dsx and appends
# a line to 3ds/logs/perf/pushed_builds.txt. The archive is best effort: a failed archive prints one
# ARCHIVE WARNING line and the push goes on. "--archive-only" does the archive step and stops (no push), and fails on error.
# The 3DS address comes from $N3DS_IP, else from a UDP broadcast (the same probe 3dslink uses).
# The game runs a log server on TCP port 17492 when netloaded (3ds/src/pm_main.c). This PC
# connects to it, the same direction as the upload, so a PC firewall or VPN does not block it.
# Before the send the console IP gets one ping (informational). After a NO LOG, livelog.py --probe makes one TCP connect
# to 17492 and prints its class (refused, timeout, unreachable, or connected).
# Output: build3ds/hw_live_log_<last octet of the IP>.txt (also printed), one file per console, so two
# consoles can be captured at the same time. Run it in the background for long runs.
set -e
cd "$(dirname "$0")/.."
secs=0
ipopt=""
nobuild=0
archonly=0
attach=0
checklog=""
while :; do
    case "$1" in
        -t) secs=$2; shift 2 ;;
        --ip) ipopt=$2; shift 2 ;;
        --no-build) nobuild=1; shift ;;
        --archive-only) archonly=1; shift ;;
        --attach) attach=1; shift ;;
        --check-log) checklog=$2; shift 2 ;;
        *) break ;;
    esac
done

# The 3dsx this push sends: build3ds/pm_3ds.3dsx, or PM_SRC3DSX (bench.py build-at sends a lane build from its scratch folder).
SRC3DSX=${PM_SRC3DSX:-build3ds/pm_3ds.3dsx}
# Archive the 3dsx this push sends: 3ds/logs/builds/pm_3ds_<build>_<git hash>.3dsx (build = the number
# before the first "-" in PM_BUILD_STR, the boot line's build number). An existing archive with the same
# name must have the same sha256, or the archive is skipped (ARCHIVE WARNING). One line goes to 3ds/logs/perf/pushed_builds.txt.
# PM_ARCHIVE_DIR and PM_PUSH_LOG override the paths (tests only).
archive_build() {
    src=$SRC3DSX
    adir=${PM_ARCHIVE_DIR:-3ds/logs/builds}
    plog=${PM_PUSH_LOG:-3ds/logs/perf/pushed_builds.txt}
    [ -f "$src" ] || { echo "ARCHIVE WARNING: no $src"; return 1; }
    bstr=$(sed -n 's/^#define PM_BUILD_STR "\(.*\)"/\1/p' 3ds/include/pm_build.h)
    hash=$(sed -n 's/^#define PM_GIT_HASH "\(.*\)"/\1/p' 3ds/include/pm_build.h)
    build=${bstr%%-*}
    case "$build" in ''|*[!0-9]*) echo "ARCHIVE WARNING: no build number in 3ds/include/pm_build.h ($bstr)"; return 1 ;; esac
    [ -n "$hash" ] || { echo "ARCHIVE WARNING: no git hash in 3ds/include/pm_build.h"; return 1; }
    name="pm_3ds_${build}_${hash}.3dsx"
    dst="$adir/$name"
    sha=$(sha256sum "$src" | cut -d' ' -f1)
    if [ -f "$dst" ]; then
        dsha=$(sha256sum "$dst" | cut -d' ' -f1)
        [ "$dsha" = "$sha" ] || { echo "ARCHIVE WARNING: $dst exists with another sha256, not overwritten"; return 1; }
    else
        cp -f "$src" "$dst"
        csha=$(sha256sum "$dst" | cut -d' ' -f1)
        [ "$csha" = "$sha" ] || { echo "ARCHIVE WARNING: copy $dst sha256 differs from $src"; return 1; }
    fi
    bytes=$(wc -c <"$dst" | tr -d ' ')
    printf '%s  %s  %s bytes  sha256 %s  build %s  git %s  flags: %s\n' "$(date '+%Y-%m-%d %H:%M')" "$name" "$bytes" "$sha" "$build" "$hash" "$1" >>"$plog"
    echo "ARCHIVED $dst (sha256 $sha)"
}

# Build only when build3ds/pm_3ds.3dsx is older than its inputs (same input list as Invoke-Build in
# 3ds/go.ps1) or the build number in pm_build.h differs from the one build.sh would write now.
# This script never stashes or changes user files.
if [ "$nobuild" -eq 0 ] && [ "$attach" -eq 0 ] && [ -z "$checklog" ]; then
    OUT=build3ds; [ -n "$PM_LANE" ] && OUT=build3ds/lane-$PM_LANE
    read V0 C0 <<EOF2
$(tr -d '\015' < 3ds/build_base.txt)
EOF2
    BN=$((V0 + $(git rev-list --count HEAD) - C0))
    if [ -n "$PM_LANE" ] || [ "$(git branch --show-current)" = "lane-b" ]; then BN="$BN-laneB"; fi
    stale=0
    if [ ! -f "$OUT/pm_3ds.3dsx" ] || [ ! -f 3ds/include/pm_build.h ]; then stale=1
    elif ! grep -q "\"$BN\"" 3ds/include/pm_build.h; then stale=1
    elif [ -n "$(find 3ds/src 3ds/include 3ds/shaders src include 3ds/CMakeLists.txt 3ds/build_base.txt -type f -newer "$OUT/pm_3ds.3dsx" -not -name pm_build.h 2>/dev/null | head -1)" ]; then stale=1
    fi
    if [ "$stale" -eq 1 ]; then echo "--- building (3dsx older than inputs) ---"; sh 3ds/build.sh
    else echo "--- build up to date, skipping ---"; fi
fi
if [ "$archonly" -eq 1 ]; then archive_build "archive-only" || exit 1; exit 0; fi
# PYTHON_EXE lets a caller (3ds/go.ps1) pass a reliably-resolved interpreter path: bash's own PATH
# (MSYS) does not always see the Windows "py" launcher even when PowerShell's PATH does.
PY=${PYTHON_EXE:-py}
if [ -z "$PYTHON_EXE" ] && ! command -v py >/dev/null 2>&1; then PY=python3; fi
# "py -3" is the Windows launcher form. python3 takes no -3.
py3() { if [ "$PY" = python3 ]; then python3 "$@"; else "$PY" -3 "$@"; fi; }

# Read the live log of $ip for $secs (0: until the game closes, at most PM_LOG_MAX). A reconnect after an early end
# (livelog.py exit 3) appends to the same file, one line each, 3 times at most. Sets $log.
read_live_log() {
    if [ "$secs" = 0 ]; then secs=${PM_LOG_MAX:-3600}; echo "reading the game's log from $ip:17492 until the game closes, at most $secs s"; else echo "reading the game's log from $ip:17492 for $secs s"; fi
    log="build3ds/hw_live_log_${ip##*.}.txt"
    t_end=$(( $(date +%s) + secs ))
    rc=0
    py3 -u 3ds/tools/livelog.py "$ip" "$secs" "$log" || rc=$?
    k=0
    while [ "$rc" -eq 3 ] && [ "$k" -lt 3 ]; do
        left=$(( t_end - $(date +%s) ))
        [ "$left" -gt 0 ] || break
        k=$((k + 1))
        echo "live log ended early (no data): reconnect $k/3 for $left s, appending to $log"
        rc=0
        py3 -u 3ds/tools/livelog.py "$ip" "$left" "$log" --append || rc=$?
    done
    echo "--- log saved to $log"
}

# boot_check <log>: 0 when the log's boot line (build N git H, lane suffix allowed) equals the sent build.
# Sets sent_b (full PM_BUILD_STR of the sent 3dsx), sent_h (git hash) and boot_log ("<build> <hash>" or none).
boot_check() {
    sent_b=${PM_SENT_BUILD:-$(sed -n 's/^#define PM_BUILD_STR "\(.*\)"/\1/p' 3ds/include/pm_build.h)}
    sent_h=${PM_SENT_GIT:-$(sed -n 's/^#define PM_GIT_HASH "\(.*\)"/\1/p' 3ds/include/pm_build.h)}
    boot_log=$(sed -n 's/.* build \([^ ]*\) git \([^ ]*\) .*/\1 \2/p' "$1" 2>/dev/null | head -1)
    [ -n "$boot_log" ] || boot_log=none
    [ -n "$sent_b" ] && [ "$boot_log" = "$sent_b $sent_h" ]
}

if [ -n "$checklog" ]; then
    if boot_check "$checklog"; then echo "MATCH: log build $boot_log = sent $sent_b $sent_h"; exit 0; fi
    echo "STALE BINARY: log build ${boot_log}, sent ${sent_b:-none}"; exit 1
fi

# $1: push | send | attach. Prints the ingest hint and the result, then exits.
report_log() {
    nlines=$(wc -l <"$log" 2>/dev/null || echo 0)
    if [ "$nlines" -eq 0 ]; then
        echo "NO LOG: no line from $ip:17492 in $secs s (game did not start or log server not reached)"
        # One TCP connect now: refused = the game's server is not up (game side), timeout/unreachable = network, connected = no line came.
        py3 -u 3ds/tools/livelog.py --probe "$ip" || true
        case "$1" in
            attach) echo "ATTACH FAILED: no live log from $ip" ;;
            send) echo "PUSH FAILED stage=send, and no live log" ;;
            *) echo "PUSH FAILED stage=no log" ;;
        esac
        exit 1
    fi
    if [ "$1" = send ]; then
        # The send failed: the live log may be of another binary (an older game still running). Ingest only when it is the sent build.
        if ! boot_check "$log"; then
            echo "STALE BINARY: log build ${boot_log}, sent ${sent_b:-none}"
            echo "PUSH FAILED stage=send, $nlines log lines, not the sent build: nothing to record"
            exit 1
        fi
        bno=${sent_b%%-*}
        echo "record: python 3ds/tools/bench.py ingest $log --bench ${PM_BENCH:-<name>} --host o3ds --build $bno --commit $sent_h"
        echo "PUSH FAILED stage=send, but the live log is the sent build ($nlines lines: the game ran)"
        exit 1
    fi
    # The bench record of this log: bench name from PM_BENCH (else <name>), build number from the log's "build N git H" line.
    bno=$(sed -n 's/.* build \([0-9][0-9]*\) git .*/\1/p' "$log" | head -1)
    echo "record: python 3ds/tools/bench.py ingest $log --bench ${PM_BENCH:-<name>} --host o3ds${bno:+ --build $bno}"
    case "$1" in
        push) echo "PUSH OK, $nlines log lines"; exit 0 ;;
        attach) echo "ATTACH OK, $nlines log lines"; exit 0 ;;
    esac
}

ip=${ipopt:-${N3DS_IP:-${PM_O3DS_IP:-$(py3 - <<'EOF'
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
s.bind(("", 17491))
s.settimeout(1.0)
for _ in range(8):
    s.sendto(b"3dsboot", ("255.255.255.255", 17491))
    try:
        data, addr = s.recvfrom(64)
        if data.startswith(b"boot3ds"):
            print(addr[0]); break
    except socket.timeout:
        pass
EOF
)}}}
if [ -z "$ip" ]; then echo "no 3DS netloader found (press Y in the Homebrew Launcher)"; exit 1; fi
echo "3DS at $ip"
# One ping for the log only: ICMP may be blocked, so no reply does not stop the push.
if ping -n 1 -w 1000 "$ip" >/dev/null 2>&1; then echo "ping $ip: reply"; else echo "ping $ip: no reply (ICMP may be blocked, the push goes on)"; fi
if [ "$attach" -eq 1 ]; then
    echo "attach: no send, reading the live log of $ip"
    read_live_log
    report_log attach
fi
if [ -n "$PM_SRC3DSX" ]; then
    echo "archive skipped: PM_SRC3DSX is not the main build (3ds/include/pm_build.h describes the main build)"
else
    archive_build "ip=$ip $*" || true # best effort: the push goes on without an archive (its warning line is printed above)
fi
[ -f "$SRC3DSX" ] || { echo "no $SRC3DSX. Nothing was pushed."; exit 1; }

# A console that isn't sitting in the Homebrew Launcher yet (most common cause - this needs a human
# to open it there) must not cost minutes of silence. A few short attempts instead of one long one.
#
# HARD REQUIREMENT, not a tuning choice: once the console has sent back ANY response at all (the
# first byte of 3dslink's own --verbose output), this attempt is NEVER force-killed again, no
# matter how long it runs. Confirmed directly: force-killing a netload that had already started
# (the old flat-20s-deadline version did this on every attempt) left BOTH O3DS and N3DS executing
# corrupted code - a DataAbort with a null-ish fault address and a PC that resolves to the wrong
# function entirely, i.e. a hard crash, not a clean abort. A still-silent attempt (console never
# answered at all) has not touched the console's running state yet, so killing that one is safe -
# that is the ONLY case this script ever kills.
# "docker kill" by name, not relying on signal forwarding through the attached client process:
# plain "timeout docker run" does not reliably stop a Docker Desktop container (confirmed directly -
# the very first version of this, timeout-only, still hung past its own timeout).
attempts=3
stall_secs=15
# SEND_SECS: the whole upload (13 MB) must end within this time once the console has answered.
# A flaky Wi-Fi link can hang 3dslink after "Sending ..." for ever (confirmed on the O3DS).
# The user decision 2026-10-20 overrides the old rule "never kill after the first answer":
# kill by PID after SEND_SECS, count the attempt as failed, retry.
send_secs=${SEND_SECS:-60}
LINK=${PM_3DSLINK:-/opt/devkitpro/tools/bin/3dslink}
ok=0
stage=connect
reason=""
i=1
while [ "$i" -le "$attempts" ]; do
    echo "netload attempt $i/$attempts (connect timeout ${stall_secs}s, send timeout ${send_secs}s)..."
    cname="pm3ds_netload_$$_$i"
    outfile="build3ds/.netload_attempt_$$_$i.log"
    : > "$outfile"
    usedocker=0
    if [ -x "$LINK" ]; then
        echo "+ $LINK -a $ip $SRC3DSX -- --verbose $*"
        stdbuf -oL "$LINK" -a "$ip" "$SRC3DSX" -- --verbose livelog "$@" >"$outfile" 2>&1 &
    else
        usedocker=1
        src_dir=$(cd "$(dirname "$SRC3DSX")" && pwd)
        src_name=$(basename "$SRC3DSX")
        echo "+ docker run devkitpro/devkitarm 3dslink -a $ip /out/$src_name -- --verbose $*"
        MSYS_NO_PATHCONV=1 docker run --name "$cname" --rm -v "$src_dir:/out:ro" devkitpro/devkitarm:latest \
            stdbuf -oL /opt/devkitpro/tools/bin/3dslink -a "$ip" "/out/$src_name" -- --verbose livelog "$@" >"$outfile" 2>&1 &
    fi
    dpid=$!
    tail -n +1 -f "$outfile" &
    tpid=$!
    last_size=0
    stalled=0
    started=0
    killed=0
    sent_for=0
    while kill -0 "$dpid" 2>/dev/null; do
        sleep 1
        cur_size=$(wc -c <"$outfile" 2>/dev/null || echo 0)
        if [ "$cur_size" -gt "$last_size" ]; then
            last_size=$cur_size
            stalled=0
            started=1
        elif [ "$started" -eq 0 ]; then
            stalled=$((stalled + 1))
        fi
        [ "$started" -eq 1 ] && sent_for=$((sent_for + 1))
        if [ "$started" -eq 0 ] && [ "$stalled" -ge "$stall_secs" ]; then
            echo "attempt $i: no response for ${stall_secs}s - killing (console never answered)"
            stage=connect; reason="no answer from $ip in ${stall_secs}s"
            killed=1
        elif [ "$started" -eq 1 ] && [ "$sent_for" -ge "$send_secs" ]; then
            echo "attempt $i: send not finished after ${send_secs}s - killing 3dslink"
            stage=send; reason="send stalled for ${send_secs}s"
            killed=1
        fi
        if [ "$killed" -eq 1 ]; then
            # set -e is on: every kill here ends with || true
            kill "$dpid" 2>/dev/null || true
            if [ "$usedocker" -eq 1 ]; then docker kill "$cname" >/dev/null 2>&1 || true; fi
            sleep 1
            kill -9 "$dpid" 2>/dev/null || true
            break
        fi
    done
    rc=0
    wait "$dpid" 2>/dev/null || rc=$?
    kill "$tpid" 2>/dev/null || true
    wait "$tpid" 2>/dev/null || true
    rm -f "$outfile"
    if [ "$killed" -eq 0 ] && [ "$rc" -eq 0 ]; then
        ok=1
        break
    fi
    if [ "$killed" -eq 0 ]; then
        if [ "$started" -eq 1 ]; then stage=send; reason="3dslink exit $rc after the console answered"
        else stage=connect; reason="3dslink exit $rc, console did not answer"; fi
    fi
    i=$((i + 1))
    [ "$i" -le "$attempts" ] && sleep 2
done
if [ "$ok" -ne 1 ]; then
    if [ "$stage" != send ]; then
        echo "PUSH FAILED stage=$stage: $reason (after $attempts attempts, $ip)"
        exit 1
    fi
    echo "PUSH FAILED stage=send: $reason (after $attempts attempts, $ip). The console answered, so the live log is read anyway: the game may be running."
    read_live_log
    report_log send
fi
read_live_log
report_log push
