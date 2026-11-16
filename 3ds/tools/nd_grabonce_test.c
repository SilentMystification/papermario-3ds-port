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
