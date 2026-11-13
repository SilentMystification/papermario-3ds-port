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
# "--profile prof|log|none" appends the named profile words of bench_defs.json "profiles" after the game args
# (the first match wins, so game args given by hand override the profile). "--dry" prints the 3dslink argv and exits:
# no build, no ip lookup, no send.
# MEMGATE: before the send (and in --dry) 3ds/tools/memcheck.py checks the map next to the 3dsx (PM_MEMCHECK_MAP overrides it)
# against the O3DS XL applet-mode budget (3ds/tools/mem_budget.json). OVER prints a warning and stops the push, exit 4.
# "--mem-ok" pushes an OVER build anyway: the override is printed and goes into the flags of the pushed_builds.txt line.
# NETLOADER REFUSED: a 3dslink output with a "N sent (P%)" line below 100 is a partial send (the console
# refused or closed the upload, typically not enough free RAM in applet mode). Then the instruction block is printed
# with the 3dsx size and the last line of pushed_builds.txt. "--check-send <3dslink output file>" runs only that check (tests).
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
# "--idle <s>" (default 300): the live log ends when no complete line came for s seconds (no connection counts), with
# the line "live log idle <s> s: stop". -t stays the hard limit. A line resets the timer.
# Owners: this bash's Windows pid and its caller's (PM_OWNER_PID from go.ps1, else the parent bash's). The live log
# (livelog.py --owner), the netload tail (tail --pid) and the netload wait end when an owner is gone. A send in progress
# is never killed (the console answered): only a silent attempt is killed, as before.
set -e
cd "$(dirname "$0")/.."
# PUSHEND1: end_push <rc> <result> <reason>: the one end line of a push (result ok, sent-no-log or failed), also in
# build3ds/out/push_result.txt (go.ps1 puts it into its PUSH END line). Exit codes: 0 ok, 6 sent but the game sent no line, else failed.
end_push() { line="PUSH RESULT $2 rc $1: $3"; echo "$line"; mkdir -p build3ds/out; printf '%s\n' "$line" >build3ds/out/push_result.txt; exit "$1"; }
secs=0
idle=300
ipopt=""
nobuild=0
archonly=0
attach=0
checklog=""
checksend=""
partial_pct=""
profname=""
dry=0
memok=0
memflag=""
# PUSHSTALL: netio_tx "<rx> / <tx>" prints the tx part of docker stats NetIO in bytes (decimal units B, kB, MB, GB, TB).
netio_tx() {
    printf '%s\n' "$1" | awk -F'/' '{ v = $2; gsub(/ /, "", v); n = v + 0; u = v; sub(/^[0-9.]+/, "", u); m = 1
        if (u == "kB") m = 1e3; else if (u == "MB") m = 1e6; else if (u == "GB") m = 1e9; else if (u == "TB") m = 1e12
        printf "%.0f\n", n * m }'
}
while :; do
    case "$1" in
        -t) secs=$2; shift 2 ;;
        --ip) ipopt=$2; shift 2 ;;
        --no-build) nobuild=1; shift ;;
        --archive-only) archonly=1; shift ;;
        --attach) attach=1; shift ;;
        --check-log) checklog=$2; shift 2 ;;
        --check-send) checksend=$2; shift 2 ;;
        --profile) profname=$2; shift 2 ;;
        --dry) dry=1; shift ;;
        --mem-ok) memok=1; shift ;;
        --idle) idle=$2; shift 2 ;;
        --netio) netio_tx "$2"; exit 0 ;;
        *) break ;;
    esac
done

# Owners (Windows pids, comma list): this bash and its caller. go.ps1 sets PM_OWNER_PID to its own pid; without it the
# caller is the parent bash. /proc/<pid>/winpid is the Windows pid of a cygwin process (empty when there is none).
own_win=$(cat /proc/$$/winpid 2>/dev/null || true)
caller=${PM_OWNER_PID:-$(cat /proc/$PPID/winpid 2>/dev/null || true)}
owners=$own_win${caller:+,$caller}

# The 3dsx this push sends: build3ds/pm_3ds.3dsx, or PM_SRC3DSX (bench.py build-at sends a lane build from its scratch folder).
SRC3DSX=${PM_SRC3DSX:-build3ds/pm_3ds.3dsx}

# The profile words (bench_defs.json "profiles") go after the game args, so the game args given by hand come first and win.
if [ -n "$profname" ]; then
    PROF_PY='import json, sys
