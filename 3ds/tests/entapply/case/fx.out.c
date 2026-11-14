#include "common.h"
#include "nd_gtab.h"
ND_PSO_DESC(pe_fx_5, 0xfcffb3ff, 0xff65feff, 0x1e080cf0, 0x00504340, 0x00000005, 0x00000000, 0x00000000, 0x00000000, 1, 0, 1, 0, 0, 0, 1, 0, 4, 0, 0, 0);
ND_PSO_DESC(pe_fx_9_k0, 0xfcffb3ff, 0xff65feff, 0x1e080cf0, 0x00504340, 0x00000005, 0x00000000, 0x00000000, 0x00000000, 1, 0, 1, 0, 0, 0, 1, 0, 4, 0, 0, 0);
ND_PSO_DESC(pe_fx_9_k1, 0xfcffb3ff, 0xff65feff, 0x1e080cf0, 0x00504340, 0x00000005, 0x00000000, 0x00000000, 0x00000000, 1, 0, 1, 0, 0, 0, 1, 0, 3, 0, 0, 0);

void fx_main(void) {
    nd_draw_list(D_X1, pe_fx_5);
    nd_rec_dl(D_X2);
    if (a) nd_rec_dl(D_X3);
    nd_rec_dl(D_X4);
    switch (fxk) {
        case 0: nd_draw_list(D_K1, pe_fx_9_k0); break;
        default: nd_draw_list(D_K1, pe_fx_9_k1); break;
    }
    nd_rec_dl_k(D_K2, fxk, 3);
    nd_draw_list(D_S1, ND_SELF);
}
