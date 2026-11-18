#include "model.h"
#include "animation_script.h"
#include "model.h"
#include "pm_interp_fi.h"
#include "nd_cmd.h"
#include "nd_op.h" /* HC4e2b: the typed unit of a list word */
#include "nd_tx.h"
#include "nd_gtab.h"
#include "pm_port.h"
#include "nd_mesh.h"

extern s32 gGeneralHeapGen;

typedef struct DisplayListBufferHandle {
    /* 0x0 */ s32 ttl;
    /* 0x4 */ void* addr;
} DisplayListBufferHandle; // size = 0x8

typedef DisplayListBufferHandle AnimatedMeshVertexCopyList[0x60];

s32 gAnimCount;

BSS AnimatedMeshVertexCopyList D_801533C0;
BSS AnimatedMeshVertexCopyList D_801536C0;
BSS AnimatedMeshList D_801539C0;
BSS AnimatedMeshList D_80153A00;
BSS AnimatedMeshList* gCurrentAnimMeshListPtr;
BSS s32 gAnimModelFogEnabled;
BSS s32 gAnimModelFogR;
BSS s32 gAnimModelFogG;
BSS s32 gAnimModelFogB;
BSS s32 gAnimModelFogA;
BSS s32 gAnimModelFogStart;
BSS s32 gAnimModelFogEnd;
BSS s32 gAnimVtxSegment;
BSS s32 D_80153A64;
BSS Matrix4f gAnimRotMtx;
BSS Matrix4f gAnimScaleMtx;
BSS Matrix4f gAnimTranslateMtx;
BSS Matrix4f gAnimRotScaleMtx;
BSS StaticAnimatorNode** gAnimTreeRoot;


#include "nd_rmdef.h"
#include "nd_st.h"
#include "nd_val.h"
#include "nd_xf.h"
#define AN_RM(l) ND_RM_REC(l)

AnimScript gAnimScriptDefault = {
    as_Wait(60)
    as_End
};

void appendGfx_animator_node(ModelAnimator*, AnimatorNode*, Matrix4f);

// MESHST: a built mesh (NdMesh) is freed after 3 frames: core 0 may still read it when the node drops it
static void animator_free_later(void* p) {
    DisplayListBufferHandle* handle;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(D_801533C0); i++) {
        handle = &D_801533C0[i];
        if (handle->ttl < 0) {
            break;
        }
    }

    ASSERT(i < ARRAY_COUNT(D_801533C0));
    handle->addr = p;
    handle->ttl = 3;
}

void animator_make_mirrorZ(Matrix4f mtx) {
    guMtxIdentF(mtx);
    mtx[0][0] = 1.0f;
    mtx[1][1] = 1.0f;
    mtx[2][2] = -1.0f;
    mtx[3][3] = 1.0f;
}

void animator_make_mirrorY(Matrix4f mtx) {
    guMtxIdentF(mtx);
    mtx[0][0] = 1.0f;
    mtx[1][1] = -1.0f;
    mtx[2][2] = 1.0f;
    mtx[3][3] = 1.0f;
}

void animator_make_mirrorX(Matrix4f mtx) {
    guMtxIdentF(mtx);
    mtx[0][0] = -1.0f;
    mtx[1][1] = 1.0f;
    mtx[2][2] = 1.0f;
    mtx[3][3] = 1.0f;
}

void animator_make_identity(Matrix4f mtx) {
    guMtxIdentF(mtx);
    mtx[0][0] = 1.0f;
    mtx[1][1] = 1.0f;
    mtx[2][2] = 1.0f;
    mtx[3][3] = 1.0f;
}

AnimatorNode* get_animator_child_with_id(AnimatorNode* node, s32 id) {
    s32 i;

    if (node->uniqueIndex == id) {
        return node;
    }

    for (i = 0; i < ARRAY_COUNT(node->children); i++) {
        if (node->children[i] != nullptr) {
            AnimatorNode* child = get_animator_child_with_id(node->children[i], id);

            if (child != nullptr) {
                return child;
            }

        }
    }

    return nullptr;
}

AnimatorNode* get_animator_child_for_model(AnimatorNode* node, s32 modelId) {
    s32 i;

    if (node->fcData.modelID == modelId) {
        return node;
    }

    for (i = 0; i < ARRAY_COUNT(node->children); i++) {
        if (node->children[i] != nullptr) {
            AnimatorNode* child = get_animator_child_for_model(node->children[i], modelId);

            if (child != nullptr) {
                return child;
            }

        }
    }

    return nullptr;
}

void free_animator_nodes(AnimatorNode* root) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(root->children); i++) {
        if (root->children[i] != nullptr) {
            free_animator_nodes(root->children[i]);
        }
    }
    if (root->ndMesh != nullptr && ((NdMesh*)root->ndMesh)->gen == (u32)gGeneralHeapGen) animator_free_later(root->ndMesh);
    heap_free(root);
}

void clear_animator_list(void) {
    s32 i;

    if (gGameStatusPtr->context == CONTEXT_WORLD) {
        gCurrentAnimMeshListPtr = &D_801539C0;
        for (i = 0; i < ARRAY_COUNT(D_801533C0); i++) {
            D_801533C0[i].ttl = -1;
        }

        for (i = 0; i < ARRAY_COUNT(D_801536C0); i++) {
            D_801536C0[i].ttl = -1;
        }
    } else {
        gCurrentAnimMeshListPtr = &D_80153A00;
    }

    for (i = 0; i < ARRAY_COUNT(*gCurrentAnimMeshListPtr); i++) {
        (*gCurrentAnimMeshListPtr)[i] = nullptr;
    }

    gAnimModelFogR = 10;
    gAnimModelFogG = 10;
    gAnimModelFogB = 10;
    gAnimModelFogA = 10;
    gAnimModelFogStart = 800;
    gAnimCount = 0;
    gAnimModelFogEnabled = 0;
    gAnimModelFogEnd = 1000;
}

