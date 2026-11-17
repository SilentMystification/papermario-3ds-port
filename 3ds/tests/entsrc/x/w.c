#include "nd_gtab.h"
ND_PSO_DESC(pe_w_7, 0x00000001, 0x00000002, 0x00000003, 0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0);
void f(Entity* entity) {
    Gfx* gfx = ENTITY_ADDR(entity, Gfx*, NDN(Ent_A));

    nd_draw_list(gfx, pe_w_7);
    nd_draw_list(NDN(N(card_gfx)), pe_w_7);
    nd_draw_list(ENTITY_ADDR(entity, NdLst, NDN(Ent_B)), pe_w_7);
}
