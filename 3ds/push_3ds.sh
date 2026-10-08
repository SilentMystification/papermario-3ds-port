#!/bin/sh
# Push build3ds/pm_3ds.3dsx to the 3DS Homebrew Launcher netloader, then read the game's
# live log over Wi-Fi for -t seconds.
# Usage (repo root, Git Bash): sh 3ds/push_3ds.sh [-t seconds] [game args...]
#   e.g. sh 3ds/push_3ds.sh -t 90 gputest
# The 3DS address comes from $N3DS_IP, else from a UDP broadcast (the same probe 3dslink uses).
# The game runs a log server on TCP port 17492 when netloaded (3ds/src/pm_main.c). This PC
# connects to it, the same direction as the upload, so a PC firewall or VPN does not block it.
# Output: build3ds/hw_live_log_<last octet of the IP>.txt (also printed), one file per console, so two
# consoles can be captured at the same time. Run it in the background for long runs.
set -e
cd "$(dirname "$0")/.."
secs=60
if [ "$1" = "-t" ]; then secs=$2; shift 2; fi
# PYTHON_EXE lets a caller (3ds/go.ps1) pass a reliably-resolved interpreter path: bash's own PATH
# (MSYS) does not always see the Windows "py" launcher even when PowerShell's PATH does.
PY=${PYTHON_EXE:-py}

ip=${N3DS_IP:-$("$PY" -3 - <<'EOF'
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
)}
if [ -z "$ip" ]; then echo "no 3DS netloader found (press Y in the Homebrew Launcher)"; exit 1; fi
echo "3DS at $ip"

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
ok=0
i=1
while [ "$i" -le "$attempts" ]; do
    echo "netload attempt $i/$attempts (killable only before the console responds, ${stall_secs}s)..."
    cname="pm3ds_netload_$$_$i"
    outfile="build3ds/.netload_attempt_$$_$i.log"
    : > "$outfile"
    ( MSYS_NO_PATHCONV=1 docker run --name "$cname" --rm -v "$(pwd)/build3ds:/out:ro" devkitpro/devkitarm:latest \
        stdbuf -oL /opt/devkitpro/tools/bin/3dslink -a "$ip" /out/pm_3ds.3dsx -- --verbose "$@" >"$outfile" 2>&1 ) &
    dpid=$!
    tail -n +1 -f "$outfile" &
    tpid=$!
    last_size=0
    stalled=0
    started=0
    killed=0
    while kill -0 "$dpid" 2>/dev/null; do
        sleep 1
        cur_size=$(wc -c <"$outfile" 2>/dev/null || echo 0)
        if [ "$cur_size" -gt "$last_size" ]; then
            last_size=$cur_size
            stalled=0
            started=1 # the console has responded - this attempt is no longer killable, ever
        elif [ "$started" -eq 0 ]; then
            stalled=$((stalled + 1))
        fi
        if [ "$started" -eq 0 ] && [ "$stalled" -ge "$stall_secs" ]; then
            echo "attempt $i: no response for ${stall_secs}s - force-killing (console never answered, nothing to corrupt)"
            docker kill "$cname" >/dev/null 2>&1
            killed=1
            break
        fi
    done
    rc=0
    wait "$dpid" || rc=$?
    kill "$tpid" 2>/dev/null || true
    wait "$tpid" 2>/dev/null || true
    rm -f "$outfile"
    if [ "$killed" -eq 0 ] && [ "$rc" -eq 0 ]; then
        ok=1
        break
    fi
    i=$((i + 1))
    [ "$i" -le "$attempts" ] && sleep 2
done
if [ "$ok" -ne 1 ]; then
    echo "netload failed after $attempts attempts - the console at $ip is most likely not sitting in the"
    echo "Homebrew Launcher (press Y there first), not a tooling problem. Not the game's own log server -"
    echo "that only starts once netload succeeds."
    exit 1
fi

echo "reading the game's log from $ip:17492 for $secs s"
log="build3ds/hw_live_log_${ip##*.}.txt"
"$PY" -3 -u 3ds/tools/livelog.py "$ip" "$secs" "$log"
echo "--- log saved to $log"
