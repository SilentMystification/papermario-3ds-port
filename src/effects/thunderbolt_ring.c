#include "common.h"
#include "effects_internal.h"
#include "nd_gtab.h"
#include "nd_val.h"
#include "nd_xf.h"

extern Gfx D_09000000_3B46A0[];
extern Gfx D_090000C8_3B4768[];

void thunderbolt_ring_init(EffectInstance* effect);
void thunderbolt_ring_update(EffectInstance* effect);
void thunderbolt_ring_render(EffectInstance* effect);
void thunderbolt_ring_appendGfx(void* effect);

EffectInstance* thunderbolt_ring_main(s32 arg0, f32 posX, f32 posY, f32 posZ, f32 arg4, s32 lifeTime) {
    EffectBlueprint effectBp;
    EffectInstance* effect;
    ThunderboltRingFXData* data;
    s32 numParts = 1;

    effectBp.init = thunderbolt_ring_init;
    effectBp.update = thunderbolt_ring_update;
    effectBp.renderScene = thunderbolt_ring_render;
    effectBp.unk_00 = 0;
    effectBp.renderUI = nullptr;
    effectBp.effectID = EFFECT_THUNDERBOLT_RING;

    effect = create_effect_instance(&effectBp);
    effect->numParts = numParts;

    data = effect->data.thunderboltRing = general_heap_malloc(numParts * sizeof(*data));
    ASSERT(data != nullptr);

    data->unk_00 = arg0;
    data->lifeTime = 0;
    if (lifeTime <= 0) {
        data->timeLeft = 1000;
    } else {
        data->timeLeft = lifeTime;
    }
    data->unk_24 = 255;
    data->pos.x = posX;
    data->pos.y = posY;
    data->pos.z = posZ;
    data->unk_28 = arg4;
    data->unk_18 = 255;
    data->unk_1C = 230;
    data->unk_20 = 50;
    data->unk_30 = 0;
    data->unk_2C = 1.0f;

    return effect;
}

void thunderbolt_ring_init(EffectInstance* effect) {
}

void thunderbolt_ring_update(EffectInstance *effect) {
    ThunderboltRingFXData* data = effect->data.thunderboltRing;
    s32 lifeTime;
    s32 timeLeft;

    if (effect->flags & FX_INSTANCE_FLAG_DISMISS) {
        effect->flags &= ~FX_INSTANCE_FLAG_DISMISS;
        data->timeLeft = 16;
    }
    if (data->timeLeft < 1000) {
        data->timeLeft--;
    }

    timeLeft = data->timeLeft;
    data->lifeTime++;
    lifeTime = data->lifeTime;

    if (timeLeft < 0) {
        remove_effect(effect);
        return;
    }
    if (timeLeft < 8) {
        data->unk_2C += (8 - timeLeft) * 0.3;
    }
    if (lifeTime < 16) {
        data->unk_24 = (lifeTime * 16) + 15;
    }
    if (timeLeft < 8) {
        data->unk_24 = (timeLeft * 32) + 31;
    }
}

void thunderbolt_ring_render(EffectInstance* effect) {
    RenderTask renderTask;
    RenderTask* retTask;

    renderTask.appendGfx = thunderbolt_ring_appendGfx;
    renderTask.appendGfxArg = effect;
    renderTask.dist = 10;
    renderTask.renderMode = RENDER_MODE_CLOUD_NO_ZCMP;

    retTask = queue_render_task(&renderTask);
}

void thunderbolt_ring_appendGfx(void* effect) {
    ThunderboltRingFXData* data = ((EffectInstance*)effect)->data.thunderboltRing;
    s32 lifeTime = data->lifeTime;
    s32 unk_24 = data->unk_24;
    f32 scaleY = 1.0f;
    Gfx* savedGfxPos;
    Gfx* savedGfxPos2;
    NdSub sub;
    Matrix4f sp20;
    Matrix4f sp60;
    s32 i;

    nd_val_seg(0x09, VIRTUAL_TO_PHYSICAL(((EffectInstance*)effect)->shared->graphics));

    guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->unk_28, data->pos.x, data->pos.y, data->pos.z);
    guMtxF2L(sp20, &gDisplayContext->matrixStack[gMatrixListPos]);

    nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    nd_rec_dl(D_09000000_3B46A0);

    nd_sub_open(&sub); /* DIRECT D6c: the list for later is a stream range */

    for (i = 0; i < 5; i++) {
        guRotateF(sp20, i * 72 + 90, 0.0f, 0.0f, 1.0f);
        guScaleF(sp60, 1.0f, scaleY, 1.0f);
        guMtxCatF(sp60, sp20, sp20);
        guPositionF(sp60, 0.0f, 0.0f, 0.0f, 1.0f, 30.0f, 0.0f, 0.0f);
        guMtxCatF(sp60, sp20, sp20);
        guMtxF2L(sp20, &gDisplayContext->matrixStack[gMatrixListPos]);

        nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        nd_rec_dl(D_090000C8_3B4768);
        nd_xf_pop(G_MTX_MODELVIEW);
    }

    nd_sub_close(&sub);
    nd_val_prim(0, 0, data->unk_18, data->unk_1C, data->unk_20, unk_24);
    nd_val_env(255, 255, 255, 0);

    for (i = 0; i < data->unk_30 + 1; i++) {
        f32 angle = lifeTime * 8 + i * 180 / (data->unk_30 + 1);
        f32 temp = (f32) (i + 1) + cos_deg(angle) * 0.1;

        guPositionF(sp20, 0.0f, 0.0f, i * 36, temp * data->unk_2C, 0.0f, 0.0f, 0.0f);
        guMtxF2L(sp20, &gDisplayContext->matrixStack[gMatrixListPos]);

        nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        nd_sub_call(&sub);
        nd_xf_pop(G_MTX_MODELVIEW);
    }

    nd_xf_pop(G_MTX_MODELVIEW);
}
