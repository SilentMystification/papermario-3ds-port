#!/bin/sh
# perf_assemble.sh - build 3ds/logs/perf/baseline_<build>.txt from the run logs (bash port of the
# Select-String part of perf_capture.ps1).
# Usage (repo root): sh 3ds/tools/perf_assemble.sh [build] [dir]   default build 840, dir 3ds/logs/perf
# Input: <dir>/<build>_<RUN>.txt, <build>_<RUN>_phase.txt (E1 E2 G), <build>_G_pull.txt.
B=${1:-840}
D=${2:-3ds/logs/perf}
OUT=$D/baseline_$B.txt

# pick file pattern count first|last
pick() {
    if [ ! -f "$1" ]; then echo "  (no log)"; return; fi
    m=$(grep -E "$2" "$1" | sed 's/^[[:space:]]*/  /; s/[[:space:]]*$//')
    if [ -z "$m" ]; then echo "  (no line)"; return; fi
    if [ "$4" = first ]; then echo "$m" | head -n "$3"; else echo "$m" | tail -n "$3"; fi
}

{
    echo "PERF baseline, build $B (includes PO2), O3DS 192.168.1.180, map_restore_table.c not in the build, $(date '+%Y-%m-%d %H:%M')"
    for r in A B C D F E1 E2 G; do
        f=$D/${B}_$r.txt
        echo
        case $r in
            A) echo "== run A: loglevel=debug"
               pick "$f" 'mem commit limit|mem split H03|APT_SetAppCpuTimeLimit' 3 first ;;
            B|C|D|F)
               echo "== run $r"
               pick "$f" 'audio cost avg' 3 last
               pick "$f" 'audio posts' 2 last
               pick "$f" 'pace: wall' 5 last
               [ $r = C ] && pick "$f" 'linear used .* peak|texture failures' 2 last ;;
            *)
               echo "== run $r (phase)"
               p=$D/${B}_${r}_phase.txt
               if [ -f "$p" ]; then sed 's/^/  /' "$p"; else echo "  (no phase report)"; fi
               if [ $r = G ]; then
                   g=$D/${B}_G_pull.txt
                   pick "$g" 'b7 f' 6 last
                   pick "$g" 'vram free' 4 last
               fi ;;
        esac
    done
} > "$OUT"
echo "baseline written: $OUT"
