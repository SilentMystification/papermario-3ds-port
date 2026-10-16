#include "common.h"
#include "pm_port.h"
#include "audio/public.h"

/* Systems kmr_20 reaches that are not linked yet. Each is replaced by its real file later. */


void enable_partner_ai(void) {}
void func_800EF314(void) {}
void func_800EF3E4(void) {}
void func_800EF414(s32 posX, s32 posZ) {}

void create_standard_popup_menu(void* menu) {}

Mtx Entity_BombableRock_FragmentMatrices[8];
Gfx* Entity_BombableRock_FragmentsRender[8];
void func_800EF300(void) {}
void partner_set_tether_distance(f32 dist) {}
void hide_popup_menu(void) {}
void destroy_popup_menu(void) {}


/* bed.c and records.c include extracted textures that are not in the tree. */
EvtScript kmr_20_EVS_Inspect_Records = { Return End };
EvtScript kmr_20_EVS_SetupBed = { Return End };

/* Entity model display lists live in ROM entity_model_* segments; no model draws yet. */
Gfx Entity_BrickBlock_Render[] = { gsSPEndDisplayList() };
Gfx Entity_Hammer3Block_Render[] = { gsSPEndDisplayList() };


Gfx Entity_ScriptSpring_RenderPart1[] = { gsSPEndDisplayList() };
Gfx Entity_ScriptSpring_RenderPart2[] = { gsSPEndDisplayList() };
Gfx Entity_ScriptSpring_RenderPart3[] = { gsSPEndDisplayList() };
Gfx Entity_ScriptSpring_RenderPart4[] = { gsSPEndDisplayList() };
Gfx Entity_ScriptSpring_RenderPart5[] = { gsSPEndDisplayList() };
Gfx Entity_ScriptSpring_RenderPart6[] = { gsSPEndDisplayList() };
Gfx Entity_ScriptSpring_RenderPart7[] = { gsSPEndDisplayList() };
Gfx* Entity_ShatteringBrickBlock_FragmentsRender[16];
Mtx Entity_ShatteringBrickBlock_FragmentsMatrices[16];
Gfx* Entity_ShatteringHammer1Block_FragmentsRender[16];
Mtx Entity_ShatteringHammer1Block_FragmentsMatrices[16];
Gfx* Entity_ShatteringHammer2Block_FragmentsRender[16];
Mtx Entity_ShatteringHammer2Block_FragmentsMatrices[16];
Gfx* Entity_ShatteringHammer3Block_FragmentsRender[16];
Mtx Entity_ShatteringHammer3Block_FragmentsMatrices[16];
void clear_world_menus(void) {}
BackgroundHeader gBackgroundImage;
unsigned char pm_world_entity_heap[0x18000] __attribute__((aligned(16)));