void reset_animator_list(void) {
    if (gGameStatusPtr->context == CONTEXT_WORLD) {
        gCurrentAnimMeshListPtr = &D_801539C0;
    } else {
        gCurrentAnimMeshListPtr = &D_80153A00;
    }

    gAnimModelFogR = 10;
    gAnimModelFogG = 10;
    gAnimModelFogB = 10;
    gAnimModelFogA = 10;
    gAnimModelFogStart = 800;
    gAnimCount = 0;
    gAnimModelFogEnabled = 0;
    gAnimModelFogEnd = 1000;
}

void delete_model_animator_node(AnimatorNode* node) {
    s32 i;

    node->flags = 0;

    for (i = 0; i < ARRAY_COUNT(node->children); i++) {
        if (node->children[i] != nullptr) {
            delete_model_animator_node(node->children[i]);
            node->children[i] = nullptr;
        }
    }

    if (node->ndMesh != nullptr && ((NdMesh*)node->ndMesh)->gen == (u32)gGeneralHeapGen) animator_free_later(node->ndMesh);
    heap_free(node);
}

void delete_model_animator_nodes(ModelAnimator* animator) {
    animator->nextUniqueID = 0;

    if (animator->rootNode != nullptr) {
        delete_model_animator_node(animator->rootNode);
        animator->rootNode = nullptr;
    }
}

void delete_model_animator(ModelAnimator* animator) {
    s32 i;

    animator->nextUniqueID = 0;

    if (animator->rootNode != nullptr) {
        delete_model_animator_node(animator->rootNode);
        animator->rootNode = nullptr;

        for (i = 0; i < ARRAY_COUNT(*gCurrentAnimMeshListPtr); i++) {
            if ((*gCurrentAnimMeshListPtr)[i] == animator) {
                (*gCurrentAnimMeshListPtr)[i] = nullptr;
                break;
            }
        }

        heap_free(animator);
        gAnimCount--;
    }
}

s32 create_model_animator(s16* animPos) {
    ModelAnimator* animator;
    s32 i, j;

    for (i = 0; i < ARRAY_COUNT(*gCurrentAnimMeshListPtr); i++) {
        if ((*gCurrentAnimMeshListPtr)[i] == nullptr) {
            break;
        }
    }

    ASSERT(i < ARRAY_COUNT(*gCurrentAnimMeshListPtr));

    (*gCurrentAnimMeshListPtr)[i] = animator = heap_malloc(sizeof(*animator));
    gAnimCount++;

    ASSERT(animator != nullptr);

    animator->flags = MODEL_ANIMATOR_FLAG_UPDATE_PENDING | MODEL_ANIMATOR_FLAG_ENABLED | MODEL_ANIMATOR_FLAG_CAM_2 | MODEL_ANIMATOR_FLAG_CAM_1 | MODEL_ANIMATOR_FLAG_CAM_0;
    animator->renderMode = RENDER_MODE_ALPHATEST;
    animator->nextUpdateTime = 1.0f;
    animator->timeScale = 1.0f;
    animator->animReadPos = animPos;

    if (animPos == nullptr) {
        animator->animReadPos = gAnimScriptDefault;
    }

    animator->savedReadPos = animPos;
    animator->animationBuffer = nullptr;
    animator->baseAddr = nullptr;
    animator->fpRenderCallback = nullptr;
    animator->rootNode = nullptr;
    animator->nextUniqueID = 0;

    for (j = 0; j < ARRAY_COUNT(animator->staticNodeIDs); j++) {
        animator->staticNodeIDs[j] = j + 1;
    }

    if (gGameStatusPtr->context != CONTEXT_WORLD) {
        i |= BATTLE_ID_BIT;
    }
    return i;
}

s32 create_mesh_animator(s16* animPos, s16* animBuffer) {
    ModelAnimator* animator;
    s32 i, j;

    for (i = 0; i < ARRAY_COUNT(*gCurrentAnimMeshListPtr); i++) {
        if ((*gCurrentAnimMeshListPtr)[i] == nullptr) {
            break;
        }
    }

    ASSERT(i < ARRAY_COUNT(*gCurrentAnimMeshListPtr));

    (*gCurrentAnimMeshListPtr)[i] = animator = heap_malloc(sizeof(*animator));
    gAnimCount++;

    ASSERT(animator != nullptr);

    animator->flags = MODEL_ANIMATOR_FLAG_UPDATE_PENDING | MODEL_ANIMATOR_FLAG_ENABLED | MODEL_ANIMATOR_FLAG_CAM_2 | MODEL_ANIMATOR_FLAG_CAM_1 | MODEL_ANIMATOR_FLAG_CAM_0;
    animator->renderMode = RENDER_MODE_ALPHATEST;
    animator->baseAddr = nullptr;
    animator->fpRenderCallback = nullptr;
    animator->rootNode = nullptr;
    animator->nextUniqueID = 0;
    animator->animationBuffer = animBuffer;
    animator->nextUpdateTime = 1.0f;
    animator->timeScale = 1.0f;
    animPos = (s16*)(((s32)animPos & 0xFFFFFF) + (s32)animator->animationBuffer);
    animator->animReadPos = animPos;
    animator->savedReadPos = animPos;

    for (j = 0; j < ARRAY_COUNT(animator->staticNodeIDs); j++) {
        animator->staticNodeIDs[j] = j + 1;
    }

    if (gGameStatusPtr->context != CONTEXT_WORLD) {
        i |= BATTLE_ID_BIT;
    }
    return i;
}

