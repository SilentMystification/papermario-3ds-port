#pragma once
/* MTX1: reader stamps. done and s are reader entry counts (nd_wq.c, modular 32 bit): due when done has reached s. */
static inline int pm_stamp_due(unsigned done, unsigned s) { return (int)(done - s) >= 0; }
