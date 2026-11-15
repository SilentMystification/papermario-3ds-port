#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/BombableRock_1.vtx.inc.c"
#include "entity/model/BombableRock_2.vtx.inc.c"
#include "entity/model/BombableRock_3.vtx.inc.c"
#include "entity/model/BombableRock_4.vtx.inc.c"
#include "entity/model/BombableRock_5.vtx.inc.c"

INCLUDE_IMG("entity/model/BombableRock.png", D_0A000840_E9DE90);
INCLUDE_PAL("entity/model/BombableRock.pal", D_0A001040_E9E690);
NDN_DECL(D_0A001060_E9E6B0);

Mtx Entity_BombableRock_FragmentMatrices[] = {
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0x00000023, 0x00000001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0x000006D4, 0x00000000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0x00090012, 0x00100001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0xA924647E, 0xBB9A0000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0x00100012, 0xFFF70001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0xBB9A647E, 0x56DC0000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0xFFF00012, 0x00090001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0x4466647E, 0xA9240000 }
        }
    },
    {
        .m = {
            { 0x00010000, 0x00000000,
              0x00000001, 0x00000000 },
            { 0x00000000, 0x00010000,
              0xFFF70012, 0xFFF00001 },
            { 0x00000000, 0x00000000,
              0x00000000, 0x00000000 },
            { 0x00000000, 0x00000000,
              0x56DC647E, 0x44660000 }
        }
    },
};

NDN_DECL(D_0A001258_E9E8A8);

NDN_DECL(D_0A0012F8_E9E948);

NDN_DECL(D_0A001308_E9E958);

NDN_DECL(D_0A0013A8_E9E9F8);

NDN_DECL(D_0A0013B8_E9EA08);

NDN_DECL(D_0A001458_E9EAA8);

NDN_DECL(D_0A001468_E9EAB8);

NDN_DECL(D_0A001508_E9EB58);

NDN_DECL(D_0A001518_E9EB68);

NDN_DECL(D_0A001610_E9EC60);

NDN_DECL(D_0A001620_E9EC70);

NDN_DECL(D_0A001650_E9ECA0);

Gfx* Entity_BombableRock_FragmentsRender[] = {
    NDN(D_0A001610_E9EC60),
    NDN(D_0A001508_E9EB58),
    NDN(D_0A001458_E9EAA8),
    NDN(D_0A0013A8_E9E9F8),
    NDN(D_0A0012F8_E9E948),
};