AnimatorNode* add_anim_node(ModelAnimator* animator, s32 parentNodeID, AnimatorNodeBlueprint* nodeBP) {
    AnimatorNode* ret;
    AnimatorNode* child;
    s32 nextUniqueID;
    s32 nullRootNode;
    s32 i;

    ret = heap_malloc(sizeof(*ret));
    ASSERT(ret != nullptr);

    ret->flags = MODEL_ANIMATOR_FLAG_ENABLED;
    ret->displayList = nodeBP->displayList;
    ret->basePos.x = nodeBP->basePos.x;
    ret->basePos.y = nodeBP->basePos.y;
    ret->basePos.z = nodeBP->basePos.z;
    ret->pos.x = 0.0f;
    ret->pos.y = 0.0f;
    ret->pos.z = 0.0f;
    ret->rot.x = nodeBP->rot.x;
    ret->rot.y = nodeBP->rot.y;
    ret->rot.z = nodeBP->rot.z;
    ret->scale.x = 1.0f;
    ret->scale.y = 1.0f;
    ret->scale.z = 1.0f;
    ret->vertexStartOffset = -1;
    ret->ndMesh = nullptr;

    for (i = 0; i < ARRAY_COUNT(ret->children); i++) {
        ret->children[i] = nullptr;
    }

    nextUniqueID = animator->nextUniqueID;
    nextUniqueID = nextUniqueID + 1;

    if (parentNodeID == 0) {
        s32 nullRootNode = animator->rootNode == nullptr; // todo ???

        ret->uniqueIndex = nextUniqueID;

        if (nullRootNode) {
            animator->rootNode = ret;
        } else {
            child = get_animator_child_with_id(animator->rootNode, 1);
            ASSERT(child != nullptr);

            for (i = 0; i < ARRAY_COUNT(child->children); i++) {
                if (child->children[i] == nullptr) {
                    child->children[i] = ret;
                    break;
                }
            }

            ASSERT(i < ARRAY_COUNT(child->children));
        }
    } else {
        ret->uniqueIndex = nextUniqueID;
        child = get_animator_child_with_id(animator->rootNode, parentNodeID);
        ASSERT(child != nullptr);

        for (i = 0; i < ARRAY_COUNT(child->children); i++) {
            if (child->children[i] == nullptr) {
                child->children[i] = ret;
                break;
            }
        }

        ASSERT(i < ARRAY_COUNT(child->children));
    }

    animator->nextUniqueID = nextUniqueID;
    return ret;
}

void update_model_animator(s32 animatorID) {
    ModelAnimator* animator;
    s32 temp;
    s32 i;

    if (gGameStatusPtr->context != CONTEXT_WORLD && !(animatorID & BATTLE_ID_BIT)) {
        return;
    }

    animatorID &= ~BATTLE_ID_BIT;
    animator = (*gCurrentAnimMeshListPtr)[animatorID];

    if (animator == nullptr || animator->flags == 0) {
        return;
    }

    if (animator->flags & MODEL_ANIMATOR_FLAG_FREEZE_ANIMATION) {
        return;
    }

    animator->flags &= ~MODEL_ANIMATOR_FLAG_UPDATE_PENDING;
    animator->nextUpdateTime -= animator->timeScale;

    temp = 0;
    if (animator->nextUpdateTime <= 0.0f) {
        if (!(animator->flags & MODEL_ANIMATOR_FLAG_MESH)) {
            do {
                temp = step_model_animator(animator);
            } while (temp > 0);
        } else {
            animator->nextUpdateTime = 1.0f;
            reload_mesh_animator_tree(animator);
            do {
                temp = step_mesh_animator(animator);
            } while (temp > 0);
        }
    }
    if (temp == -1) {
        return;
    }

    FI_OPEN("animator", FI_TAG(0xB0000000, animatorID, animator->treeIndexPos));
    animator_update_model_transforms(animator, nullptr);
    FI_CLOSE();

    for (i = 0; i < ARRAY_COUNT(D_801533C0); i++) {
        if (D_801533C0[i].ttl >= 0) {
            D_801533C0[i].ttl--;
            if (D_801533C0[i].ttl == 0) {
                D_801533C0[i].ttl = -1;
                general_heap_free(D_801533C0[i].addr);
            }
        }
    }

    for (i = 0; i < ARRAY_COUNT(D_801536C0); i++) {
        if (D_801536C0[i].ttl >= 0) {
            D_801536C0[i].ttl--;
            if (D_801536C0[i].ttl == 0) {
                D_801536C0[i].ttl = -1;
                general_heap_free(D_801536C0[i].addr);
            }
        }
    }
}

void update_model_animator_with_transform(s32 animatorID, Mtx* mtx) {
    ModelAnimator* animator;
    s32 temp;
    s32 i;

    if (gGameStatusPtr->context != CONTEXT_WORLD && !(animatorID & BATTLE_ID_BIT)) {
        return;
    }

    animatorID &= ~BATTLE_ID_BIT;
    animator = (*gCurrentAnimMeshListPtr)[animatorID];

    if (animator == nullptr || animator->flags == 0) {
        return;
    }

    if (animator->flags & MODEL_ANIMATOR_FLAG_FREEZE_ANIMATION) {
        return;
    }

    animator->flags &= ~MODEL_ANIMATOR_FLAG_UPDATE_PENDING;
    animator->nextUpdateTime -= animator->timeScale;

    temp = 0;
    if (animator->nextUpdateTime <= 0.0f) {
        if (!(animator->flags & MODEL_ANIMATOR_FLAG_MESH)) {
            do {
                temp = step_model_animator(animator);
            } while (temp > 0);
        } else {
            animator->nextUpdateTime = 1.0f;
            reload_mesh_animator_tree(animator);
            do {
                temp = step_mesh_animator(animator);
            } while (temp > 0);
        }
    }
    if (temp == -1) {
        return;
    }

    animator_update_model_transforms(animator, mtx);

    for (i = 0; i < ARRAY_COUNT(D_801533C0); i++) {
        if (D_801533C0[i].ttl >= 0) {
            D_801533C0[i].ttl--;
            if (D_801533C0[i].ttl == 0) {
                D_801533C0[i].ttl = -1;
                general_heap_free(D_801533C0[i].addr);
            }
        }
    }

    for (i = 0; i < ARRAY_COUNT(D_801536C0); i++) {
        if (D_801536C0[i].ttl >= 0) {
            D_801536C0[i].ttl--;
            if (D_801536C0[i].ttl == 0) {
                D_801536C0[i].ttl = -1;
                general_heap_free(D_801536C0[i].addr);
            }
        }
    }
}

