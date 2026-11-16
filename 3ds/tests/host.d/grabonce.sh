#!/bin/sh
# GRABSEQ: nd_grab_once (nd_grabonce.h) copies once per drawn frame; nd_target.c keys both grabs on pm_gpu_seq, never on pm_frame_count().
mkdir -p /tmp/grabonce_seed
sed 's/if (\*last == seq) return 0;/if (0) return 0;/' 3ds/include/nd_grabonce.h > /tmp/grabonce_seed/nd_grabonce.h
gcc -m32 -w -I /tmp/grabonce_seed -o /tmp/grabonce_seed/t 3ds/tools/nd_grabonce_test.c || exit 1
got=$(/tmp/grabonce_seed/t); if [ "$got" != "grabonce FAIL walk 0 copies 2" ]; then echo "grabonce seed FAIL: got [$got]"; exit 1; fi
echo "        if (hold_grab_f == pm_frame_count()) return;" | grep -q "grab_f.*pm_frame_count" || { echo "grabonce lint seed FAIL"; exit 1; }
if grep -n "grab_f.*pm_frame_count" 3ds/src/nd_target.c; then echo "grabonce lint FAIL: a grab key on pm_frame_count"; exit 1; fi
n=$(grep -c "nd_grab_once(&\(hold\|splash\)_grab_f, pm_gpu_seq)" 3ds/src/nd_target.c)
if [ "$n" != "2" ]; then echo "grabonce lint FAIL: $n of 2 grab keys on pm_gpu_seq"; exit 1; fi
gcc -m32 -Wall -Wextra -I 3ds/include -o /tmp/grabonce_t 3ds/tools/nd_grabonce_test.c && /tmp/grabonce_t || exit 1
