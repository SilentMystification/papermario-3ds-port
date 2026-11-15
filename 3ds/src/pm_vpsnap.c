#include <string.h>
#include "ultra64.h"
#include "pm_port.h"

extern s32 gCurrentDisplayContextIndex;

/* CORE2-3: viewport copies for the list (gbi_custom.h PM_VP). One pool per display context; a pool is reused when its context is built
 * again, two frames later, after the walk of the frame that used it. Full pool: the original pointer (one warning). */
/* HWFIX: a repeat of one of the last 2 values of this frame gets the same slot (sprite.c:338-339 snaps vpAlt and vpMain per component). */
#define PM_VP_POOL 128
Vp* pm_vp_snap(const Vp* v) {
    static Vp pool[2][PM_VP_POOL];
    static unsigned n, last_f = ~0u;
    static int warned;
    unsigned f = pm_frame_count();
    if (f != last_f) { last_f = f; n = 0; }
    Vp* const p = pool[gCurrentDisplayContextIndex & 1];
    for (unsigned k = n > 2u ? n - 2u : 0u; k < n; k++) if (!memcmp(&p[k], v, sizeof *v)) return &p[k];
    if (n >= PM_VP_POOL) { if (!warned) { warned = 1; pm_log_w("vp pool full (%u viewports in one frame): original pointer", n); } return (Vp*)v; }
    Vp* c = &p[n++];
    *c = *v;
    return c;
}
