/* SPEEDX2: the Speed x2 cheat. The base speeds are saved when the cheat turns on and set from the base each step, so the
 * speed is doubled once and does not compound. Turning the cheat off restores the base. */
#ifndef PM_SPEEDX2_H
#define PM_SPEEDX2_H

typedef struct { float walk, run; int on; } PmSpd;

static inline void pm_speedx2(PmSpd* s, int want, float* walk, float* run) {
    if (want && !s->on) { s->walk = *walk; s->run = *run; s->on = 1; }
    if (want) { *walk = s->walk * 2.0f; *run = s->run * 2.0f; } else if (s->on) { *walk = s->walk; *run = s->run; s->on = 0; }
}

#endif
