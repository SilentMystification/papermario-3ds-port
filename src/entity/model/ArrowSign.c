#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

Vtx Entity_ArrowSign_vtxSign[] = {
    {{{   -10,   -10,     0 }, 0, {     0,     0 }, { 255, 255, 255, 255 }}},
    {{{    20,   -10,     0 }, 0, {  2048,     0 }, { 255, 255, 255, 255 }}},
    {{{    20,    10,     0 }, 0, {  2048,  1024 }, { 255, 255, 255, 255 }}},
    {{{   -10,    10,     0 }, 0, {     0,  1024 }, { 255, 255, 255, 255 }}},
};
Vtx Entity_ArrowSign_vtxPole[] = {
    {{{    -2,     0,    -1 }, 0, {     0,     0 }, {  25,  20,   0, 255 }}},
    {{{     1,     0,    -1 }, 0, {     0,     0 }, {  25,  20,   0, 255 }}},
    {{{     1,    19,    -1 }, 0, {     0,     0 }, {  25,  20,   0, 255 }}},
    {{{    -2,    19,    -1 }, 0, {     0,     0 }, {  25,  20,   0, 255 }}},
};

INCLUDE_IMG("entity/model/ArrowSign.png", D_0A000080_E59F80);
INCLUDE_PAL("entity/model/ArrowSign.pal", D_0A000880_E5A780);
NDN_DECL(Entity_ArrowSign_LoadTexture);

Mtx Entity_ArrowSign_mtxSign = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, -0.180848,
    0.000000, 1.000000, 0.000000, 20.000000,
    0.000000, 0.000000, 1.000000,  1.000000,
    0.000000, 0.000000, 0.000000,  1.000000
);

NDN_DECL(Entity_ArrowSign_RenderPole);

NDN_DECL(Entity_ArrowSign_RenderPole_);

NDN_DECL(Entity_ArrowSign_RenderRotatedSign);

NDN_DECL(Entity_ArrowSign_RenderSign);

NDN_DECL(Entity_ArrowSign_RenderPole__);

NDN_DECL(Entity_ArrowSign_Render);
