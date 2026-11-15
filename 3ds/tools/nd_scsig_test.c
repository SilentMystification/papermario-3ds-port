/* HWFIX: host test of nd_sc_flags (3ds/include/nd_scsig.h). Prints "scsig ok 10/10" or "scsig FAIL <name>" and returns 1. */
#include <stdio.h>
#include "nd_scsig.h"

static const char* names[10] = { "tex_on", "use_t1", "t1_valid", "dual_on", "box_prod", "box_pat", "box_folded", "comb_set", "comb_alpha_shade", "vprim" };

/* the flag word with only argument i set to 1; i = -1 gives all zero */
static unsigned flags_of(int i) {
    return nd_sc_flags(i == 0, i == 1, i == 2, i == 3, i == 4, i == 5, i == 6, i == 7, i == 8, i == 9);
}

int main(void) {
    unsigned v[11];
    v[10] = flags_of(-1);
    for (int i = 0; i < 10; i++) v[i] = flags_of(i);
    for (int i = 0; i < 10; i++) {
        int ok = v[i] != v[10];
        for (int j = 0; j < i && ok; j++) if (v[i] == v[j]) ok = 0;
        if (!ok) { printf("scsig FAIL %s\n", names[i]); return 1; }
    }
    puts("scsig ok 10/10");
    return 0;
}
