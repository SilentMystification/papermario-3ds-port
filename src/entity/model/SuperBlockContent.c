#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

INCLUDE_IMG("entity/model/SuperBlockContent.png", D_0A000000_E4A6A0);
INCLUDE_PAL("entity/model/SuperBlockContent.pal", D_0A000200_E4A8A0);
INCLUDE_PAL("entity/model/SuperBlockContent.pal2.pal", D_0A000220_E4A8C0);
INCLUDE_PAL("entity/model/SuperBlockContent.pal3.pal", D_0A000240_E4A8E0);
INCLUDE_PAL("entity/model/SuperBlockContent.pal4.pal", D_0A000260_E4A900);

Vtx Entity_SuperBlockContent_vtx[] = {
    {{{    13,    13,     0 }, 0, {     0,     0 }, { 255, 255, 255, 255 }}},
    {{{   -13,    13,     0 }, 0, {  1024,     0 }, { 255, 255, 255, 255 }}},
    {{{   -13,   -13,     0 }, 0, {  1024,  1024 }, { 255, 255, 255, 255 }}},
    {{{    13,   -13,     0 }, 0, {     0,  1024 }, { 255, 255, 255, 255 }}},
};

NDN_DECL(Entity_SuperBlockContent_Prepare);

NDN_DECL(Entity_SuperBlockContent_RenderImage);

NDN_DECL(Entity_SuperBlockContent_Render);

NDN_DECL(Entity_SuperBlockContent_Render2);
