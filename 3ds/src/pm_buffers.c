#include "common.h"
#include "pm_port.h"
#include "audio/public.h"

/* Real N64 address 0x80200000 (undefined_syms.txt). The decoded background file (header, CI8 raster, palette) lands here. */
unsigned char gBackgroundImage_buf[0x40000] __asm__("gBackgroundImage") __attribute__((aligned(16)));

unsigned char pm_world_entity_heap[0x18000] __attribute__((aligned(16)));