p = json.load(open("3ds/tools/bench_defs.json", encoding="utf-8")).get("profiles", {})
n = sys.argv[1]
if n not in p:
    print("push_3ds.sh: unknown --profile %s (prof, log, none)" % n, file=sys.stderr)
    sys.exit(2)
print(p[n])'
    PYP=${PYTHON_EXE:-py}
    if [ -z "$PYTHON_EXE" ] && ! command -v py >/dev/null 2>&1; then PYP=python3; fi
    if [ "$PYP" = python3 ]; then pw=$(python3 -c "$PROF_PY" "$profname"); else pw=$("$PYP" -3 -c "$PROF_PY" "$profname"); fi
    for w in $pw; do set -- "$@" "$w"; done
fi
# CLEANB: keep the newest 3 .3dsx archives in $adir. "dry" lists what would go, "run" deletes;
# $2 (the archive just written or checked) is never deleted.
prune_archives() {
    adir=${PM_ARCHIVE_DIR:-3ds/logs/builds}
    ls -t "$adir"/pm_3ds_*.3dsx 2>/dev/null | awk -v keep="$2" 'NR>3 && $0!=keep' | while read -r old; do
        if [ "$1" = dry ]; then echo "would prune archive $old"; else rm -f -- "$old" && echo "pruned archive $old"; fi
    done
}
# mem_gate: memcheck of the map next to the 3dsx. OVER without --mem-ok: warning block and exit 4. A missing map is a note only.
mem_gate() {
    mmap=${PM_MEMCHECK_MAP:-$(dirname "$SRC3DSX")/pm_3ds.map}
    mrc=0
    if command -v py >/dev/null 2>&1; then mout=$(py -3 3ds/tools/memcheck.py --map "$mmap") || mrc=$?
    else mout=$(python3 3ds/tools/memcheck.py --map "$mmap") || mrc=$?; fi
    printf '%s\n' "$mout"
    [ "$mrc" -eq 3 ] || return 0
    mline=$(printf '%s\n' "$mout" | sed -n '1s/^memcheck: //p')
    if [ "$memok" -eq 1 ]; then
        echo "MEM OVERRIDE (--mem-ok): pushing an OVER build ($mline)"
        memflag="mem-ok($mline) "
        return 0
    fi
    echo "=================================================================="
    echo "MEMORY OVER: $mline"
    mm=$(printf '%s\n' "$mline" | sed -n 's/.* margin \(-*[0-9]*\) .*/\1/p')
    if [ "${mm:-0}" -lt 0 ]; then fit="the ROM will not fit"; else fit="the ROM may not fit (margin inside the safety of mem_budget.json)"; fi
    echo "On the O3DS XL in applet mode (Homebrew Launcher through a game or Download Play) $fit:"
    echo "the game stops at boot with \"Not enough memory for the ROM\". Push not sent."
    echo "Make the build smaller (largest .data/.bss above), or push anyway with --mem-ok (go.ps1 -MemOk)."
    echo "=================================================================="
    end_push 4 failed "memory OVER, push not sent (go.ps1 -MemOk sends anyway)"
}
if [ "$dry" -eq 1 ]; then
    mem_gate
    dip=${ipopt:-${N3DS_IP:-${PM_O3DS_IP:-<netloader broadcast>}}}
    echo "dry run: nothing built, nothing sent"
    echo "3dslink argv: ${PM_3DSLINK:-/opt/devkitpro/tools/bin/3dslink} -a $dip $SRC3DSX -- --verbose livelog $*"
    prune_archives dry ""
    exit 0
fi
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
    prune_archives run "$dst"
    echo "ARCHIVED $dst (sha256 $sha)"
}

# build.sh is the only stale check (content key, BSTALE): it skips the build when nothing changed.
# This script never stashes or changes user files.
if [ "$nobuild" -eq 0 ] && [ "$attach" -eq 0 ] && [ -z "$checklog" ] && [ -z "$checksend" ]; then
    sh 3ds/build.sh
