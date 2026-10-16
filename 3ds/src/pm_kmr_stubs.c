#include "common.h"
#include "pm_port.h"
#include "audio/public.h"

/* Systems kmr_20 reaches that are not linked yet. Each is replaced by its real file later. */




Mtx Entity_BombableRock_FragmentMatrices[8];
Gfx* Entity_BombableRock_FragmentsRender[8];


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
/* Real N64 address 0x80200000 (undefined_syms.txt). The decoded background file (header, CI8 raster, palette) lands here. */
unsigned char gBackgroundImage_buf[0x40000] __asm__("gBackgroundImage") __attribute__((aligned(16)));

unsigned char pm_world_entity_heap[0x18000] __attribute__((aligned(16)));
