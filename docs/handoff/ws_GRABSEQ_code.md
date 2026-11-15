# ws_GRABSEQ code blocks (copy exactly)

## Block A (pm_gpu.c, Edit 1)
Line above `void pm_gpu_begin_wait(void) {`:
```c
unsigned pm_gpu_seq; /* GRABSEQ: one step per frame begin, where the off-screen slots drop (nd_target.c grab_cur keys the grab hold and the splash grab on it) */
```
Line after `    begin_pend = 0;`:
```c
    pm_gpu_seq++; /* GRABSEQ: a walked key frame and the next L frame share pm_frame_count(); each begin is a new drawn frame */
```

## Block B (3ds/include/nd_grabonce.h)
```c
#ifndef ND_GRABONCE_H
#define ND_GRABONCE_H
/* GRABSEQ: the grab hold and the splash grab copy the colour buffer once per drawn frame. seq: pm_gpu_seq (pm_gpu.c pm_gpu_begin_wait,
 * one step per frame begin, where the off-screen slots drop). last: the seq of the last copy. Returns 1 when the caller copies now. */
static inline int nd_grab_once(unsigned* last, unsigned seq) {
    if (*last == seq) return 0;
    *last = seq;
    return 1;
}
#endif
```

## Block C (3ds/tools/nd_grabonce_test.c)
```c
/* GRABSEQ host test: nd_grab_once (nd_grabonce.h) copies once per drawn frame. Three walks of 60 FPS: L of step 5 (frame count 4), a walked
 * key frame of step 5 (frame count 5), L of step 6 (frame count 5). Each walk begins a frame (seq 1, 2, 3) and asks twice (two cells). */
#include <stdio.h>
#include "nd_grabonce.h"

int main(void) {
    static const unsigned seq[3] = { 1u, 2u, 3u };
    unsigned last = ~0u, w, k, n;
    for (w = 0; w < 3u; w++) {
        n = 0;
        for (k = 0; k < 2u; k++) n += (unsigned)nd_grab_once(&last, seq[w]);
        if (n != 1u) { printf("grabonce FAIL walk %u copies %u\n", w, n); return 1; }
    }
    printf("grabonce ok\n");
    return 0;
}
```

## Block D (3ds/tests/host.d/grabonce.sh)
```sh
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
```