fi
if [ "$archonly" -eq 1 ]; then archive_build "archive-only" || exit 1; exit 0; fi
if [ "$attach" -eq 0 ] && [ -z "$checklog" ] && [ -z "$checksend" ]; then mem_gate; fi
# PUSHFIX STALE 3DSX: --no-build sends the 3dsx as it is. Rule: the key of the current folder (`PM_LANE= PM_KEY_ONLY=1 sh 3ds/build.sh`,
# run in the folder that holds build3ds/pm_3ds.3dsx) must equal that folder's own build3ds/build_key.txt (written by build.sh at a
# successful build), else nothing is sent. The key hashes each folder's own generated files (build.sh key_now), so a key is valid
# only against the build_key.txt of the same folder and output. A push runs from the main folder (go.ps1 Set-Location $repo).
# PM_BUILD_KEY_FILE (tests only) names another key file; the check then runs even when PM_SRC3DSX is set.
if [ "$nobuild" -eq 1 ] && { [ -z "$PM_SRC3DSX" ] || [ -n "$PM_BUILD_KEY_FILE" ]; }; then
    have=$(cat "${PM_BUILD_KEY_FILE:-build3ds/build_key.txt}" 2>/dev/null || true)
    now=$(PM_LANE=${PM_KEY_LANE:-} PM_KEY_ONLY=1 sh 3ds/build.sh || true)
    if [ -z "$have" ] || [ "$have" != "$now" ]; then
        echo "STALE 3DSX: build key $(echo "$have" | cut -c1-12), tree key $(echo "$now" | cut -c1-12) (a source, include or tool input changed since the build). Build first. Nothing was pushed."
        end_push 5 failed "stale 3dsx, nothing sent"
    fi
fi
# SEND FILE: the 3dsx this run sends, its size, the build of pm_build.h and the key it was built with (build_key.txt).
if [ "$attach" -eq 0 ] && [ -z "$checklog" ] && [ -z "$checksend" ]; then
    if [ -f "$SRC3DSX" ]; then sfz="$(wc -c <"$SRC3DSX" | tr -d ' ')"; else sfz="n/a"; fi
    sfb=$(sed -n 's/^#define PM_BUILD_STR "\(.*\)"/\1/p' 3ds/include/pm_build.h 2>/dev/null || true)
    sfk=$(cut -c1-12 "${PM_BUILD_KEY_FILE:-build3ds/build_key.txt}" 2>/dev/null || true)
    echo "SEND FILE: $SRC3DSX $sfz bytes build ${sfb:-none} key ${sfk:-none}"
fi
# PYTHON_EXE lets a caller (3ds/go.ps1) pass a reliably-resolved interpreter path: bash's own PATH
# (MSYS) does not always see the Windows "py" launcher even when PowerShell's PATH does.
PY=${PYTHON_EXE:-py}
if [ -z "$PYTHON_EXE" ] && ! command -v py >/dev/null 2>&1; then PY=python3; fi
# "py -3" is the Windows launcher form. python3 takes no -3.
py3() { if [ "$PY" = python3 ]; then python3 "$@"; else "$PY" -3 "$@"; fi; }
# owner_gone: 0 when one of $owners is no longer running (livelog.py --alive). Only checked between the netload polls.
owner_gone() { ! py3 -u 3ds/tools/livelog.py --alive "$owners" >/dev/null 2>&1; }

