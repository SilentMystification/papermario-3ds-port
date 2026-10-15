#include "common.h"
#include "entity.h"

/* entity/Shadow.c needs extracted shadow textures. Shadows exist but draw nothing. */

void entity_Shadow_init(Shadow* shadow) {
    shadow->scale.x = 0.1f;
    shadow->scale.y = 0.1f;
    shadow->scale.z = 0.1f;
}

s32 entity_can_collide_with_jumping_player(Entity* entity) {
    return (entity->collisionFlags & ENTITY_COLLISION_PLAYER_TOUCH_CEILING) && (gPlayerStatus.flags & PS_FLAG_JUMPING);
}

Gfx Entity_RenderNone[] = { gsSPEndDisplayList() };

EntityModelScript Entity_RenderNone_Script = {
    ems_Draw(Entity_RenderNone, 60)
    ems_Restart
    ems_End
};

#define SHADOW(name) \
    ShadowBlueprint name = { \
        .flags = ENTITY_FLAG_DISABLE_COLLISION, \
        .renderCommandList = Entity_RenderNone_Script, \
        .animModelNode = nullptr, \
        .onCreateCallback = entity_Shadow_init, \
        .entityType = ENTITY_TYPE_SHADOW, \
        .aabbSize = { 25, 10, 25 } \
    }

SHADOW(CircularShadowA);
SHADOW(CircularShadowB);
SHADOW(SquareShadow);