s32 step_model_animator(ModelAnimator* animator) {
    s16* args = animator->animReadPos;
    AnimatorNode* node;
    f32 x, y, z;
    s32 flags;
    s32 nodeId;

    switch (*args++) {
        case AS_END:
            return -1;
        case AS_SET_RENDER_MODE:
            animator->renderMode = *args++;
            animator->animReadPos = args;
            return 1;
        case AS_WAIT:
            animator->nextUpdateTime = *args++;
            animator->animReadPos = args;
            break;
        case AS_END_LOOP:
            animator->animReadPos = animator->savedReadPos;
            return 1;
        case AS_LOOP:
            animator->animReadPos = animator->savedReadPos = args;
            return 1;
        case AS_SET_FLAGS:
            flags = *args++;
            animator->animReadPos = args;
            animator->flags |= flags & 0xFFFF;
            return 1;
        case AS_SET_NODE_FLAGS:
            node = get_animator_child_with_id(animator->rootNode, animator->staticNodeIDs[*args++ - 1]);
            flags = *args++;
            node->flags |= flags;
            animator->animReadPos = args;
            return 1;
        case AS_CLEAR_NODE_FLAGS:
            node = get_animator_child_with_id(animator->rootNode, animator->staticNodeIDs[*args++ - 1]);
            flags = *args++;
            node->flags &= ~flags;
            animator->animReadPos = args;
            return 1;
        case AS_OP_19:
            animator->flags |= MODEL_ANIMATOR_FLAG_NO_FLIP;
            animator->animReadPos = args;
            return 1;
        case AS_SET_ROTATION:
            nodeId = animator->staticNodeIDs[*args++ - 1];
            x = (f32)*args++ * 180.0 / 32767.0;
            y = (f32)*args++ * 180.0 / 32767.0;
            z = (f32)*args++ * 180.0 / 32767.0;
            animator->animReadPos = args;

            node = get_animator_child_with_id(animator->rootNode, nodeId);
            ASSERT(node != nullptr);
            node->rot.x = x;
            node->rot.y = y;
            node->rot.z = z;
            return 1;
        case AS_ADD_ROTATION:
            nodeId = animator->staticNodeIDs[*args++ - 1];
            x = (f32)*args++ * 180.0 / 32767.0;
            y = (f32)*args++ * 180.0 / 32767.0;
            z = (f32)*args++ * 180.0 / 32767.0;
            animator->animReadPos = args;

            node = get_animator_child_with_id(animator->rootNode, nodeId);
            ASSERT(node != nullptr);
            node->rot.x += x;
            node->rot.y += y;
            node->rot.z += z;
            return 1;
        case AS_SET_POS:
            nodeId = animator->staticNodeIDs[*args++ - 1];
            x = *args++;
            y = *args++;
            z = *args++;
            animator->animReadPos = args;
            node = get_animator_child_with_id(animator->rootNode, nodeId);
            ASSERT(node != nullptr);
            node->pos.x = x;
            node->pos.y = y;
            node->pos.z = z;
            return 1;
        case AS_SET_SCALE:
            nodeId = animator->staticNodeIDs[*args++ - 1];
            x = (f32)*args++ * 180.0 / 32767.0;
            y = (f32)*args++ * 180.0 / 32767.0;
            z = (f32)*args++ * 180.0 / 32767.0;
            animator->animReadPos = args;

            node = get_animator_child_with_id(animator->rootNode, nodeId);
            ASSERT(node != nullptr);
            node->scale.x = x;
            node->scale.y = y;
            node->scale.z = z;
            return 1;
    }
    return 0;
}

void animator_update_model_transforms(ModelAnimator* animator, Mtx* rootTransform) {
    Matrix4f flipMtx;

    if (animator->rootNode != nullptr) {
        switch (animator->flags & (MODEL_ANIMATOR_FLAG_FLIP_Z | MODEL_ANIMATOR_FLAG_FLIP_Y | MODEL_ANIMATOR_FLAG_FLIP_X)) {
            case MODEL_ANIMATOR_FLAG_FLIP_Z:
                animator_make_mirrorZ(flipMtx);
                break;
            case MODEL_ANIMATOR_FLAG_FLIP_Y:
                animator_make_mirrorY(flipMtx);
                break;
            case MODEL_ANIMATOR_FLAG_FLIP_X:
                animator_make_mirrorX(flipMtx);
                break;
            default:
                animator_make_identity(flipMtx);
                break;
        }
        animator_node_update_model_transform(animator, flipMtx, animator->rootNode, rootTransform);
    }
}

void animator_node_update_model_transform(ModelAnimator* animator, f32 (*flipMtx)[4], AnimatorNode* node,
                                          Mtx* rootTransform) {
    Matrix4f sp10;
    s32 i;

    guRotateRPYF(gAnimRotMtx, clamp_angle(node->rot.x), clamp_angle(node->rot.y), clamp_angle(node->rot.z));
    guScaleF(gAnimScaleMtx, node->scale.x, node->scale.y, node->scale.z);
    guTranslateF(gAnimTranslateMtx, node->basePos.x + node->pos.x, node->basePos.y + node->pos.y, node->basePos.z + node->pos.z);
    guMtxCatF(gAnimScaleMtx, gAnimRotMtx, gAnimRotScaleMtx);
    guMtxCatF(gAnimRotScaleMtx, gAnimTranslateMtx, sp10);

    if (!(animator->flags & MODEL_ANIMATOR_FLAG_NO_FLIP)) {
        guMtxCatF(sp10, flipMtx, sp10);
    }

    copy_matrix(sp10, node->mtx);

    if (node->flags & MODEL_ANIMATOR_FLAG_HAS_MODEL) {
        Model* model = get_model_from_list_index(get_model_list_index_from_tree_index(node->fcData.modelID));

        copy_matrix(sp10, model->userTransformMtx);
        guMtxL2F(sp10, rootTransform);
        guMtxCatF(model->userTransformMtx, sp10, model->userTransformMtx);
        model->flags |= MODEL_FLAG_MATRIX_DIRTY;
    }

    for (i = 0; i < ARRAY_COUNT(node->children); i++) {
        if (node->children[i] != nullptr) {
            animator_node_update_model_transform(animator, sp10, node->children[i], rootTransform);
        }
    }
}