# Read the live log of $ip for $secs (0: until the game closes, at most PM_LOG_MAX). A reconnect after an early end
# (livelog.py exit 3) appends to the same file, one line each, 3 times at most. Sets $log.
read_live_log() {
    if [ "$secs" = 0 ]; then secs=${PM_LOG_MAX:-3600}; echo "reading the game's log from $ip:17492 until the game closes, at most $secs s (stops at $idle s without a line, or when the game closes)"; else echo "reading the game's log from $ip:17492 for $secs s (stops at $idle s without a line, or when the game closes)"; fi
    log="build3ds/hw_live_log_${ip##*.}.txt"
    t_end=$(( $(date +%s) + secs ))
    rc=0
    idle_stop=0
    # --idle: no line for $idle s ends the read (livelog prints "live log idle <s> s: stop"); --owner: the read ends with its owners.
    py3 -u 3ds/tools/livelog.py "$ip" "$secs" "$log" --idle "$idle" --owner "$owners" || rc=$?
    [ "$rc" -eq 4 ] && idle_stop=1
    k=0
    while [ "$rc" -eq 3 ] && [ "$k" -lt 3 ]; do
        left=$(( t_end - $(date +%s) ))
        [ "$left" -gt 0 ] || break
        k=$((k + 1))
        echo "live log ended early (no data): reconnect $k/3 for $left s, appending to $log"
        rc=0
        py3 -u 3ds/tools/livelog.py "$ip" "$left" "$log" --append --idle "$idle" --owner "$owners" || rc=$?
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

# last_sent_pct <3dslink output>: percentage of the last "N sent (P%)" line, empty when none.
# 3dslink may end its progress with CR, so CR counts as a line end.
last_sent_pct() {
    tr '\r' '\n' <"$1" 2>/dev/null | sed -n 's/.* sent (\([0-9.]*\)%).*/\1/p' | tail -n 1
}

# note_partial <3dslink output>: a send line below 100% sets partial_pct (the upload was refused or cut).
note_partial() {
    p=$(last_sent_pct "$1")
    [ -n "$p" ] || return 0
    if awk -v p="$p" 'BEGIN { exit !(p + 0 < 100) }'; then partial_pct=$p; fi
}

# refusal_hint: when partial_pct is set, print the NETLOADER REFUSED block (3dsx size, last pushed size). Else nothing.
refusal_hint() {
    [ -n "$partial_pct" ] || return 0
    if [ -f "$SRC3DSX" ]; then sz="$(wc -c <"$SRC3DSX" | tr -d ' ') bytes"; else sz="n/a (no $SRC3DSX)"; fi
    plog=${PM_PUSH_LOG:-3ds/logs/perf/pushed_builds.txt}
    lastpush=$(tail -n 1 "$plog" 2>/dev/null | awk -v f="$plog" '{ for (i = 2; i <= NF; i++) { if ($i == "bytes") b = $(i - 1); if ($i == "build") n = $(i + 1); if ($i == "git") g = $(i + 1) } if (b != "") printf "%s bytes (build %s git %s)", b, n, g }') || true
    [ -n "$lastpush" ] || lastpush="none in $plog"
    echo "=========================================================="
    echo "NETLOADER REFUSED (likely not enough free RAM in applet mode). On the 3DS: hold R while you start any game to open the Homebrew Launcher in title mode with full memory, then start the netloader (Y) and push again."
    echo "3dsx size: $sz"
    echo "send stopped at: ${partial_pct}%"
    echo "last pushed size (last line of $plog, accept is not logged): $lastpush"
    echo "=========================================================="
}

if [ -n "$checksend" ]; then
    note_partial "$checksend"
    if [ -n "$partial_pct" ]; then refusal_hint; echo "REFUSAL PATTERN: send stopped at ${partial_pct}%"; exit 0; fi
    echo "NO REFUSAL PATTERN: no send line below 100% in $checksend"; exit 1
fi

# $1: push | send | attach. Prints the ingest hint and the result, then exits.
report_log() {
    nlines=$(wc -l <"$log" 2>/dev/null || echo 0)
    if [ "$nlines" -eq 0 ]; then
        if [ "${idle_stop:-0}" -eq 1 ]; then echo "NO LOG: no line from $ip:17492 in $idle s (idle stop; game did not start or log server not reached)"
        else echo "NO LOG: no line from $ip:17492 in $secs s (game did not start or log server not reached)"; fi
        # One TCP connect now: refused = the game's server is not up (game side), timeout/unreachable = network, connected = no line came.
        # After an idle stop the probe is skipped: the idle line already says the console sent nothing.
        if [ "${idle_stop:-0}" -eq 0 ]; then py3 -u 3ds/tools/livelog.py --probe "$ip" || true; fi
        [ "$1" = attach ] && end_push 1 failed "attach: no live log from $ip"
        end_push 6 sent-no-log "the 3dsx was sent, the game sent no line to $ip:17492 (see the NO LOG line above)"
    fi
    # The bench record of this log: bench name from PM_BENCH (else <name>), build number from the log's "build N git H" line.
    bno=$(sed -n 's/.* build \([0-9][0-9]*\) git .*/\1/p' "$log" | head -1)
    echo "record: python 3ds/tools/bench.py ingest $log --bench ${PM_BENCH:-<name>} --host o3ds${bno:+ --build $bno}"
    case "$1" in
        push) end_push 0 ok "sent, $nlines lines read" ;;
        attach) end_push 0 ok "attach, $nlines lines read" ;;
    esac
}

if [ -n "$ipopt" ]; then ip=$ipopt
elif [ -n "$N3DS_IP" ]; then ip=$N3DS_IP
elif [ -n "$PM_O3DS_IP" ]; then ip=$PM_O3DS_IP
else
    # PUSHFIX: the broadcast probe takes up to 8 s and printed nothing before; say so, and say when it got no answer.
    echo "probe: netloader broadcast (8 tries)..."
    ip=$(py3 - <<'EOF'
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
)
    [ -n "$ip" ] || echo "probe: no answer"
