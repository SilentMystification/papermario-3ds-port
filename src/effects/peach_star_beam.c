#include "common.h"
#include "effects_internal.h"
#include "pm_interp_fi.h"
#include "nd_gtab.h"
ND_PSO_DESC(pe_peach_star_beam_179, 0xfc121824, 0xff33ffff, 0x1e082cf0, 0x005041d8, 0x00200405, 0x00000000, 0x00000000, 0x0000007f, 1, 0, 1, 0, 0, 0, 1, 1, 2, 0, 0, 0);
ND_PSO_DESC(pe_peach_star_beam_203, 0xfc367fff, 0x5ffcf238, 0x1e18acf0, 0x0c191238, 0x00000005, 0x00000000, 0x00000000, 0x0000007f, 1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
ND_PSO_DESC(pe_peach_star_beam_221, 0xfc3217ff, 0x5ffefe38, 0x1e182cf0, 0x0c1845d8, 0x00000005, 0x00000000, 0x00000000, 0x0000007f, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0);
ND_PSO_DESC(pe_peach_star_beam_224, 0xfcffc7ff, 0xfffefe38, 0x1e182cf0, 0x0c184b50, 0x00200405, 0x00000000, 0x00000000, 0x0000007f, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0);
ND_PSO_DESC(pe_peach_star_beam_249, 0xfc367fff, 0x5ffcf238, 0x1e18acf0, 0x0c191238, 0x00000005, 0x00000000, 0x00000000, 0x0000007f, 1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
#include "nd_val.h"
#include "nd_xf.h"

NDN_DECL(D_09004FE8_3EAF18);
extern Gfx D_09005090_3EAFC0[];
extern Gfx D_090050F0_3EB020[];
NDN_DECL(D_09005168_3EB098);
NDN_DECL(D_090051E0_3EB110);
NDN_DECL(D_09005258_3EB188);
NDN_DECL(D_090052D0_3EB200);
NDN_DECL(D_09005348_3EB278);
NDN_DECL(D_090053C0_3EB2F0);
NDN_DECL(D_09005438_3EB368);
NDN_DECL(D_090054B0_3EB3E0);
NDN_DECL(D_09005570_3EB4A0);
NDN_DECL(D_09005590_3EB4C0);

NdLst D_E010CA10[] = { NDN(D_090054B0_3EB3E0) };

NdLst D_E010CA14[] = { NDN(D_09004FE8_3EAF18) };

NdLst D_E010CA18[8] = {
    NDN(D_09005590_3EB4C0), NDN(D_09005570_3EB4A0), NDN(D_09005570_3EB4A0), NDN(D_09005570_3EB4A0),
    NDN(D_09005570_3EB4A0), NDN(D_09005570_3EB4A0), NDN(D_09005570_3EB4A0), NDN(D_09005570_3EB4A0)
};

NdLst D_E010CA38[8] = {
    D_090050F0_3EB020, NDN(D_09005168_3EB098), NDN(D_090051E0_3EB110), NDN(D_09005258_3EB188),
    NDN(D_090052D0_3EB200), NDN(D_09005348_3EB278), NDN(D_090053C0_3EB2F0), NDN(D_09005438_3EB368)
};

void peach_star_beam_init(EffectInstance* effect);
void peach_star_beam_update(EffectInstance* effect);
void peach_star_beam_render(EffectInstance* effect);
void peach_star_beam_appendGfx(void* effect);

EffectInstance* peach_star_beam_main(s32 type, f32 x, f32 y, f32 z, f32 arg4, s32 duration) {
    EffectBlueprint bp;
    EffectInstance* effect;
    PeachStarBeamFXData* data;
    s32 numParts = 1;
    s32 i;

    bp.unk_00 = 0;
    bp.init = peach_star_beam_init;
    bp.update = peach_star_beam_update;
    bp.renderScene = peach_star_beam_render;
    bp.renderUI = nullptr;
    bp.effectID = EFFECT_PEACH_STAR_BEAM;

    effect = create_effect_instance(&bp);
    effect->numParts = numParts;
    data = effect->data.peachStarBeam = general_heap_malloc(sizeof(*data));
    ASSERT(effect->data.peachStarBeam != nullptr);

    data->type = type;
    data->lifetime = 0;
    if (duration <= 0) {
        data->timeLeft = 1000;
    } else {
        data->timeLeft = duration;
    }
    data->beamAlpha = 224;
    data->pos.x = x;
    data->pos.y = y;
    data->pos.z = z;
    data->unk_3C = 255;
    data->beamScale = arg4;
    data->circleRadius = 80.0f;
    data->twinkYOffset = 50.0f;
    data->primR = 255;
    data->primG = 255;
    data->primB = 120;
    data->envR = 255;
    data->envG = 255;
    data->envB = 255;
    data->envA = 255;
    data->rotSpeed = 5.0f;
    data->rotAngle = 0;
    data->circleCenter.x = x;
    data->circleCenter.y = y + 50.0f;
    data->circleCenter.z = z;

    for (i = 0; i < ARRAY_COUNT(data->spirits); i++) {
        PeachStarBeamSpirit* part = &data->spirits[i];

        if (type == 0 && i == 0) {
            part->flags = 0;
        } else {
            part->flags = 1;
        }
    }
    return effect;
}

void peach_star_beam_init(EffectInstance* effect) {
}

void peach_star_beam_update(EffectInstance* effect) {
    PeachStarBeamFXData* data = effect->data.peachStarBeam;
    f32 rotationAngle;
    f32 radius;
    f32 centerX;
    f32 centerY;
    f32 centerZ;
    f32 spiritAngle;
    s32 i;

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

    data->rotAngle += data->rotSpeed;
    rotationAngle = data->rotAngle;
    radius = data->circleRadius;
    centerX = data->circleCenter.x;
    centerY = data->circleCenter.y;
    centerZ = data->circleCenter.z;

    for (i = 0; i < ARRAY_COUNT(data->spirits); i++) {
        PeachStarBeamSpirit* part = &data->spirits[i];

        if (i == 0) {
            part->pos.x = centerX;
            part->pos.y = centerY + data->twinkYOffset;
            part->pos.z = centerZ;
        } else {
            spiritAngle = rotationAngle + ((-360 + (360 * i)) / 7);
            part->pos.x = centerX + radius * sin_deg(spiritAngle);
            part->pos.y = centerY;
            part->pos.z = centerZ + radius * cos_deg(spiritAngle);
        }
        if (!(part->flags & 2)) {
            part->lockedPos.x = part->pos.x;
            part->lockedPos.y = part->pos.y;
            part->lockedPos.z = part->pos.z;
        }
    }
}

void peach_star_beam_render(EffectInstance* effect) {
    RenderTask renderTask;
    RenderTask* retTask;

    renderTask.appendGfx = peach_star_beam_appendGfx;
    renderTask.appendGfxArg = effect;
    renderTask.dist = 10;
    renderTask.renderMode = RENDER_MODE_CLOUD_NO_ZCMP;

    retTask = queue_render_task(&renderTask);
    retTask->renderMode |= RENDER_TASK_FLAG_REFLECT_FLOOR;
}

void peach_star_beam_appendGfx(void* effect) {
    Matrix4f sp20, sp60;
    Camera* camera = &gCameras[gCurrentCameraID];
    PeachStarBeamFXData* data = ((EffectInstance*)effect)->data.peachStarBeam;
    PeachStarBeamSpirit* part;
    s32 alpha = data->beamAlpha;
    f32 partX, partY, partZ;
    s32 i;

    nd_val_seg(0x9, VIRTUAL_TO_PHYSICAL(((EffectInstance*)effect)->shared->graphics));
    nd_draw_list(D_09005090_3EAFC0, pe_peach_star_beam_179);

    for (i = 0; i < ARRAY_COUNT(data->spirits); i++) {
        part = &data->spirits[i];
        if (part->flags & 1) {
            if (part->flags & 2) {
                partX = part->lockedPos.x;
                partY = part->lockedPos.y;
                partZ = part->lockedPos.z;
            } else {
                partX = part->pos.x;
                partY = part->pos.y;
                partZ = part->pos.z;
            }

            if (!(data->pos.z < partZ)) {
                FI_OPEN("star_beam_spirit", (uintptr_t) part);
                guPositionF(sp20, 0.0f, 0.0f, 0.0f, SPRITE_WORLD_SCALE_F, partX, partY, partZ);
                guMtxF2L(sp20, &gDisplayContext->matrixStack[gMatrixListPos]);

                nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                nd_val_prim(0, 0, 128, 128, 128, 255);
                nd_val_env(128, 128, 128, 127);
                nd_draw_list(D_E010CA38[i], ND_SELF);
                nd_draw_list(D_E010CA18[i], pe_peach_star_beam_203);
                nd_xf_pop(G_MTX_MODELVIEW);
                FI_CLOSE();
            }
        }
    }

    FI_OPEN("star_beam_beam", 0);
    guTranslateF(sp20, data->pos.x, data->pos.y - (((f32) (255 - data->unk_3C) * 400.0) / 255.0), data->pos.z);
    guScaleF(sp60, data->beamScale * 0.4, data->beamScale * 0.4, data->beamScale * 0.4);
    guMtxCatF(sp60, sp20, sp20);
    guMtxF2L(sp20, &gDisplayContext->matrixStack[gMatrixListPos]);

    nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    nd_xf_push(camera->mtxBillboard, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
    nd_val_prim(0, 0, data->primR, data->primG, data->primB, alpha);
    nd_val_env(data->envR, data->envG, data->envB, data->envA);
    nd_draw_list(D_E010CA14[0], ND_SELF);
    nd_draw_list(D_E010CA10[0], pe_peach_star_beam_221);
    nd_xf_pop(G_MTX_MODELVIEW);
    FI_CLOSE();
    nd_draw_list(D_09005090_3EAFC0, pe_peach_star_beam_224);

    for (i = 0; i < ARRAY_COUNT(data->spirits); i++) {
        part = &data->spirits[i];

        if (part->flags & 1) {
            if (part->flags & 2) {
                partX = part->lockedPos.x;
                partY = part->lockedPos.y;
                partZ = part->lockedPos.z;
            } else {
                partX = part->pos.x;
                partY = part->pos.y;
                partZ = part->pos.z;
            }

            if (!(partZ <= data->pos.z)) {
                FI_OPEN("star_beam_spirit", (uintptr_t) part);
                guPositionF(sp20, 0.0f, 0.0f, 0.0f, 0.67f, partX, partY, partZ);
                guMtxF2L(sp20, &gDisplayContext->matrixStack[gMatrixListPos]);

                nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                nd_val_prim(0, 0, 128, 128, 128, 255);
                nd_val_env(128, 128, 128, 127);
                nd_draw_list(D_E010CA38[i], ND_SELF);
                nd_draw_list(D_E010CA18[i], pe_peach_star_beam_249);
                nd_xf_pop(G_MTX_MODELVIEW);
                FI_CLOSE();
            }
        }
    }
}
