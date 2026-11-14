/* selfapply fixture (ws_FXSELF): a.out.c and a.log are the expected output of selfapply.py on a.c with tab.txt. */
#include "nd_gtab.h"

static const Gfx* const D_ARR[] = { D_T_A, D_T_B, };
static const Gfx* const D_ARR2[] = { D_T_A, D_X_NOT2, };

void fx_arr(int idx) {
    nd_rec_dl(D_ARR[idx]);
}

void fx_arr2(int idx) {
    nd_rec_dl(D_ARR2[idx]);
}

void fx_plain(void) {
    nd_rec_dl(D_T_C); /* the table of the caller */
}

void fx_notab(void) {
    nd_rec_dl(D_X_NOT);
}

void fx_local(int k) {
    const Gfx* l = D_T_A;
    if (k) l = D_Y_NOT;
    nd_rec_dl(l);
}

void fx_field(Thing* it) {
    nd_rec_dl(it->displayList);
}

void fx_multi(void) {
    nd_rec_dl(
        D_T_A);
}

void fx_if(int k) {
    if (k) nd_rec_dl(D_T_B);
}

/* nd_rec_dl(D_T_A); the old form in a comment */