fi
if [ -z "$ip" ]; then end_push 1 failed "no 3DS netloader found (press Y in the Homebrew Launcher)"; fi
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
    archive_build "ip=$ip $memflag$*" || true # best effort: the push goes on without an archive (its warning line is printed above)
fi
[ -f "$SRC3DSX" ] || end_push 1 failed "no $SRC3DSX, nothing was pushed"

# A console that isn't sitting in the Homebrew Launcher yet (most common cause - this needs a human
# to open it there) must not cost minutes of silence. A few short attempts instead of one long one.
#
# Kill rule. A still-silent attempt (console never answered at all) has not touched the console's
# running state yet, so it is killed after stall_secs with no answer. Once the console has sent back
# ANY response (the first byte of 3dslink's own --verbose output), the attempt is killed only in two
# cases: (1) NOGROW_SECS (default 60) with no growth of the output (a stalled send), or (2) the
# size-scaled send cap (15 s per MB, at least 120 s; SEND_SECS overrides). The no-progress limit exists
# because force-killing a netload that had already started can leave O3DS and N3DS executing corrupted
# code (a DataAbort with a null-ish fault address, a PC in the wrong function, a hard crash). The old
# fixed 60 s total send limit was removed (user decision 2026-11-12): it killed healthy sends of the
# 38 to 51 MB 3dsx.
# "docker kill" by name, not relying on signal forwarding through the attached client process:
# plain "timeout docker run" does not reliably stop a Docker Desktop container (confirmed directly -
# the very first version of this, timeout-only, still hung past its own timeout).
attempts=3
mkdir -p build3ds/out
stall_secs=15
# Send kill rule (user decision 2026-11-12). A fixed send time killed healthy sends of the 38 to 51 MB 3dsx
# at a random percentage, so the old 60 s cap is gone. The attempt is killed only when the console has
# answered and the output has not grown for NOGROW_SECS (a stalled send), or when the send runs past a
# hard cap of 15 s per MB (at least 120 s). SEND_SECS, when set, overrides the cap. A flaky Wi-Fi link can
# still hang 3dslink after "Sending ..." for ever (confirmed on the O3DS): the no-progress kill ends that.
nogrow_secs=${NOGROW_SECS:-60}
size_mb=$(( ($(wc -c <"$SRC3DSX") + 1048575) / 1048576 ))
send_cap=$(( size_mb * 15 )); [ "$send_cap" -lt 120 ] && send_cap=120
send_cap=${SEND_SECS:-$send_cap}
LINK=${PM_3DSLINK:-/opt/devkitpro/tools/bin/3dslink}
ok=0
stage=connect
reason=""
ownerdead=0
dpid=""
tpid=""
# cleanup (EXIT, INT, TERM, HUP): the netload tail ends with this script. 3dslink (or its docker client) is killed here only
# while the console has not answered (started 0), the one case this script ever kills.
cleanup() {
    [ -n "$tpid" ] && kill "$tpid" 2>/dev/null || true
    if [ -n "$dpid" ] && [ "${started:-0}" -eq 0 ]; then
        if [ "${usedocker:-0}" -eq 1 ]; then docker kill "$cname" >/dev/null 2>&1 || true; fi
        kill "$dpid" 2>/dev/null || true
    fi
    if [ -n "${outfile:-}" ]; then rm -f "$outfile"; fi
}
trap cleanup EXIT INT TERM HUP
i=1
while [ "$i" -le "$attempts" ]; do
    echo "netload attempt $i/$attempts (connect timeout ${stall_secs}s, no-progress ${nogrow_secs}s, send cap ${send_cap}s for ${size_mb} MB)..."
    cname="pm3ds_netload_$$_$i"
    outfile="build3ds/.netload_attempt_$$_$i.log"
    : > "$outfile"
    usedocker=0
    if [ -x "$LINK" ]; then
        echo "+ $LINK -a $ip $SRC3DSX -- --verbose $*"
        stdbuf -o0 "$LINK" -a "$ip" "$SRC3DSX" -- --verbose livelog "$@" >"$outfile" 2>&1 &
    else
        usedocker=1
        src_dir=$(cd "$(dirname "$SRC3DSX")" && pwd)
        src_name=$(basename "$SRC3DSX")
        echo "+ docker run devkitpro/devkitarm 3dslink -a $ip /out/$src_name -- --verbose $*"
        MSYS_NO_PATHCONV=1 docker run --name "$cname" --rm -v "$src_dir:/out:ro" devkitpro/devkitarm:latest \
            stdbuf -o0 /opt/devkitpro/tools/bin/3dslink -a "$ip" "/out/$src_name" -- --verbose livelog "$@" >"$outfile" 2>&1 &
    fi
    dpid=$!
    # --pid: the tail ends when this script's bash ends, even when it is killed without its trap.
    tail --pid=$$ -n +1 -f "$outfile" &
    tpid=$!
    last_size=0
    stalled=0
    started=0
    killed=0
    sent_for=0
    nogrow=0
    warned=0
    last_tx=0
    while kill -0 "$dpid" 2>/dev/null; do
        sleep 1
        cur_size=$(wc -c <"$outfile" 2>/dev/null || echo 0)
        if [ "$cur_size" -gt "$last_size" ]; then
            last_size=$cur_size
            stalled=0
            nogrow=0
            started=1
        elif [ "$started" -eq 0 ]; then
            stalled=$((stalled + 1))
        else
            nogrow=$((nogrow + 1))
        fi
        [ "$started" -eq 1 ] && sent_for=$((sent_for + 1))
        # PUSHSTALL: 3dslink prints nothing while it sends. In docker the container's sent bytes are the progress (every 5 s).
        if [ "$usedocker" -eq 1 ] && [ "$started" -eq 1 ] && [ $((sent_for % 5)) -eq 0 ]; then
            tx=$(netio_tx "$(docker stats --no-stream --format '{{.NetIO}}' "$cname" 2>/dev/null || true)")
            if [ -n "$tx" ] && [ "$tx" -gt "$last_tx" ]; then last_tx=$tx; nogrow=0; fi
        fi
        # PUSHFIX: the console answered but the output has not grown for 30 s: say so once. Nothing is killed here.
        if [ "$started" -eq 1 ] && [ "$nogrow" -ge 30 ] && [ "$warned" -eq 0 ]; then
            warned=1
            echo "attempt $i: no send progress for 30 s (no output, no network bytes, last line: $(tr '\r' '\n' <"$outfile" | tail -n 1))"
        fi
        # An owner gone: no new attempt, and a silent attempt is killed (a sent one runs on to its own end, the no-progress kill or the cap).
        if [ "$ownerdead" -eq 0 ] && owner_gone; then
            ownerdead=1
            echo "owner of this push is gone: stopping the netload"
            if [ "$started" -eq 0 ]; then stage=connect; reason="owner gone"; killed=1; fi
        fi
        if [ "$started" -eq 0 ] && [ "$stalled" -ge "$stall_secs" ]; then
            echo "attempt $i: no response for ${stall_secs}s - killing (console never answered)"
            stage=connect; reason="no answer from $ip in ${stall_secs}s"
            killed=1
        elif [ "$started" -eq 1 ] && [ "$nogrow" -ge "$nogrow_secs" ]; then
            echo "attempt $i: no send progress for ${nogrow_secs}s - killing 3dslink (last line: $(tr '\r' '\n' <"$outfile" | tail -n 1))"
            stage=send; reason="no send progress for ${nogrow_secs}s"
            killed=1
        elif [ "$started" -eq 1 ] && [ "$sent_for" -ge "$send_cap" ]; then
            echo "attempt $i: send not finished after ${send_cap}s (cap) - killing 3dslink"
            stage=send; reason="send cap ${send_cap}s"
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
    note_partial "$outfile"
    # PUSHFIX: every attempt's 3dslink output is kept (it was deleted before, and the raw output was lost).
    keep="build3ds/out/push_${ip}_$(date +%Y%m%d-%H%M%S)_a${i}.txt"
    mv -f "$outfile" "$keep"
    echo "3dslink output: $keep"
    if [ "$killed" -eq 0 ] && [ "$rc" -eq 0 ]; then
        ok=1
        partial_pct=""
        break
    fi
    if [ "$killed" -eq 0 ]; then
        if [ "$started" -eq 1 ]; then stage=send; reason="3dslink exit $rc after the console answered"
        else stage=connect; reason="3dslink exit $rc, console did not answer"; fi
    fi
    [ "$ownerdead" -eq 1 ] && break
    i=$((i + 1))
    [ "$i" -le "$attempts" ] && sleep 2
done
if [ "$ok" -ne 1 ]; then
    [ "$ownerdead" -eq 1 ] && end_push 1 failed "the caller of this push is gone ($reason)"
    refusal_hint
    end_push 1 failed "stage=$stage: $reason (after $attempts attempts, $ip). The live log is not read."
fi
read_live_log
report_log push
