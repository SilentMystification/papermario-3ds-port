#include "common.h"
#include "nd_gtab.h"
#include "entity.h"
#include "include_asset.h"

extern IMG_BIN ui_battle_menu_spirits_png[];
extern PAL_BIN ui_battle_menu_spirits_pal[];
extern IMG_BIN ui_battle_solid_star_png[];
extern PAL_BIN ui_battle_solid_star_pal[];

INCLUDE_IMG("ui/battle/cursor_hand.png", ui_battle_cursor_hand_png);
INCLUDE_PAL("ui/battle/cursor_hand.pal", ui_battle_cursor_hand_pal);

Vtx BtlCursorModelVtx[] = {
    {{{ -22, -6, 0 }, 0, { 1024, 1024 }, { 0, 0, 0, 255 }}},
    {{{   9, -6, 0 }, 0, {    0, 1024 }, { 0, 0, 0, 255 }}},
    {{{   9, 25, 0 }, 0, {    0,    0 }, { 0, 0, 0, 255 }}},
    {{{ -22, 25, 0 }, 0, { 1024,    0 }, { 0, 0, 0, 255 }}},
};

NDN_DECL(BtlCursorModelGfx);

EntityModelScript EMS_CursorIcon = STANDARD_ENTITY_MODEL_SCRIPT(NDN(BtlCursorModelGfx), RENDER_MODE_ALPHATEST);

Vtx BtlBonkModelVtx[] = {
    {{{ -16, -16, 0 }, 0, {    0,    0 }, { 0, 0, 0, 255 }}},
    {{{  15, -16, 0 }, 0, { 1024,    0 }, { 0, 0, 0, 255 }}},
    {{{  15,  15, 0 }, 0, { 1024, 1024 }, { 0, 0, 0, 255 }}},
    {{{ -16,  15, 0 }, 0, {    0, 1024 }, { 0, 0, 0, 255 }}},
};

NDN_DECL(BtlBonkModelGfx);

EntityModelScript EMS_BonkIcon = STANDARD_ENTITY_MODEL_SCRIPT(NDN(BtlBonkModelGfx), RENDER_MODE_CLOUD_NO_ZCMP);

Vtx BtlStarModelVtx[] = {
    {{{ -16, -16, 0 }, 0, {    0, 1024 }, { 0, 0, 0, 255 }}},
    {{{  15, -16, 0 }, 0, { 1024, 1024 }, { 0, 0, 0, 255 }}},
    {{{  15,  15, 0 }, 0, { 1024,    0 }, { 0, 0, 0, 255 }}},
    {{{ -16,  15, 0 }, 0, {    0,    0 }, { 0, 0, 0, 255 }}},
};

NDN_DECL(BtlStarModelGfx);

EntityModelScript EMS_StarIcon = STANDARD_ENTITY_MODEL_SCRIPT(NDN(BtlStarModelGfx), RENDER_MODE_ALPHATEST);