void render_animated_model(s32 animatorID, Mtx* rootTransform) {
    ModelAnimator* animator;
    RenderTask rt;
    RenderTask* rtPtr = &rt;

    if (gGameStatusPtr->context != CONTEXT_WORLD && !(animatorID & BATTLE_ID_BIT)) {
        return;
    }

    animatorID &= ~BATTLE_ID_BIT;
    animator = (*gCurrentAnimMeshListPtr)[animatorID];

    if (animator == nullptr || animator->flags == 0) {
        return;
    }

    if (!(animator->flags & MODEL_ANIMATOR_FLAG_UPDATE_PENDING)
        && animator->flags & (1 << gCurrentCamID)
        && !(animator->flags & MODEL_ANIMATOR_FLAG_HIDDEN)
    ) {
        animator->mtx = *rootTransform;
        animator->baseAddr = nullptr;
        rtPtr->appendGfxArg = animator;
        rtPtr->appendGfx = (void (*)(void*))appendGfx_animator;
        rtPtr->dist = 0;
        rtPtr->renderMode = animator->renderMode;
        queue_render_task(rtPtr);
    }
}

void render_animated_model_with_vertices(s32 animatorID, Mtx* rootTransform, s32 segment, void* baseAddr) {
    ModelAnimator* animator;
    RenderTask rt;
    RenderTask* rtPtr = &rt;

    if (gGameStatusPtr->context != CONTEXT_WORLD && !(animatorID & BATTLE_ID_BIT)) {
        return;
    }

    animatorID &= ~BATTLE_ID_BIT;
    animator = (*gCurrentAnimMeshListPtr)[animatorID];

    if (animator == nullptr || animator->flags == 0) {
        return;
    }

    if (!(animator->flags & MODEL_ANIMATOR_FLAG_UPDATE_PENDING)
        && animator->flags & (1 << gCurrentCamID)
        && !(animator->flags & MODEL_ANIMATOR_FLAG_HIDDEN)
    ) {
        animator->mtx = *rootTransform;
        gAnimVtxSegment = segment;
        animator->baseAddr = baseAddr;
        rtPtr->appendGfxArg = animator;
        rtPtr->appendGfx = (void (*)(void*))appendGfx_animator;
        rtPtr->dist = 0;
        rtPtr->renderMode = animator->renderMode;
        queue_render_task(rtPtr);
    }
}

void appendGfx_animator(ModelAnimator* animator) {
    Matrix4f sp10;

    if (animator->baseAddr != nullptr) {
        nd_val_seg(gAnimVtxSegment, VIRTUAL_TO_PHYSICAL(animator->baseAddr));
    }

    gDisplayContext->matrixStack[gMatrixListPos] = animator->mtx;
    nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    //TODO find better match
    switch (gAnimModelFogEnabled != 0) {
        case false:
            switch (animator->renderMode) {
                case RENDER_MODE_SURFACE_OPA:
                    AN_RM(Gfx_RM1_SURFACE_OPA);
                    break;
                case RENDER_MODE_DECAL_OPA:
                    AN_RM(Gfx_RM1_DECAL_OPA);
                    break;
                case RENDER_MODE_INTERSECTING_OPA:
                    AN_RM(Gfx_RM1_INTERSECTING_OPA);
                    break;
                case RENDER_MODE_ALPHATEST:
                    AN_RM(Gfx_RM1_ALPHATEST);
                    break;
                case RENDER_MODE_SURFACE_XLU_LAYER1:
                    AN_RM(Gfx_RM1_SURFACE_XLU);
                    break;
                case RENDER_MODE_DECAL_XLU:
                    AN_RM(Gfx_RM1_DECAL_XLU);
                    break;
                case RENDER_MODE_INTERSECTING_XLU:
                    AN_RM(Gfx_RM1_INTERSECTING_XLU);
                    break;
            }
            break;
        case true:
            switch (animator->renderMode) {
                case RENDER_MODE_SURFACE_OPA:
                    AN_RM(Gfx_RM3_SURFACE_OPA);
                    break;
                case RENDER_MODE_DECAL_OPA:
                    AN_RM(Gfx_RM3_DECAL_OPA);
                    break;
                case RENDER_MODE_INTERSECTING_OPA:
                    AN_RM(Gfx_RM3_INTERSECTING_OPA);
                    break;
                case RENDER_MODE_ALPHATEST:
                    AN_RM(Gfx_RM3_ALPHATEST);
                    break;
                case RENDER_MODE_SURFACE_XLU_LAYER1:
                    AN_RM(Gfx_RM3_SURFACE_XLU);
                    break;
                case RENDER_MODE_DECAL_XLU:
                    AN_RM(Gfx_RM3_DECAL_XLU);
                    break;
                case RENDER_MODE_INTERSECTING_XLU:
                    AN_RM(Gfx_RM3_INTERSECTING_XLU);
                    break;
            }

            nd_val_fog(gAnimModelFogR, gAnimModelFogG, gAnimModelFogB, gAnimModelFogA);
            nd_pso_fogpos(gAnimModelFogStart, gAnimModelFogEnd);
            break;
    }

    guMtxL2F(sp10, &animator->mtx);
    appendGfx_animator_node(animator, animator->rootNode, sp10);
    nd_xf_pop(G_MTX_MODELVIEW);
}

