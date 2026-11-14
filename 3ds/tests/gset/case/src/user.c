#include "effects/fake.h"
extern Gfx X_OK[];
Gfx* handles[] = { X_OK, 0 };
void user(void) {
    nd_draw_list(X_OK, ND_SELF);
    n = sizeof(X_OK);
    p = &X_OK;
    q = X_OK[1];
    nd_draw_list(X_BAD, ND_SELF);
}
