#include "common.h"
#include "nd_gtab.h"

void fx_main(void) {
    nd_rec_dl(D_X1);
    nd_rec_dl(D_X2);
    if (a) nd_rec_dl(D_X3);
    nd_rec_dl(D_X4);
    nd_rec_dl_k(D_K1, fxk, 2);
    nd_rec_dl_k(D_K2, fxk, 3);
    nd_rec_dl(D_S1);
}