void appendGfx_animator_node(ModelAnimator* animator, AnimatorNode* node, Matrix4f mtx) {
    s32 i;

    if (node->flags & MODEL_ANIMATOR_FLAG_HIDDEN) {
        for (i = 0; i < ARRAY_COUNT(node->children); i++) {
            if (node->children[i] != nullptr) {
                guMtxCatF(node->mtx, mtx, node->mtx);
                appendGfx_animator_node(animator, node->children[i], node->mtx);
            }
        }
        return;
    }

    guMtxCatF(node->mtx, mtx, node->mtx);
    guMtxF2L(node->mtx, &gDisplayContext->matrixStack[gMatrixListPos]);
    nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    {
        const int cull = (animator->flags & MODEL_ANIMATOR_FLAG_CULL_BACK) != 0;
        nd_tex_on(0, 0, 0, G_TX_RENDERTILE, G_OFF);
        nd_pso_lod(G_TL_TILE);
        nd_pso_lut(G_TT_NONE);
        nd_pso_geom_clr(G_LIGHTING);
        nd_pso_geom_set(cull ? G_CULL_BACK : 0);
        if (!gAnimModelFogEnabled) nd_pso_cc(G_CC_DECALRGBA, G_CC_DECALRGBA);
        else nd_pso_cc(PM_CC_42, PM_CC2_MULTIPLY_SHADE);
    }

    if (animator->fpRenderCallback != nullptr) animator->fpRenderCallback(animator->renderCallbackArg);


    if (node->displayList != nullptr) {
        if (node->vertexStartOffset < 0) {
            /* S6-4: a static node list (its vertices sit in the model data behind gAnimVtxSegment): the walker draws it from the static pools. The other form
             * (vertexStartOffset >= 0) is a fresh heap copy of the list with per frame vertices every frame: named fallback ent_dynvtx, the walker runs it. */
            nd_rec_ent_draw(node->displayList, gAnimVtxSegment, animator->baseAddr != nullptr ? VIRTUAL_TO_PHYSICAL(animator->baseAddr) : 0, 1); /* DIRECT D3: word 2 w0 1 = the animator form (log); the record and its G_DL go to the frame stream */
        } else {
            const NdNDev* mesh = nd_ntab_find(node->displayList); /* MESH1: the device units of the list (a moved list's key is its entry) */
            const uint32_t* units = mesh != nullptr ? mesh->u : (const uint32_t*)node->displayList;
            u32 nu = mesh != nullptr ? (u32)mesh->nu : 0;

            if (mesh == nullptr) {
                while ((units[2 * nu] >> 24) != ND_FS_END) {
                    nu++;
                }
            }
            { static int s_mesh_log; if (!s_mesh_log) { s_mesh_log = 1; pm_log_evt("anim mesh f%u units %d dev %d", pm_frame_count(), (int)nu, mesh != nullptr); } }
            {
                NdMesh* m = (NdMesh*)node->ndMesh;
                if (m != nullptr && m->gen != (u32)gGeneralHeapGen) m = nullptr; /* the heap was made again: the buffer is gone */
                else if (m != nullptr && m->base != animator->baseAddr) { animator_free_later(m); m = nullptr; }
                if (m == nullptr) {
                    m = general_heap_malloc(nd_mesh_size(units, nu));
                    ASSERT(m != nullptr);
                    nd_mesh_build(m, units, nu, node->fcData.vtxList, node->vertexStartOffset, animator->baseAddr);
                    m->gen = (u32)gGeneralHeapGen;
                    node->ndMesh = m;
                }
                nd_rec_ndev(&m->d);
            }
        }
    }

    for (i = 0; i < ARRAY_COUNT(node->children); i++) {
        if (node->children[i] != nullptr) {
            appendGfx_animator_node(animator, node->children[i], node->mtx);
        }
    }
}

AnimatorNode* get_animator_node_for_tree_index(ModelAnimator* animator, s32 arg1) {
    return get_animator_child_with_id(animator->rootNode, animator->staticNodeIDs[arg1 - 1]);
}

AnimatorNode* get_animator_node_with_id(ModelAnimator* animator, s32 id) {
    return get_animator_child_for_model(animator->rootNode, id);
}

void set_animator_tree_to_node_map(ModelAnimator* animator, s32* nodeIDs, s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        animator->staticNodeIDs[i] = *nodeIDs;
        nodeIDs++;
    }
}

ModelAnimator* get_animator_by_index(s32 animModelID) {
    return (*gCurrentAnimMeshListPtr)[animModelID & ~BATTLE_ID_BIT];
}

ModelAnimator* set_animator_render_callback(s32 animModelID, void* callbackArg, void (*callbackFunc)(void*)) {
    ModelAnimator* ret = (*gCurrentAnimMeshListPtr)[animModelID & ~BATTLE_ID_BIT];

    ret->fpRenderCallback = callbackFunc;
    ret->renderCallbackArg = callbackArg;
    return ret;
}

void enable_anim_model_fog(void) {
    gAnimModelFogEnabled = true;
}

void disable_anim_model_fog(void) {
    gAnimModelFogEnabled = false;
}

void set_anim_model_fog_dist(s32 start, s32 end) {
    gAnimModelFogStart = start;
    gAnimModelFogEnd = end;
}

void set_anim_model_fog_color(s32 r, s32 g, s32 b, s32 a) {
    gAnimModelFogR = r;
    gAnimModelFogG = g;
    gAnimModelFogB = b;
    gAnimModelFogA = a;
}

s32 is_anim_model_fog_enabled(void) {
    return gAnimModelFogEnabled;
}

void get_anim_model_fog_distance(s32* start, s32* end) {
    *start = gAnimModelFogStart;
    *end = gAnimModelFogEnd;
}

void get_anim_model_fog_color(s32* r, s32* g, s32* b, s32* a) {
    *r = gAnimModelFogR;
    *g = gAnimModelFogG;
    *b = gAnimModelFogB;
    *a = gAnimModelFogA;
}

void set_animator_flags(s32 index, s32 bits) {
    ModelAnimator* animator = (*gCurrentAnimMeshListPtr)[index & ~BATTLE_ID_BIT];

    animator->flags |= bits;
}

