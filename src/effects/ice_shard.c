#include "common.h"
#include "effects_internal.h"
#include "nd_gtab.h"
ND_PSO_DESC(pe_ice_shard_176, 0xfcffc7ff, 0xff8dfeff, 0x1e182cf0, 0x0c184b50, 0x00200005, 0x00000000, 0x00000000, 0x0000007f, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0);
ND_PSO_DESC(pe_ice_shard_169, 0xfc262a60, 0x150c937f, 0x1e182cf0, 0x0c184b50, 0x00200005, 0x00000000, 0x00000000, 0x0000007f, 1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
#include "nd_val.h"
#include "nd_xf.h"
#include "nd_st.h"

NDN_DECL(D_09001000_3F9CC0);
NDN_DECL(D_09001148_3F9E08);
NDN_DECL(D_09001168_3F9E28);

Gfx* D_E01108B0[] = { NDN(D_09001148_3F9E08) };
Gfx* D_E01108B4[] = { NDN(D_09001000_3F9CC0) };

void ice_shard_init(EffectInstance* effect);
void ice_shard_update(EffectInstance* effect);
void ice_shard_render(EffectInstance* effect);
void ice_shard_appendGfx(void* effect);

EffectInstance* ice_shard_main(
    s32 type,
    f32 posX,
    f32 posY,
    f32 posZ,
    f32 scale,
    s32 duration
) {
    EffectBlueprint bp;
    EffectInstance* effect;
    IceShardFXData* data;
    s32 numParts = 1;

    bp.init = ice_shard_init;
    bp.update = ice_shard_update;
    bp.renderScene = ice_shard_render;
    bp.unk_00 = 0;
    bp.renderUI = nullptr;
    bp.effectID = EFFECT_ICE_SHARD;

    effect = create_effect_instance(&bp);
    effect->numParts = numParts;
    data = effect->data.iceShard = general_heap_malloc(numParts * sizeof(*data));
    ASSERT(effect->data.iceShard != nullptr);

    data->type = type;
    data->lifetime = 0;
    if (duration <= 0) {
        data->timeLeft = 1000;
    } else {
        data->timeLeft = duration;
    }
    data->primCol.a = 255;
    data->pos.x = posX;
    data->pos.y = posY;
    data->pos.z = posZ;
    data->scale = scale;
    data->primCol.r = 255;
    data->primCol.g = 255;
    data->primCol.b = 255;
    data->envCol.r = 255;
    data->envCol.g = 255;
    data->envCol.b = 255;
    data->envCol.a = 255;
    data->animFrame = 0;
    data->animRate = (rand_int(1) * 2 - 1) * 0.25 * (rand_int(4) * 0.1 + 0.1);
    data->rot = rand_int(359);
    data->vel.x = rand_int(10) - 5;
    data->vel.y = rand_int(10) - 5;
    data->vel.z = rand_int(10) - 5;
    data->angularVel = rand_int(40) - 20;
    data->gravAccel = -0.1f;

    return effect;
}

void ice_shard_init(EffectInstance* effect) {
}

void ice_shard_update(EffectInstance* effect) {
    IceShardFXData* data = effect->data.iceShard;
    s32 type = data->type;

    if (effect->flags & FX_INSTANCE_FLAG_DISMISS) {
        effect->flags &= ~FX_INSTANCE_FLAG_DISMISS;
        data->timeLeft = 16;
    }

    if (data->timeLeft < 1000) {
        data->timeLeft--;
    }

    data->lifetime++;

    if (data->timeLeft < 0) {
        remove_effect(effect);
        return;
    }

    if (data->timeLeft < 16) {
        data->primCol.a = data->timeLeft * 16;
    }

    data->animFrame += data->animRate;

    if (data->animFrame < 0.0f) {
        data->animFrame += 1.0f;
    }
    if (data->animFrame > 1.0f) {
        data->animFrame -= 1.0f;
    }

    data->vel.y += data->gravAccel;
    data->pos.x += data->vel.x;
    data->pos.y += data->vel.y;
    data->pos.z += data->vel.z;
    data->rot += data->angularVel;

    if (type >= 2 && data->pos.y < 0.0f && data->vel.y < 0.0f) {
        data->pos.y = 0.0f;
        data->vel.y *= -0.7;
    }
}

void ice_shard_render(EffectInstance* effect) {
    RenderTask renderTask;
    RenderTask* retTask;

    renderTask.appendGfx = ice_shard_appendGfx;
    renderTask.appendGfxArg = effect;
    renderTask.dist = 10;
    renderTask.renderMode = RENDER_MODE_CLOUD_NO_ZCMP;

    retTask = queue_render_task(&renderTask);
    retTask->renderMode |= RENDER_TASK_FLAG_REFLECT_FLOOR;
}

void ice_shard_appendGfx(void* effect) {
    IceShardFXData* data = ((EffectInstance*)effect)->data.iceShard;
    s32 alpha = data->primCol.a;
    s32 type = data->type;
    s32 texCoordS;
    s32 texCoordT;
    s32 envAlpha;
    Matrix4f sp20;
    Matrix4f unused;

    nd_val_seg(0x09, VIRTUAL_TO_PHYSICAL(((EffectInstance*)effect)->shared->graphics));

    guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->scale, data->pos.x, data->pos.y, data->pos.z);
    guMtxF2L(sp20, &gDisplayContext->matrixStack[gMatrixListPos]);

    nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    guRotateF(sp20, data->rot, 0.0f, 0.0f, 1.0f);
    guMtxF2L(sp20, &gDisplayContext->matrixStack[gMatrixListPos]);

    nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
    nd_draw_list(D_E01108B4[0], ND_SELF);

    texCoordS = (type % 2) * 32;
    texCoordT = (s32) (data->animFrame * 4.0f) * 32;
    envAlpha = (data->animFrame * 4.0f - (s32) data->animFrame * 4) * 256.0f;

    nd_val_tile_size(G_TX_RENDERTILE, (texCoordS     ) * 4, (texCoordT     ) * 4, (texCoordS + 31) * 4, (texCoordT + 31) * 4);
    nd_val_tile_size(1, (texCoordS     ) * 4, (texCoordT + 32) * 4, (texCoordS + 31) * 4, (texCoordT + 63) * 4);

    nd_val_prim(0, 0, data->primCol.r, data->primCol.g, data->primCol.b, alpha);
    nd_val_env(data->envCol.r, data->envCol.g, data->envCol.b, envAlpha);
    nd_draw_list(D_E01108B0[0], pe_ice_shard_169);
    nd_xf_pop(G_MTX_MODELVIEW);

    if (type % 2 == 1 && (s32) (data->animFrame * 4.0f) == 3) {
        nd_val_prim(0, 0, data->primCol.r, data->primCol.g, data->primCol.b, alpha * envAlpha / 255);
        nd_pso_cc(PM_CC_3C, PM_CC_3C);
        nd_draw_list(NDN(D_09001168_3F9E28), pe_ice_shard_176);
    }

    nd_xf_pop(G_MTX_MODELVIEW);
}
