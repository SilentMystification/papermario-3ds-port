#!/bin/sh
# run_azahar.sh - Linux port of the core of 3ds/run_azahar.ps1 (flatpak Azahar).
# Usage (repo root): PM_ROW=<row> sh 3ds/run_azahar.sh [-x 3dsx] [-d "switches"] [-t secs] [-u regex] [-o outlog] [-c n] [-F]
#   -x 3dsx to run (default build3ds/pm_3ds.3dsx)   -d game switches (debug3ds.txt)
#   -t timeout in seconds (default 60)               -u stop when a log line matches this regex
#   -o copy of the game log (default build3ds/last_log.txt)
#   -c n  stop when -u matched n times (default 1)
#   -F    fast: Azahar frame_limit=1000, vsync off. Saves qt-config.ini first and restores it at the end (also on Ctrl-C).
# Same rules as the .ps1: one Azahar user at a time (flock), 8 starts per row (3ds/logs/test_budget.csv,
# PM_BUDGET overrides), kill only this run's Azahar (flatpak kill by app id), stop on [CRASH] or 10 s of log silence.
# Not ported: -Turbo, -Capture, -Gdb, -Smoke, cached reruns, the tour size check.
set -u
cd "$(dirname "$0")/.."
X=build3ds/pm_3ds.3dsx; D=""; T=60; U=""; O=build3ds/last_log.txt; C=1; FAST=0
while [ $# -gt 0 ]; do case "$1" in
    -F) FAST=1; shift; continue ;;
    -x) X=$2;; -d) D=$2;; -t) T=$2;; -u) U=$2;; -o) O=$2;; -c) C=$2;;
    *) echo "bad arg $1"; exit 2;; esac; shift 2; done
APP=org.azahar_emu.Azahar
SD=$HOME/.var/app/$APP/data/azahar-emu/sdmc/3ds/PaperMario
ROW=${PM_ROW:-adhoc}; MAX=${PM_BUDGET:-8}; [ "$ROW" = adhoc ] && MAX=12
B=3ds/logs/test_budget.csv; mkdir -p 3ds/logs; touch $B
used=$(awk -F, -v r="$ROW" '$2==r && $5=="start"' $B | wc -l)
if [ "$used" -ge "$MAX" ]; then echo "BUDGET: row $ROW used $used runs. Stop and report what blocks you."; exit 9; fi
exec 9>/tmp/pm_azahar_run.lock
flock -w 900 9 || { echo "LOCK TIMEOUT"; exit 3; }
if flatpak ps 2>/dev/null | grep -q $APP; then echo "Azahar already runs (not started by this script). Stop."; exit 4; fi
sha=$(sha256sum "$X" | cut -c1-64); stamp=$(date +%Y-%m-%dT%H:%M:%S)
echo "$stamp,$ROW,$sha,x=$X d=$D,start,," >> $B
rm -rf "$SD/shots" "$(dirname "$O")/shots"   # the "shots" switch writes $SD/shots; golden.py reads it next to the log
mkdir -p "$SD"; cp -f "$X" "$SD/pm_3ds.3dsx"; rm -f "$SD/log.txt"
if [ -n "$D" ]; then printf '%s' "$D" > "$SD/debug3ds.txt"; else rm -f "$SD/debug3ds.txt"; fi
echo "budget: row $ROW run $((used+1))/$MAX; flatpak run $APP $SD/pm_3ds.3dsx; switches: $D"
QC=$HOME/.var/app/$APP/config/azahar-emu/qt-config.ini
restore_cfg() { [ -f /tmp/claude-1000/qt-config.user ] && cp -f /tmp/claude-1000/qt-config.user "$QC" && rm -f /tmp/claude-1000/qt-config.user; }
trap 'flatpak kill $APP >/dev/null 2>&1; restore_cfg; exit 130' INT TERM
restore_cfg   # a killed earlier run left a backup: put the user file back first
if [ "$FAST" -eq 1 ]; then
    cp -f "$QC" /tmp/claude-1000/qt-config.user
    sed -i -e 's/^frame_limit=.*/frame_limit=1000/' -e 's/^frame_limit\\default=.*/frame_limit\\default=false/' \
           -e 's/^use_vsync=.*/use_vsync=false/' -e 's/^use_vsync\\default=.*/use_vsync\\default=false/' "$QC"
    echo "fast: $(grep -E '^(frame_limit|use_vsync|is_new_3ds)=' "$QC" | tr '\n' ' ')"
fi
flatpak run $APP "$SD/pm_3ds.3dsx" >/tmp/claude-1000/azahar_stdout.txt 2>&1 &
start=$(date +%s); reason="timeout ($T s)"; last=-1; lastgrow=$start
while [ $(( $(date +%s) - start )) -lt "$T" ]; do
    sleep 1
    if [ -f "$SD/log.txt" ]; then
        sz=$(wc -c < "$SD/log.txt")
        if [ "$sz" != "$last" ]; then last=$sz; lastgrow=$(date +%s)
        elif [ $(( $(date +%s) - lastgrow )) -gt 10 ]; then reason="game log silent for 10 s"; break; fi
        if grep -q '\[CRASH\]' "$SD/log.txt"; then reason=crash; break; fi
        if [ -n "$U" ] && [ "$(grep -cE "$U" "$SD/log.txt")" -ge "$C" ]; then reason="matched $U"; break; fi
    elif [ $(( $(date +%s) - start )) -gt 40 ]; then reason="no game log after 40 s (dialog?)"; break; fi
done
sleep 1; flatpak kill $APP >/dev/null 2>&1; wait 2>/dev/null; restore_cfg
rm -f "$SD/debug3ds.txt"
[ -f "$SD/log.txt" ] && cp -f "$SD/log.txt" "$O"
[ -d "$SD/shots" ] && cp -r "$SD/shots" "$(dirname "$O")/shots"
echo "stopped after $(( $(date +%s) - start )) s: $reason"
echo "$(date +%Y-%m-%dT%H:%M:%S),$ROW,$sha,x=$X d=$D,end,0,$reason" >> $B
tail -n 8 "$O" 2>/dev/null
