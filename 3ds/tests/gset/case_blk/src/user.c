#include "effects/fake.h"
extern Gfx A[];
void user(void) {
    nd_draw_list(A, ND_SELF);
}
