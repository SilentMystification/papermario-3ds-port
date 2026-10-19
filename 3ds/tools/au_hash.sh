#!/bin/bash
# AU1: audio hash of the smoke runs. usage: au_hash.sh <tag> <repeat count>. Needs PM_ROW. Prints one line per run.
cd "$(dirname "$0")/../.."
tag=$1; n=${2:-2}
for sc in "preset=toad" "preset=yard" "preset=toad battle"; do
  for i in $(seq 1 $n); do
    powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Timeout $((300+i)) -Debug "fast $sc audiohash audiolock fixedseed smokeframes=700" >/dev/null 2>&1
    echo "$tag [$sc] run $i: $(grep 'audiohash.*final' build3ds/last_log.txt | sed 's/^#[0-9]* //')"
  done
done