void clear_animator_flags(s32 index, s32 bits) {
    ModelAnimator* animator = (*gCurrentAnimMeshListPtr)[index & ~BATTLE_ID_BIT];

    animator->flags &= ~bits;
}

void play_model_animation(s32 index, s16* animPos) {
    ModelAnimator* animator = (*gCurrentAnimMeshListPtr)[index & ~BATTLE_ID_BIT];

    if (animator->animationBuffer != nullptr) {
        animPos = (s16*) (((s32)animPos & 0xFFFFFF) + (s32)animator->animationBuffer); // TODO: array access? / cleanup
    }
    animator->animReadPos = animPos;
    animator->savedReadPos = animPos;
    animator->treeIndexPos = 0;
    animator->nextUpdateTime = 1.0f;
}

void play_model_animation_starting_from(s32 index, s16* animPos, s32 framesToSkip) {
    s32 indexMasked = index & ~BATTLE_ID_BIT;
    ModelAnimator* animator = (*gCurrentAnimMeshListPtr)[indexMasked];
    s32 i;

    if (animator->animationBuffer != nullptr) {
        animPos = (s16*) (((s32)animPos & 0xFFFFFF) + (s32)animator->animationBuffer); // TODO: array access? / cleanup
    }

    animator->animReadPos = animPos;
    animator->savedReadPos = animPos;
    animator->treeIndexPos = 0;
    animator->nextUpdateTime = 1.0f;

    for (i = 0; i < framesToSkip; i++) {
        update_model_animator(indexMasked);
    }
}

void load_model_animator_node(StaticAnimatorNode* node, ModelAnimator* animator, s32 parentNodeID, s32* treeIndexToNodeIDs) {
    AnimatorNodeBlueprint bp;
    AnimatorNodeBlueprint* bpPtr = &bp;
    AnimatorNode* newNode;
    s32 i;

    if (node != nullptr) {
        bpPtr->displayList = node->displayList;
        bpPtr->basePos.x = 0.0f;
        bpPtr->basePos.y = 0.0f;
        bpPtr->basePos.z = 0.0f;
        bpPtr->rot.x = ((f32) node->rot.x * 180.0) / 32767.0;
        bpPtr->rot.y = ((f32) node->rot.y * 180.0) / 32767.0;
        bpPtr->rot.z = ((f32) node->rot.z * 180.0) / 32767.0;

        newNode = add_anim_node(animator, parentNodeID, bpPtr);

        if (node->modelID != 0) {
            newNode->fcData.modelID = node->modelID - 1;
            newNode->flags |= MODEL_ANIMATOR_FLAG_HAS_MODEL;
        }

        i = 0;
        while (gAnimTreeRoot[i] != node) {
            i++;
        }

        treeIndexToNodeIDs[i] = newNode->uniqueIndex;

        if (node->child != nullptr) {
            load_model_animator_node(node->child, animator, newNode->uniqueIndex, treeIndexToNodeIDs);
        }

        if (node->sibling != nullptr) {
            load_model_animator_node(node->sibling, animator, parentNodeID, treeIndexToNodeIDs);
        }
    }
}

void load_model_animator_tree(s32 index, StaticAnimatorNode** tree) {
    ModelAnimator* animator = (*gCurrentAnimMeshListPtr)[index & ~BATTLE_ID_BIT];
    s32 nodeIDs[ARRAY_COUNT(animator->staticNodeIDs)];

    if (animator == nullptr || animator->flags == 0) {
        return;
    }

    gAnimTreeRoot = tree;
    load_model_animator_node(*tree, animator, 0, nodeIDs);
    set_animator_tree_to_node_map(animator, nodeIDs, ARRAY_COUNT(animator->staticNodeIDs));
}

void load_mesh_animator_node(StaticAnimatorNode* node, ModelAnimator* animator, s32 parentNodeID, s32* treeIndexToNodeIDs) {
    if (node != nullptr) {
        if (node->child != nullptr && parentNodeID == 0) {
            load_mesh_animator_node(node->child, animator, 0, treeIndexToNodeIDs);
        } else {
            do {
                animator->staticNodes[parentNodeID] = node;
                node = node->sibling;
                parentNodeID++;
            } while (node != nullptr);
        }
    }
}

void load_mesh_animator_tree(s32 index, StaticAnimatorNode** tree) {
    s32 indexMasked = index & ~BATTLE_ID_BIT;
    ModelAnimator* animator = (*gCurrentAnimMeshListPtr)[indexMasked];
    s32 nodeIDs[ARRAY_COUNT(animator->staticNodeIDs)];
    s32 i;

    if (animator == nullptr || animator->flags == 0) {
        return;
    }

    if ((*tree)->vertexStartOffset == 0) {
        load_model_animator_tree(indexMasked, tree);
        return;
    }

    gAnimTreeRoot = tree;
    animator->staticRoot = tree;
    animator->treeIndexPos = 0;
    animator->savedTreePos = 0;

    for (i = 0; i < ARRAY_COUNT(animator->staticNodes); i++) {
        animator->staticNodes[i] = nullptr;
    }

    load_mesh_animator_node(*gAnimTreeRoot, animator, 0, nodeIDs);
    animator->flags |= MODEL_ANIMATOR_FLAG_MESH;
}

void reload_mesh_animator_node(StaticAnimatorNode* node, ModelAnimator* animator, s32 parentNodeID, s32* treeIndexToNodeIDs) {
    AnimatorNodeBlueprint bp;
    AnimatorNodeBlueprint* bpPtr = &bp;
    AnimatorNode* newNode;
    s32 i;

    if (node != nullptr) {
        bpPtr->displayList = node->displayList;
        bpPtr->basePos.x = 0.0f;
        bpPtr->basePos.y = 0.0f;
        bpPtr->basePos.z = 0.0f;
        bpPtr->rot.x = ((f32) node->rot.x * 180.0) / 32767.0;
        bpPtr->rot.y = ((f32) node->rot.y * 180.0) / 32767.0;
        bpPtr->rot.z = ((f32) node->rot.z * 180.0) / 32767.0;

        newNode = add_anim_node(animator, parentNodeID, bpPtr);
        newNode->vertexStartOffset = node->vertexStartOffset;
        newNode->fcData.vtxList = node->vtxList;

        i = 0;
        while (gAnimTreeRoot[i] != node) {
            i++;
        }

        treeIndexToNodeIDs[i] = newNode->uniqueIndex;

        if (node->child != nullptr) {
            reload_mesh_animator_node(node->child, animator, newNode->uniqueIndex, treeIndexToNodeIDs);
        }
    }
}

