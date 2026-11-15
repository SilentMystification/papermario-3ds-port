#include "common.h"
#include "effects_internal.h"
#include "nd_gtab.h"
ND_PSO_DESC(pe_stat_change_274_k0, 0xfc127e24, 0xfffff3f9, 0x1e082cf0, 0x00551208, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
ND_PSO_DESC(pe_stat_change_274_k1, 0xfc127e24, 0xfffff3f9, 0x1e082cf0, 0x00551208, 0x00000000, 0x00000000, 0x00000000, 0x0000007f, 1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
ND_PSO_DESC(pe_stat_change_274_k2, 0xfc127e24, 0xfffff3f9, 0x1e08acf0, 0x00551208, 0x00000000, 0x00000000, 0x00000000, 0x0000007f, 1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
ND_PSO_DESC(pe_stat_change_274_k3, 0xfc123224, 0xff67ffff, 0x1e082cf0, 0x00504340, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
ND_PSO_DESC(pe_stat_change_274_k4, 0xfc123224, 0xff67ffff, 0x1e082cf0, 0x00504340, 0x00000000, 0x00000000, 0x00000000, 0x0000007f, 1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
ND_PSO_DESC(pe_stat_change_274_k5, 0xfc123224, 0xff67ffff, 0x1e08acf0, 0x00504340, 0x00000000, 0x00000000, 0x00000000, 0x0000007f, 1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
ND_PSO_DESC(pe_stat_change_288, 0xfc123224, 0xff67ffff, 0x1e082cf0, 0x00504340, 0x00200005, 0x00000000, 0x00000000, 0x00000000, 1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
ND_PSO_DESC(pe_stat_change_296, 0xfc123224, 0xff67ffff, 0x1e082cf0, 0x00504340, 0x00200005, 0x00000000, 0x00000000, 0x00000000, 1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
#include "nd_val.h"
#include "nd_xf.h"
#include "nd_st.h"

enum ArrowType {
    ARROW_TYPE_ATK_UP = 0,
    ARROW_TYPE_ATK_DOWN = 1,
    ARROW_TYPE_DEF_UP = 2,
};

typedef struct ExtraArrowDataEntry {
    u8 unk_00;
    u8 unk_01;
} ExtraArrowDataEntry;

typedef struct ArrowDataTableEntry {
    s32 type;
    s32 value;
} ArrowDataTableEntry;

NDN_DECL(D_09002700_3B1E00);
NDN_DECL(D_09002798_3B1E98);
NDN_DECL(D_09002860_3B1F60);
NDN_DECL(D_09002880_3B1F80);
NDN_DECL(D_090028A0_3B1FA0);
NDN_DECL(D_090028C0_3B1FC0);
NDN_DECL(D_090028E0_3B1FE0);
NDN_DECL(D_09002950_3B2050);
NDN_DECL(D_09002990_3B2090);
NDN_DECL(D_090029D0_3B20D0);
NDN_DECL(D_09002A10_3B2110);
NDN_DECL(D_09002A50_3B2150);
NDN_DECL(D_09002A90_3B2190);
NDN_DECL(D_09002AD0_3B21D0);
NDN_DECL(D_09002B10_3B2210);
NDN_DECL(D_09002B50_3B2250);
NDN_DECL(D_09002B90_3B2290);
NDN_DECL(D_09002BD0_3B22D0);
NDN_DECL(D_09002C10_3B2310);

Gfx* D_E00AC7B0[] = { NDN(D_09002700_3B1E00), NDN(D_09002700_3B1E00), NDN(D_09002798_3B1E98) };

Gfx* D_E00AC7BC[] = { NDN(D_09002860_3B1F60), NDN(D_09002880_3B1F80), NDN(D_09002860_3B1F60) };

Gfx* D_E00AC7C8[] = {
    NDN(D_09002950_3B2050), NDN(D_09002990_3B2090), NDN(D_090029D0_3B20D0), NDN(D_09002A10_3B2110),
    NDN(D_09002A50_3B2150), NDN(D_09002A90_3B2190), NDN(D_09002AD0_3B21D0), NDN(D_09002B10_3B2210),
    NDN(D_09002B50_3B2250), NDN(D_09002B90_3B2290), NDN(D_09002BD0_3B22D0), NDN(D_09002C10_3B2310)
};

ExtraArrowDataEntry D_E00AC7F8[] = {
    { .unk_00 = 0xFF, .unk_01 = 0x96 },
    { .unk_00 = 0xFF, .unk_01 = 0x82 },
    { .unk_00 = 0xFF, .unk_01 = 0x64 },
    { .unk_00 = 0xFF, .unk_01 = 0x46 },
    { .unk_00 = 0xFF, .unk_01 = 0x3C },
    { .unk_00 = 0xFF, .unk_01 = 0x50 },
    { .unk_00 = 0xFF, .unk_01 = 0x6E },
    { .unk_00 = 0xFF, .unk_01 = 0x73 },
    { .unk_00 = 0xFF, .unk_01 = 0x6B },
    { .unk_00 = 0xFF, .unk_01 = 0x5C },
    { .unk_00 = 0xFF, .unk_01 = 0x5A },
    { .unk_00 = 0xFF, .unk_01 = 0x5E },
    { .unk_00 = 0xFF, .unk_01 = 0x67 },
    { .unk_00 = 0xFF, .unk_01 = 0x68 },
    { .unk_00 = 0xFF, .unk_01 = 0x67 },
    { .unk_00 = 0xFF, .unk_01 = 0x64 },
    { .unk_00 = 0xFF, .unk_01 = 0x62 },
    { .unk_00 = 0xFF, .unk_01 = 0x63 },
    { .unk_00 = 0xFF, .unk_01 = 0x65 },
    { .unk_00 = 0xFF, .unk_01 = 0x64 },
    { .unk_00 = 0xF0, .unk_01 = 0x64 },
    { .unk_00 = 0xDC, .unk_01 = 0x64 },
    { .unk_00 = 0xC8, .unk_01 = 0x64 },
    { .unk_00 = 0xB4, .unk_01 = 0x64 },
    { .unk_00 = 0xA0, .unk_01 = 0x64 },
    { .unk_00 = 0x82, .unk_01 = 0x64 },
    { .unk_00 = 0x64, .unk_01 = 0x64 },
    { .unk_00 = 0x46, .unk_01 = 0x64 },
    { .unk_00 = 0x32, .unk_01 = 0x64 },
    { .unk_00 = 0x28, .unk_01 = 0x64 },
    { .unk_00 = 0x1E, .unk_01 = 0x64 },
    { .unk_00 = 0x14, .unk_01 = 0x64 },
    { .unk_00 = 0x0A, .unk_01 = 0x64 },
};

ExtraArrowDataEntry D_E00AC83C[] = {
    { .unk_00 = 0xFF, .unk_01 = 0x96 },
    { .unk_00 = 0xFF, .unk_01 = 0x82 },
    { .unk_00 = 0xFF, .unk_01 = 0x64 },
    { .unk_00 = 0xFF, .unk_01 = 0x46 },
    { .unk_00 = 0xFF, .unk_01 = 0x3C },
    { .unk_00 = 0xFF, .unk_01 = 0x50 },
    { .unk_00 = 0xFF, .unk_01 = 0x6E },
    { .unk_00 = 0xFF, .unk_01 = 0x73 },
    { .unk_00 = 0xFF, .unk_01 = 0x6B },
    { .unk_00 = 0xFF, .unk_01 = 0x5C },
    { .unk_00 = 0xFF, .unk_01 = 0x5A },
    { .unk_00 = 0xFF, .unk_01 = 0x5E },
    { .unk_00 = 0xFF, .unk_01 = 0x67 },
    { .unk_00 = 0xFF, .unk_01 = 0x68 },
    { .unk_00 = 0xFF, .unk_01 = 0x67 },
    { .unk_00 = 0xFF, .unk_01 = 0x64 },
    { .unk_00 = 0xFF, .unk_01 = 0x62 },
    { .unk_00 = 0xFF, .unk_01 = 0x63 },
    { .unk_00 = 0xFF, .unk_01 = 0x65 },
    { .unk_00 = 0xFF, .unk_01 = 0x64 },
    { .unk_00 = 0xF0, .unk_01 = 0x64 },
    { .unk_00 = 0xDC, .unk_01 = 0x64 },
    { .unk_00 = 0xC8, .unk_01 = 0x64 },
    { .unk_00 = 0xB4, .unk_01 = 0x64 },
    { .unk_00 = 0xA0, .unk_01 = 0x64 },
    { .unk_00 = 0x82, .unk_01 = 0x64 },
    { .unk_00 = 0x64, .unk_01 = 0x64 },
    { .unk_00 = 0x46, .unk_01 = 0x64 },
    { .unk_00 = 0x32, .unk_01 = 0x64 },
    { .unk_00 = 0x28, .unk_01 = 0x64 },
    { .unk_00 = 0x1E, .unk_01 = 0x64 },
    { .unk_00 = 0x14, .unk_01 = 0x64 },
    { .unk_00 = 0x0A, .unk_01 = 0x64 },
};

ArrowDataTableEntry D_E00AC880[] = {
    { .type = ARROW_TYPE_ATK_UP, .value = 1 },
    { .type = ARROW_TYPE_ATK_UP, .value = 2 },
    { .type = ARROW_TYPE_ATK_UP, .value = 3 },
    { .type = ARROW_TYPE_ATK_DOWN, .value = -1 },
    { .type = ARROW_TYPE_ATK_DOWN, .value = -2 },
    { .type = ARROW_TYPE_ATK_DOWN, .value = -3 },
    { .type = ARROW_TYPE_DEF_UP, .value = 1 },
    { .type = ARROW_TYPE_DEF_UP, .value = 2 },
    { .type = ARROW_TYPE_DEF_UP, .value = 3 },
    { .type = ARROW_TYPE_ATK_UP, .value = 0 },
};

void func_E00AC288(EffectInstance* effect);
void func_E00AC2A4(EffectInstance* effect);
void stat_change_init(EffectInstance* effect);
void stat_change_update(EffectInstance* effect);
void stat_change_render(EffectInstance* effect);

EffectInstance* stat_change_main(s32 arg0, f32 x, f32 y, f32 z, f32 scale, s32 time) {
    EffectBlueprint bp;
    EffectBlueprint* bpPtr = &bp;
    EffectInstance* effect;
    StatChangeFXData* part;
    s32 numParts = 1;

    bp.init = stat_change_init;
    bp.update = stat_change_update;
    bp.renderScene = stat_change_render;
    bp.renderUI = func_E00AC288;
    bp.unk_00 = 0;
    bp.effectID = EFFECT_STAT_CHANGE;

    effect = create_effect_instance(bpPtr);
    effect->numParts = numParts;
    part = effect->data.statChange = general_heap_malloc(numParts * sizeof(*part));

    ASSERT(effect->data.statChange != nullptr);

    part->unk_00 = arg0;
    part->pos.x = x;
    part->pos.y = y;
    part->pos.z = z;
    part->timeLeft = time;

    part->unk_18 = 0;
    part->scale = scale;
    part->unk_24 = 255;
    part->scaleX = 0;
    part->scaleY = 0;
    part->unk_30 = 0;
    part->unk_38 = 0;
    part->unk_34 = 0;

    part->arrowType = D_E00AC880[arg0].type;
    part->arrowValue = D_E00AC880[arg0].value;

    return effect;
}

void stat_change_init(EffectInstance* effect) {
}

void stat_change_update(EffectInstance* effect) {
    StatChangeFXData* part = effect->data.statChange;
    s32 old_unk18;

    part->timeLeft--;
    old_unk18 = part->unk_18++;

    if (part->timeLeft < 0) {
        remove_effect(effect);
        return;
    }

    if (part->arrowType == ARROW_TYPE_ATK_DOWN) {
        if (old_unk18 >= ARRAY_COUNT(D_E00AC83C)) {
            part->unk_24 = 0;
            part->scaleX = 0.0f;
            part->scaleY = 0.0f;
        } else {
            part->unk_24 = D_E00AC83C[old_unk18].unk_00;
            part->scaleY = (f32)D_E00AC83C[old_unk18].unk_01 * 0.01;
            part->scaleX = 1.0f - part->scaleY + 1.0f;
        }
    } else {
        if (old_unk18 >= ARRAY_COUNT(D_E00AC7F8)) {
            part->unk_24 = 0;
            part->scaleX = 0.0f;
            part->scaleY = 0.0f;
        } else {
            part->unk_24 = D_E00AC7F8[old_unk18].unk_00;
            part->scaleX = (f32)D_E00AC7F8[old_unk18].unk_01 * 0.01;
            part->scaleY = 1.0f - part->scaleX + 1.0f;
        }
    }

    part->unk_3C = 0;
    part->unk_40 = 0;

    do {} while (0); // required to match

    part->pos.x += part->unk_30;
    part->pos.y += part->unk_38;
    part->pos.z += part->unk_34;
}

void stat_change_render(EffectInstance* effect) {
}

void func_E00AC288(EffectInstance* effect) {
    func_E00AC2A4(effect);
}

void func_E00AC2A4(EffectInstance* effect) {
    StatChangeFXData* data = effect->data.statChange;
    s32 arrowType = data->arrowType;
    s32 arrowValue = data->arrowValue;
    s32 idx;
    Matrix4f sp20;
    Matrix4f sp60;

    if (data->scaleX != 0.0f && data->scaleY != 0.0f) {
        nd_val_seg(0x09, VIRTUAL_TO_PHYSICAL(((EffectInstance*)effect)->shared->graphics));

        guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->scale, data->pos.x, data->pos.y, data->pos.z);
        guMtxF2L(sp20, &gDisplayContext->matrixStack[gMatrixListPos]);

        nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);

        if (data->unk_24 == 255) {
            nd_pso_rm(AA_EN | CVG_DST_FULL | ZMODE_OPA | CVG_X_ALPHA | GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM), AA_EN | CVG_DST_FULL | ZMODE_OPA | CVG_X_ALPHA | GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM));
            nd_pso_cc(G_CC_MODULATEIDECALA, G_CC_MODULATEIDECALA);
        } else {
            nd_pso_rm(G_RM_CLD_SURF, G_RM_CLD_SURF2);
            nd_pso_cc(PM_CC_49, PM_CC_49);
        }

        guTranslateF(sp20, 0.0f, data->unk_40, 0.0f);
        guScaleF(sp60, data->scaleX, data->scaleY, 1.0f);
        guMtxCatF(sp60, sp20, sp20);
        guMtxF2L(sp20, &gDisplayContext->matrixStack[gMatrixListPos]);

        nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        nd_draw_list(D_E00AC7B0[arrowType], ND_SELF);
        nd_val_prim(0, 0, 0, 0, 0, data->unk_24);
        switch ((data->unk_24 != 255) * 3 + arrowType) {
            case 0: nd_draw_list(D_E00AC7BC[arrowType], pe_stat_change_274_k0); break;
            case 1: nd_draw_list(D_E00AC7BC[arrowType], pe_stat_change_274_k1); break;
            case 2: nd_draw_list(D_E00AC7BC[arrowType], pe_stat_change_274_k2); break;
            case 3: nd_draw_list(D_E00AC7BC[arrowType], pe_stat_change_274_k3); break;
            case 4: nd_draw_list(D_E00AC7BC[arrowType], pe_stat_change_274_k4); break;
            default: nd_draw_list(D_E00AC7BC[arrowType], pe_stat_change_274_k5); break;
        }
        nd_xf_pop(G_MTX_MODELVIEW);
        nd_draw_list(NDN(D_090028E0_3B1FE0), ND_SELF);

        guTranslateF(sp20, 0.0f, data->unk_3C, 0.0f);
        guMtxF2L(sp20, &gDisplayContext->matrixStack[gMatrixListPos]);

        nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);

        if (arrowValue < 0) {
            idx = -arrowValue;
        } else {
            idx = arrowValue;
        }

        nd_draw_list(D_E00AC7C8[idx % 10], ND_SELF);
        nd_draw_list(NDN(D_090028C0_3B1FC0), pe_stat_change_288);

        if (arrowValue >= 0) {
            nd_draw_list(D_E00AC7C8[11], ND_SELF);
        } else {
            nd_draw_list(D_E00AC7C8[10], ND_SELF);
        }

        nd_draw_list(NDN(D_090028A0_3B1FA0), pe_stat_change_296);
        nd_xf_pop(G_MTX_MODELVIEW);
        nd_xf_pop(G_MTX_MODELVIEW);
    }
}
