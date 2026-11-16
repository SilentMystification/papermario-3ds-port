#include "common.h"
#include "nd_gtab.h"
#include "entity.h"
#include "include_asset.h"

void entity_Shadow_init(Shadow* shadow) {
    shadow->scale.x = 0.1f;
    shadow->scale.y = 0.1f;
    shadow->scale.z = 0.1f;
}

s32 entity_can_collide_with_jumping_player(Entity* entity) {
    if ((entity->collisionFlags & ENTITY_COLLISION_PLAYER_TOUCH_CEILING)
        && (gPlayerStatus.flags & PS_FLAG_JUMPING)
    ) {
        return true;
    }
    return false;
}

INCLUDE_IMG("entity/shadow/square.png", D_802E9170);
INCLUDE_IMG("entity/shadow/circle.png", D_802E91F0);
#include "entity/vtx/shadow1.vtx.inc.c"
#include "entity/vtx/shadow2.vtx.inc.c"
INCLUDE_IMG("entity/shadow/square_big.png", D_802E92F0);
INCLUDE_IMG("entity/shadow/circle_big.png", D_802E94F0);

/* EN2: the texture load of each shadow is plain N64 words. */
NDN_DECL(Entity_RenderSquareShadow);

NDN_DECL(Entity_RenderCircularShadow);

NDN_DECL(Entity_RenderNone);

EntityModelScript Entity_RenderNone_Script = {
    ems_Draw(NDN(Entity_RenderNone), 60)
    ems_Restart
    ems_End
};

//TODO split files
s32 D_802E9844_padding[] = { 0, 0, 0};

EntityModelScript Entity_CircularShadowA_Render = {
    ems_SetFlags(ENTITY_MODEL_FLAG_FOG_DISABLED)
    ems_SetRenderMode(RENDER_MODE_SHADOW)
    ems_Draw(NDN(Entity_RenderCircularShadow), 60)
    ems_Restart
    ems_End
};

EntityModelScript Entity_CircularShadowB_Render = {
    ems_SetFlags(ENTITY_MODEL_FLAG_FOG_DISABLED)
    ems_SetRenderMode(RENDER_MODE_SHADOW)
    ems_Draw(NDN(Entity_RenderCircularShadow), 60)
    ems_Restart
    ems_End
};

EntityModelScript Entity_SquareShadow_Render = {
    ems_SetFlags(ENTITY_MODEL_FLAG_FOG_DISABLED)
    ems_SetRenderMode(RENDER_MODE_SHADOW)
    ems_Draw(NDN(Entity_RenderSquareShadow), 60)
    ems_Restart
    ems_End
};

ShadowBlueprint CircularShadowA = {
    .flags = ENTITY_FLAG_DISABLE_COLLISION,
    .renderCommandList = Entity_CircularShadowA_Render,
    .animModelNode =  nullptr,
    .onCreateCallback = entity_Shadow_init,
    .entityType = ENTITY_TYPE_SHADOW,
    .aabbSize = { 25, 10, 25 }
};

ShadowBlueprint CircularShadowB = {
    .flags = ENTITY_FLAG_DISABLE_COLLISION,
    .renderCommandList = Entity_CircularShadowB_Render,
    .animModelNode =  nullptr,
    .onCreateCallback = entity_Shadow_init,
    .entityType = ENTITY_TYPE_SHADOW,
    .aabbSize = { 25, 10, 25 }
};

ShadowBlueprint SquareShadow = {
    .flags = ENTITY_FLAG_DISABLE_COLLISION,
    .renderCommandList = Entity_SquareShadow_Render,
    .animModelNode =  nullptr,
    .onCreateCallback = entity_Shadow_init,
    .entityType = ENTITY_TYPE_SHADOW,
    .aabbSize = { 25, 10, 25 }
};