void reload_mesh_animator_tree(ModelAnimator* animator) {
    s32 nodeIDs[ARRAY_COUNT(animator->staticNodeIDs)];
    s32 i;

    delete_model_animator_nodes(animator);
    gAnimTreeRoot = animator->staticRoot;

    for (i = 0; i < ARRAY_COUNT(animator->staticNodes); i++) {
        nodeIDs[i] = 0;
    }

    reload_mesh_animator_node(animator->staticNodes[animator->treeIndexPos], animator, 0, nodeIDs);
    nodeIDs[0] = -1;
    set_animator_tree_to_node_map(animator, nodeIDs, ARRAY_COUNT(nodeIDs));
}

s32 step_mesh_animator(ModelAnimator* animator) {
    s16* args = animator->animReadPos;
    s16* oldPos = animator->animReadPos;
    AnimatorNode* node;
    f32 x, y, z;
    s32 flags;
    s32 nodeId;

    switch (*args++) {
        case AS_END:
            return -1;
        case AS_SET_RENDER_MODE:
            animator->renderMode = *args++;
            animator->animReadPos = args;
            return 1;
        case AS_WAIT:
            args++;
            animator->animReadPos = args;
            return 1;
        case AS_END_LOOP:
            animator->animReadPos = animator->savedReadPos;
            animator->treeIndexPos = animator->savedTreePos;
            reload_mesh_animator_tree(animator);
            return 1;
        case AS_OP_4:
            animator->animReadPos = animator->savedReadPos;
            animator->treeIndexPos = animator->savedTreePos;
            break;
        case AS_LOOP:
            animator->animReadPos = animator->savedReadPos = args;
            animator->savedTreePos = animator->treeIndexPos;
            return 1;
        case AS_SET_FLAGS:
            flags = *args++;
            animator->animReadPos = args;
            animator->flags |= flags & 0xFFFF;
            return 1;
        case AS_SET_NODE_FLAGS:
            node = get_animator_child_with_id(animator->rootNode, animator->staticNodeIDs[*args++ - 1]);
            flags = *args++;
            node->flags |= flags;
            animator->animReadPos = args;
            return 1;
        case AS_CLEAR_NODE_FLAGS:
            node = get_animator_child_with_id(animator->rootNode, animator->staticNodeIDs[*args++ - 1]);
            flags = *args++;
            node->flags &= ~flags;
            animator->animReadPos = args;
            return 1;
        case AS_OP_19:
            animator->flags |= MODEL_ANIMATOR_FLAG_NO_FLIP;
            animator->animReadPos = args;
            return 1;
        case AS_SET_ROTATION:
            nodeId = animator->staticNodeIDs[*args++ - 1];
            x = (f32)*args++ * 180.0 / 32767.0;
            y = (f32)*args++ * 180.0 / 32767.0;
            z = (f32)*args++ * 180.0 / 32767.0;
            animator->animReadPos = args;
            if (nodeId != 0xFF) {
                node = get_animator_child_with_id(animator->rootNode, nodeId);
                if (node != nullptr) {
                    node->rot.x = x;
                    node->rot.y = y;
                    node->rot.z = z;
                    return 1;
                } else {
                    animator->animReadPos = oldPos;
                    animator->treeIndexPos++;
                    break;
                }
            }
            return 1;
        case AS_ADD_ROTATION:
            nodeId = animator->staticNodeIDs[*args++ - 1];
            x = (f32)*args++ * 180.0 / 32767.0;
            y = (f32)*args++ * 180.0 / 32767.0;
            z = (f32)*args++ * 180.0 / 32767.0;
            animator->animReadPos = args;
            if (nodeId != 0xFF) {
                node = get_animator_child_with_id(animator->rootNode, nodeId);
                if (node != nullptr) {
                    node->rot.x += x;
                    node->rot.y += y;
                    node->rot.z += z;
                    return 1;
                } else {
                    animator->animReadPos = oldPos;
                    animator->treeIndexPos++;
                    break;
                }
            }
            return 1;
        case AS_SET_POS:
            nodeId = animator->staticNodeIDs[*args++ - 1];
            x = *args++;
            y = *args++;
            z = *args++;
            animator->animReadPos = args;
            if (nodeId != 0xFF) {
                node = get_animator_child_with_id(animator->rootNode, nodeId);
                if (node != nullptr) {
                    node->pos.x = x;
                    node->pos.y = y;
                    node->pos.z = z;
                    return 1;
                } else {
                    animator->animReadPos = oldPos;
                    animator->treeIndexPos++;
                    break;
                }
            }
            return 1;
        case AS_SET_SCALE:
            nodeId = animator->staticNodeIDs[*args++ - 1];
            x = (f32)*args++ * 180.0 / 32767.0;
            y = (f32)*args++ * 180.0 / 32767.0;
            z = (f32)*args++ * 180.0 / 32767.0;
            animator->animReadPos = args;
            if (nodeId != 0xFF) {
                node = get_animator_child_with_id(animator->rootNode, nodeId);
                if (node != nullptr) {
                    node->scale.x = x;
                    node->scale.y = y;
                    node->scale.z = z;
                    return 1;
                } else {
                    animator->animReadPos = oldPos;
                    animator->treeIndexPos++;
                    break;
                }
            }
            return 1;
    }
    return 0;
}
