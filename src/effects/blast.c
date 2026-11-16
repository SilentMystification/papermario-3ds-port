#include "common.h"
#include "effects_internal.h"
#include "nd_gtab.h"
ND_PSO_DESC(pe_blast_131, 0xfc267ec0, 0x1334fed2, 0x1e183cf0, 0x0c184b50, 0x00000005, 0x00000000, 0x00000000, 0x00000000, 1, 0, 1, 0, 0, 0, 1, 0, 3, 0, 0, 0);
#include "nd_val.h"
#include "nd_xf.h"

void blast_init(EffectInstance* effect);
void blast_update(EffectInstance* effect);
void blast_render(EffectInstance* effect);
void blast_appendGfx(void* effect);

NDN_DECL(D_09001300_37ECD0);
NDN_DECL(D_09001378_37ED48);
NDN_DECL(D_090013F0_37EDC0);
NDN_DECL(D_09001468_37EE38);
NDN_DECL(D_090014E0_37EEB0);
NDN_DECL(D_09001558_37EF28);
NDN_DECL(D_090015D0_37EFA0);
NDN_DECL(D_09001648_37F018);
NDN_DECL(D_090016C0_37F090);
NDN_DECL(D_09001738_37F108);
NDN_DECL(D_090017B0_37F180);

NdLst D_E007C510[] = {
    NDN(D_09001378_37ED48), NDN(D_090013F0_37EDC0), NDN(D_09001468_37EE38),
    NDN(D_090014E0_37EEB0), NDN(D_09001558_37EF28), NDN(D_090015D0_37EFA0),
    NDN(D_09001648_37F018), NDN(D_090016C0_37F090), NDN(D_09001738_37F108)
};

void blast_main(s32 arg0, f32 arg1, f32 arg2, f32 arg3, f32 arg4, s32 arg5) {
    EffectBlueprint bp;
    EffectBlueprint* bpPtr = &bp;
    EffectInstance* effect;
    BlastFXData* part;
    s32 numParts = 1;
    s32 randInt;

    bp.init = blast_init;
    bp.update = blast_update;
    bp.renderScene = blast_render;
    bp.unk_00 = 0;
    bp.renderUI = nullptr;
    bp.effectID = EFFECT_BLAST;

    effect = create_effect_instance(bpPtr);
    effect->numParts = numParts;

    part = effect->data.blast = general_heap_malloc(numParts * sizeof(*part));

    ASSERT(effect->data.blast != nullptr);

    mem_clear(part, numParts * sizeof(*part));

    part->timeLeft = arg5;
    part->unk_18 = 0;
    part->unk_1C = arg5;
    part->unk_20 = 0;
    part->unk_00 = arg0;
    part->pos.x = arg1;
    part->pos.y = arg2;
    part->pos.z = arg3;
    part->unk_10 = arg4;
    part->unk_24 = 0;

    part->unk_28 = arg0 == 0 ? 0 : 40.0;
}

void blast_init(EffectInstance* effect) {
}

void blast_update(EffectInstance* effect) {
    BlastFXData* part = effect->data.blast;

    part->unk_20 = part->unk_18++ * 9.0f / part->unk_1C;
    part->timeLeft--;

    if (part->timeLeft < 0) {
        remove_effect(effect);
        return;
    }

    part->unk_24 += part->unk_28;
}

void blast_render(EffectInstance* effect) {
    RenderTask renderTask;
    RenderTask* retTask;

    renderTask.appendGfx = blast_appendGfx;
    renderTask.appendGfxArg = effect;
    renderTask.dist = 0;
    renderTask.renderMode = RENDER_MODE_PASS_THROUGH;

    retTask = queue_render_task(&renderTask);
    retTask->renderMode |= RENDER_TASK_FLAG_REFLECT_FLOOR;
}

void blast_appendGfx(void* effect) {
    Matrix4f sp18;
    Matrix4f sp58;
    Matrix4f sp98;
    Gfx* dlist = NDN(D_090017B0_37F180);
    BlastFXData* data = ((EffectInstance*) effect)->data.blast;
    s32 unk_20 = data->unk_20;
    f32 t = 256.0f;
    s32 envAlpha = (data->unk_20 - unk_20) * t;

    nd_val_seg(0x09, VIRTUAL_TO_PHYSICAL(((EffectInstance*)effect)->shared->graphics));
    nd_draw_list(NDN(D_09001300_37ECD0), ND_SELF);
    nd_draw_list(D_E007C510[unk_20], ND_SELF);

    guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);
    guRotateF(sp58, -gCameras[gCurrentCameraID].curYaw, 0.0f, 1.0f, 0.0f);
    guMtxCatF(sp58, sp18, sp98);
    guScaleF(sp18, data->unk_10, data->unk_10, 1.0f);
    guMtxCatF(sp18, sp98, sp98);
    guRotateF(sp18, data->unk_24, 0.0f, 0.0f, 1.0f);
    guMtxCatF(sp18, sp98, sp98);
    guMtxF2L(sp98, &gDisplayContext->matrixStack[gMatrixListPos]);

    nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);

    if (data->unk_20 > 4.0f) {
        nd_val_prim(0, 0, 255, 255, 255, 127);
    } else {
        nd_val_prim(0, 0, 255, 255, 128, 127);
    }
    t = !(s32)t; // required to match
    nd_val_env(255, 255, 139, envAlpha);

    nd_draw_list(dlist, pe_blast_131);
    nd_xf_pop(G_MTX_MODELVIEW);
}
