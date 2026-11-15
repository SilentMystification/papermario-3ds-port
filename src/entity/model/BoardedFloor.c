#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/BoardedFloor_1.vtx.inc.c"
#include "entity/model/BoardedFloor_2.vtx.inc.c"
#include "entity/model/BoardedFloor_3.vtx.inc.c"
#include "entity/model/BoardedFloor_4.vtx.inc.c"
#include "entity/model/BoardedFloor_5.vtx.inc.c"
#include "entity/model/BoardedFloor_6.vtx.inc.c"
#include "entity/model/BoardedFloor_7.vtx.inc.c"
#include "entity/model/BoardedFloor_8.vtx.inc.c"
#include "entity/model/BoardedFloor_9.vtx.inc.c"
#include "entity/model/BoardedFloor_A.vtx.inc.c"
#include "entity/model/BoardedFloor_B.vtx.inc.c"
#include "entity/model/BoardedFloor_C.vtx.inc.c"

INCLUDE_IMG("entity/model/BoardedFloor.png", D_0A001400_E4FBF0);
NDN_DECL(D_0A002400_E50BF0);

Mtx Entity_BoardedFloor_FragmentMatrices[] = {
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0xFFF30002, 0xFFF30001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0xD5558000, 0x8E390000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0x00010002, 0x00080001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0xD5558000, 0x8E390000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0xFFF50001, 0x00020001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0xE65D8000, 0x046B0000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0x000C0001, 0xFFF40001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0x65928000, 0x7ED60000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0x000FFFFF, 0x00190001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0x00008000, 0x80000000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0xFFF1FFFF, 0x00190001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0x00008000, 0x80000000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0x000BFFFF, 0x000A0001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0x40008000, 0x80000000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0xFFEEFFFF, 0x000A0001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0x40008000, 0x80000000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0xFFEFFFFF, 0xFFF80001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0x80008000, 0x00000000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0x000CFFFF, 0xFFF80001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0x80008000, 0x00000000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0xFFECFFFF, 0xFFEC0001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0x00008000, 0x55550000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0x0007FFFF, 0xFFE90001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0x80008000, 0x00000000 }
        }
    },
};

NDN_DECL(D_0A002778_E50F68);

NDN_DECL(D_0A0027E8_E50FD8);

NDN_DECL(D_0A0027F8_E50FE8);

NDN_DECL(D_0A002858_E51048);

NDN_DECL(D_0A002868_E51058);

NDN_DECL(D_0A0028D8_E510C8);

NDN_DECL(D_0A0028E8_E510D8);

NDN_DECL(D_0A002958_E51148);

NDN_DECL(D_0A002968_E51158);

NDN_DECL(D_0A0029F0_E511E0);

NDN_DECL(D_0A002A00_E511F0);

NDN_DECL(D_0A002A88_E51278);

NDN_DECL(D_0A002A98_E51288);

NDN_DECL(D_0A002B08_E512F8);

NDN_DECL(D_0A002B18_E51308);

NDN_DECL(D_0A002B88_E51378);

NDN_DECL(D_0A002B98_E51388);

NDN_DECL(D_0A002C28_E51418);

NDN_DECL(D_0A002C38_E51428);

NDN_DECL(D_0A002CC8_E514B8);

NDN_DECL(D_0A002CD8_E514C8);

NDN_DECL(D_0A002D68_E51558);

NDN_DECL(D_0A002D78_E51568);

NDN_DECL(D_0A002E08_E515F8);

Gfx* Entity_BoardedFloor_FragmentsRender[] = {
    NDN(D_0A002E08_E515F8),
    NDN(D_0A002D68_E51558),
    NDN(D_0A002CC8_E514B8),
    NDN(D_0A002C28_E51418),
    NDN(D_0A002B88_E51378),
    NDN(D_0A002B08_E512F8),
    NDN(D_0A002A88_E51278),
    NDN(D_0A0029F0_E511E0),
    NDN(D_0A002958_E51148),
    NDN(D_0A0028D8_E510C8),
    NDN(D_0A002858_E51048),
    NDN(D_0A0027E8_E50FD8),
};
