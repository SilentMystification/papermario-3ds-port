#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

INCLUDE_IMG("filemenu/copyarrow.png", D_8024A200);
INCLUDE_IMG("filemenu/corners_yellow.png", D_8024A400);
INCLUDE_IMG("filemenu/corners_gray.png", D_8024B400);

NDN_DECL(filemenu_dl_copyarrow);

Vtx D_8024B6B0[] = {
    {{{ -8, -16, 0 }, 0, { 0,    0 }, { 0, 0, 0, 0 }}},
    {{{ -8,  16, 0 }, 0, { 0, 2048 }, { 0, 0, 0, 0 }}},
};

Vtx D_8024B6D0[] = {
    {{{ 56,  16, 0 }, 0, { 4096, 2048 }, { 0, 0, 0, 255 }}},
    {{{ 56, -16, 0 }, 0, { 4096,    0 }, { 0, 0, 0, 255 }}},
};

Gfx D_8024B6F0[] = {
    gsSPVertex(D_8024B6B0, 2, 0),
    gsDPPipeSync(),
    gsSPEndDisplayList(),
};

Gfx D_8024B708[] = {
    gsSPVertex(D_8024B6D0, 2, 2),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsDPPipeSync(),
    gsSPEndDisplayList(),
};
