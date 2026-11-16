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
